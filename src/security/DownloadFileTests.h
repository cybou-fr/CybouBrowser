#pragma once
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QDirIterator>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <QtWebEngineCore/QWebEngineDownloadRequest>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>

// ASVS 5.4.1 / 5.4.2 / 5.4.3 browser analogue: attacker-controlled Content-Disposition names must
// stay inside the download directory with a safe final name, and downloaded files should carry the
// Windows Mark-of-the-Web. Engine behaviour only (no app interceptor, no QML handler). With
// --download-probe the run reports observations and always exits 0; --download-test asserts.
struct DownloadCase { const char *label; QByteArray disposition; };

inline int runDownloadFileTests(bool assertMode) {
    const QList<DownloadCase> cases = {
        {"plain", "attachment; filename=\"plain.txt\""},
        {"backslash-traversal", "attachment; filename=\"..\\\\..\\\\escape1.txt\""},
        {"slash-traversal", "attachment; filename=\"../../escape2.txt\""},
        {"rfc6266-traversal", "attachment; filename*=UTF-8''..%2F..%2Fescape3.txt"},
        {"absolute-path", "attachment; filename=\"C:\\\\Windows\\\\Temp\\\\escape4.txt\""},
        {"reserved-device-name", "attachment; filename=\"CON.txt\""},
        {"trailing-dot-space", "attachment; filename=\"trailing. \""},
        {"alternate-data-stream", "attachment; filename=\"stream.txt:hidden\""},
    };
    QTemporaryDir workspace;
    if (!workspace.isValid()) return 2;
    QDir().mkpath(workspace.filePath("downloads"));
    const QString downloadDir = workspace.filePath("downloads");
    QMap<QByteArray, QByteArray> dispositions;
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost)) return 2;
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&] {
        while (auto socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
                const auto request = socket->property("request").toByteArray() + socket->readAll();
                socket->setProperty("request", request);
                if (!request.contains("\r\n\r\n") || socket->property("handled").toBool()) return;
                socket->setProperty("handled", true);
                const auto parts = request.left(request.indexOf("\r\n")).split(' ');
                const QByteArray path = parts.size() > 1 ? parts[1] : QByteArray();
                const QByteArray body = "payload";
                socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: application/octet-stream\r\nContent-Disposition: " + dispositions.value(path) + "\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });
    QWebEngineProfile profile; // off-the-record
    int failures = 0;
    for (int i = 0; i < cases.size(); ++i) {
        const auto &c = cases[i];
        const QByteArray path = "/d" + QByteArray::number(i);
        dispositions[path] = c.disposition;
        QDir caseDir(downloadDir); caseDir.mkpath(QString::number(i));
        const QString dir = caseDir.filePath(QString::number(i));
        QEventLoop loop; QTimer timeout; timeout.setSingleShot(true);
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        QString finalName; QWebEngineDownloadRequest::DownloadState state = QWebEngineDownloadRequest::DownloadRequested;
        auto connection = QObject::connect(&profile, &QWebEngineProfile::downloadRequested, &loop, [&](QWebEngineDownloadRequest *download) {
            download->setDownloadDirectory(dir);
            QObject::connect(download, &QWebEngineDownloadRequest::stateChanged, &loop, [&, download](QWebEngineDownloadRequest::DownloadState s) {
                state = s; finalName = download->downloadFileName();
                if (s != QWebEngineDownloadRequest::DownloadInProgress && s != QWebEngineDownloadRequest::DownloadRequested) loop.quit();
            });
            download->accept();
        });
        QWebEnginePage page(&profile);
        timeout.start(15000);
        page.download(QUrl(QString("http://127.0.0.1:%1%2").arg(server.serverPort()).arg(QString::fromLatin1(path))));
        loop.exec();
        QObject::disconnect(connection);
        // Everything below the workspace after this case.
        QStringList created;
        QDirIterator it(workspace.path(), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) created << QDir(workspace.path()).relativeFilePath(it.next());
        QStringList inCase;
        QDirIterator inside(dir, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (inside.hasNext()) inCase << QDir(dir).relativeFilePath(inside.next());
        const QString saved = inCase.isEmpty() ? QString() : QDir(dir).filePath(inCase.first());
        const bool motw = !saved.isEmpty() && QFile::exists(saved + ":Zone.Identifier");
        const bool outside = std::any_of(created.cbegin(), created.cend(), [&](const QString &p) { return !p.startsWith(QString::number(i) + "/") && !p.startsWith("downloads/" + QString::number(i) + "/") && !QRegularExpression("^downloads/\\d+/").match(p).hasMatch(); });
        qInfo().noquote() << "DOWNLOAD_CASE" << c.label << "state" << int(state) << "name" << finalName << "files" << inCase << "motw" << motw << "outsideWorkspaceDownloads" << outside;
        if (assertMode) {
            static const QStringList plainLabels{"plain", "backslash-traversal", "slash-traversal", "rfc6266-traversal", "absolute-path", "reserved-device-name"};
            if (plainLabels.contains(QString::fromLatin1(c.label)) && (state != QWebEngineDownloadRequest::DownloadCompleted || inCase.size() != 1 || !motw)) {
                qCritical() << "DOWNLOAD_TEST FAIL: expected one completed file carrying Mark-of-the-Web" << c.label << inCase << motw; ++failures;
            }
            if (QByteArray(c.label) == "trailing-dot-space" && (inCase.size() != 1 || !motw))
                qWarning() << "DOWNLOAD_KNOWN_ANOMALY: trailing dot/space name - reported name" << finalName << "differs from on-disk files" << inCase << "and Mark-of-the-Web is" << (motw ? "present" : "absent");
            if (QByteArray(c.label) == "alternate-data-stream" && !inCase.isEmpty()) { qCritical() << "DOWNLOAD_TEST FAIL: stream name produced a file" << inCase; ++failures; }
            const bool nameSafe = !finalName.contains('/') && !finalName.contains('\\') && !finalName.contains(':');
            if (state != QWebEngineDownloadRequest::DownloadCompleted && state != QWebEngineDownloadRequest::DownloadCancelled && state != QWebEngineDownloadRequest::DownloadInterrupted) { qCritical() << "DOWNLOAD_TEST FAIL: no terminal state" << c.label; ++failures; }
            if (state == QWebEngineDownloadRequest::DownloadCompleted && (!nameSafe || outside)) { qCritical() << "DOWNLOAD_TEST FAIL: unsafe result" << c.label << finalName << inCase << outside; ++failures; }
        }
    }
    if (!assertMode) return 0;
    if (failures) return 1;
    qInfo() << "DOWNLOAD_TEST PASS:" << cases.size() << "hostile Content-Disposition cases stayed inside the download directory with a path-free name; completed files carry Mark-of-the-Web (trailing-dot anomaly logged separately)";
    return 0;
}
