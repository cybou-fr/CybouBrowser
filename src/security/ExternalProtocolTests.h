#pragma once
#include <QDesktopServices>
#include <QEventLoop>
#include <QMap>
#include <QObject>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include <QtWebEngineCore/QWebEngineSettings>

// ASVS 3.7.3 browser analogue: a web page must not be able to launch an external application
// through an unknown URL scheme. Fabricated schemes only, each with a QDesktopServices handler that
// only counts calls, so no real application can be started even if a policy fails. The AllowAll run
// is the control proving a launch attempt is observable for every trigger method.
class ExternalSchemeCounter final : public QObject {
    Q_OBJECT
public:
    int hits = 0;
    QStringList urls;
public slots:
    void handle(const QUrl &url) { ++hits; urls << url.toString(); }
};

inline int runExternalProtocolTests() {
    ExternalSchemeCounter counter;
    for (const char *scheme : {"cybou-ext-a", "cybou-ext-b"}) QDesktopServices::setUrlHandler(QString::fromLatin1(scheme), &counter, "handle");
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost)) return 2;
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&server] {
        while (auto socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket] {
                socket->readAll();
                const QByteArray body("<!doctype html><title>external schemes</title><body>ready</body>");
                socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });
    struct Trigger { const char *name; const char *script; };
    const QList<Trigger> triggers = {
        {"location.href", "location.href='cybou-ext-a://one'"},
        {"location.assign", "location.assign('cybou-ext-b://two')"},
        {"window.open", "window.open('cybou-ext-a://three')"},
        {"iframe.src", "{const f=document.createElement('iframe');f.src='cybou-ext-b://four';document.body.appendChild(f)}"},
        {"anchor.click", "{const a=document.createElement('a');a.href='cybou-ext-a://five';document.body.appendChild(a);a.click()}"},
        {"form.submit", "{const f=document.createElement('form');f.action='cybou-ext-b://six';document.body.appendChild(f);f.submit()}"},
        {"meta-refresh", "{const m=document.createElement('meta');m.httpEquiv='refresh';m.content='0;url=cybou-ext-a://seven';document.head.appendChild(m)}"},
    };
    struct Policy { const char *name; QWebEngineSettings::UnknownUrlSchemePolicy policy; bool expectLaunch; };
    const QList<Policy> policies = {
        {"AllowAllUnknownUrlSchemes (control)", QWebEngineSettings::AllowAllUnknownUrlSchemes, true},
        {"DisallowUnknownUrlSchemes (product)", QWebEngineSettings::DisallowUnknownUrlSchemes, false},
    };
    QWebEngineProfile profile; // off-the-record, no application interceptor
    int failures = 0;
    for (const auto &policy : policies) {
        QMap<QString, int> launched;
        for (const auto &trigger : triggers) {
            QWebEnginePage page(&profile);
            page.settings()->setUnknownUrlSchemePolicy(policy.policy);
            QEventLoop load; QTimer timeout; timeout.setSingleShot(true);
            QObject::connect(&timeout, &QTimer::timeout, &load, &QEventLoop::quit);
            QObject::connect(&page, &QWebEnginePage::loadFinished, &load, &QEventLoop::quit);
            timeout.start(10000);
            page.load(QUrl(QString("http://127.0.0.1:%1/").arg(server.serverPort())));
            load.exec();
            counter.hits = 0; counter.urls.clear();
            page.runJavaScript(QString::fromLatin1(trigger.script));
            QEventLoop settle; QTimer::singleShot(1500, &settle, &QEventLoop::quit); settle.exec();
            launched[QString::fromLatin1(trigger.name)] = counter.hits;
        }
        QStringList reached;
        for (auto it = launched.cbegin(); it != launched.cend(); ++it) if (it.value() > 0) reached << it.key();
        qInfo().noquote() << "EXTERNAL_PROTOCOL_RESULT" << policy.name << "triggers that launched:" << (reached.isEmpty() ? QStringLiteral("none") : reached.join(", "));
        if (policy.expectLaunch && reached.isEmpty()) { qCritical() << "EXTERNAL_PROTOCOL_TEST FAIL: control produced no launch, so the harness cannot observe one"; ++failures; }
        if (!policy.expectLaunch && !reached.isEmpty()) { qCritical() << "EXTERNAL_PROTOCOL_TEST FAIL: product policy let a page launch an external handler via" << reached; ++failures; }
    }
    for (const char *scheme : {"cybou-ext-a", "cybou-ext-b"}) QDesktopServices::unsetUrlHandler(QString::fromLatin1(scheme));
    if (failures) return 1;
    qInfo() << "EXTERNAL_PROTOCOL_TEST PASS: control launches were observed; with DisallowUnknownUrlSchemes none of" << triggers.size() << "script triggers reached an external handler";
    return 0;
}
