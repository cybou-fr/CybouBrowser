#pragma once
#include <random>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QMap>
#include <QRegularExpression>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include "DownloadPolicy.h"
#include "NavigationGuard.h"
#include "NetworkPolicy.h"
#include "StoredUrl.h"

// Deterministic (fixed seed) fuzzing of the URL-handling code that sees attacker-controlled strings.
//  A. Robustness and properties on generated and adversarial strings: no crash, each call bounded in
//     time (ReDoS / quadratic behaviour), StoredUrl::sanitize idempotent, secret canaries removed,
//     trailing-dot and case invariance of the host classification.
//  B. Differential against the engine: random spellings of the loopback address (mixed radix, 1-4
//     parts, case, trailing dot) - whatever the engine actually connects to must be classified private.
// Not coverage-guided and not under a sanitizer; it is a floor, not the sanitizer/fuzzer work ASVS 1.4 asks for.
namespace FuzzDetail {

inline QString pick(std::mt19937 &rng, const QStringList &items) { return items[rng() % items.size()]; }

inline QString numeral(std::mt19937 &rng, quint32 value, bool allowOctal = true) {
    switch (rng() % (allowOctal ? 4 : 3)) {
    case 0: return QString::number(value);
    case 1: return "0x" + QString::number(value, 16).rightJustified(1 + rng() % 3, '0');
    case 2: return "0X" + QString::number(value, 16).toUpper();
    default: return value == 0 ? QStringLiteral("0") : "0" + QString::number(value, 8).rightJustified(1 + rng() % 2, '0');
    }
}

// A random valid spelling of 127.0.0.1 (inet_aton style: a.b.c.d, a.b.c, a.b, a).
inline QString loopbackSpelling(std::mt19937 &rng) {
    const quint32 o[4] = {127, 0, 0, 1};
    QStringList parts;
    switch (rng() % 4) {
    case 0: for (quint32 v : o) parts << numeral(rng, v); break;
    case 1: parts << numeral(rng, o[0]) << numeral(rng, o[1]) << numeral(rng, (o[2] << 8) | o[3]); break;
    case 2: parts << numeral(rng, o[0]) << numeral(rng, (o[1] << 16) | (o[2] << 8) | o[3]); break;
    default: parts << numeral(rng, (o[0] << 24) | (o[1] << 16) | (o[2] << 8) | o[3]); break;
    }
    return parts.join('.') + (rng() % 4 == 0 ? "." : "");
}

inline QString randomChunk(std::mt19937 &rng, int maxLength) {
    static const QString alphabet = QStringLiteral("abcXYZ019%-._~:/?#[]@!$&'()*+,;= \t\\é中。１‮");
    QString out;
    const int length = rng() % (maxLength + 1);
    for (int i = 0; i < length; ++i) out += alphabet[rng() % alphabet.size()];
    return out;
}

inline QString randomUrl(std::mt19937 &rng, int index) {
    const QStringList schemes{"http", "https", "ws", "wss", "ftp", "file", "javascript", "data", "blob", "about", "chrome", ""};
    const QStringList hosts{"example.com", "localhost", "localhost.", "127.0.0.1", "10.0.0.5", "192.168.1.1", "169.254.169.254", "[::1]", "[::ffff:127.0.0.1]",
                            "router.local", "a.b.c.d.e.f.g.h", "xn--bcher-kva.example", "2130706433", "0x7f.1", "8.8.8.8", ""};
    QString host = rng() % 5 == 0 ? randomChunk(rng, 40) : (rng() % 4 == 0 ? loopbackSpelling(rng) : pick(rng, hosts));
    QString url = pick(rng, schemes) + (rng() % 8 ? "://" : ":");
    if (rng() % 5 == 0) url += randomChunk(rng, 12) + (rng() % 2 ? ":" + randomChunk(rng, 6) : "") + "@";
    url += host;
    if (rng() % 3 == 0) url += ":" + (rng() % 3 ? QString::number(rng() % 70000) : randomChunk(rng, 8));
    url += "/" + randomChunk(rng, 30);
    if (rng() % 2) {
        // Well-formed boundaries around the secret so the canary property is about the parser, not about names made of garbage.
        QString lead = randomChunk(rng, 20); lead.remove(QRegularExpression("[&;=#?]"));
        const QString secret = pick(rng, {"access_token=CANARY%1", "token=CANARY%1", "code=CANARY%1", "password=CANARY%1", "sig=CANARY%1", "X-Amz-Signature=CANARY%1", "api_key=CANARY%1"}).replace("%1", QString::number(index));
        url += "?" + (lead.isEmpty() ? QString() : lead + (rng() % 2 ? "&" : ";")) + secret + (rng() % 3 == 0 ? (rng() % 2 ? "&" : ";") + QString("k=v") : QString());
    }
    if (rng() % 3 == 0) url += "#" + pick(rng, {"access_token=CANARY%1", "id_token=CANARY%1", "frag"}).replace("%1", QString::number(index));
    return url;
}

} // namespace FuzzDetail

inline int runFuzzTests() {
    using namespace FuzzDetail;
    std::mt19937 rng(0xC1B0u);
    int failures = 0, canaryInvalidUrls = 0, slowest = 0;
    QString slowestInput;
    auto fail = [&](const QString &why, const QString &input) { if (failures++ < 12) qCritical().noquote() << "FUZZ FAIL:" << why << "input" << input.left(160); };

    // ---- A. properties on generated URLs
    const int iterations = 20000;
    for (int i = 0; i < iterations; ++i) {
        const QString input = randomUrl(rng, i);
        QElapsedTimer timer; timer.start();
        const QString cleaned = StoredUrl::sanitize(input);
        const QUrl url(input, QUrl::TolerantMode);
        const bool priv = NavigationGuard::isPrivateNetwork(url);
        NavigationGuard::blocksPrivateNetworkAccess(url, QUrl("https://example.org/"), false);
        NavigationGuard::blocksPrivateNetworkAccess(url, QUrl("https://example.org/"), true);
        NavigationGuard::hasSuspiciousIdn(url);
        NavigationGuard::isLocalHost(url);
        NetworkPolicy::blocks(url, true);
        NetworkPolicy::hasWebCredentials(url);
        DownloadPolicy::allowsExternalOpen(input);
        const int ms = int(timer.elapsed());
        if (ms > slowest) { slowest = ms; slowestInput = input; }
        if (ms > 100) fail(QString("slow: %1 ms").arg(ms), input);
        if (StoredUrl::sanitize(cleaned) != cleaned) fail("sanitize not idempotent", input);
        if (cleaned.contains("CANARY") && input.contains("CANARY") && (url.isValid() ? !url.scheme().isEmpty() : true)) fail("secret canary survived sanitize", input);
        if (input.contains("CANARY") && !url.isValid()) ++canaryInvalidUrls;
        // host classification must not depend on case or on a trailing dot
        const QString host = url.host();
        if (url.isValid() && !host.isEmpty() && !host.contains(':') && !host.endsWith('.')) {
            QUrl dotted = url, upper = url;
            dotted.setHost(host + "."); upper.setHost(host.toUpper());
            if (priv != NavigationGuard::isPrivateNetwork(dotted)) fail("classification changes with a trailing dot", input);
            if (priv != NavigationGuard::isPrivateNetwork(upper)) fail("classification changes with host case", input);
        }
    }
    // ---- adversarial sizes: time bounds for pathological inputs
    const QStringList heavy{
        "http://" + QString("a.").repeated(50000) + "com/", "http://" + QString("0x").repeated(50000) + "/", "http://" + QString(200000, QChar('9')) + "/",
        "https://example.org/?" + QString("token=x&").repeated(50000), "https://example.org/?" + QString("%").repeated(200000),
        "http://[" + QString(":").repeated(50000) + "]/", "http://" + QString("。").repeated(50000) + "/", "http://example.org/" + QString("../").repeated(100000)};
    for (const auto &input : heavy) {
        QElapsedTimer timer; timer.start();
        StoredUrl::sanitize(input);
        const QUrl url(input, QUrl::TolerantMode);
        NavigationGuard::isPrivateNetwork(url); NavigationGuard::blocksPrivateNetworkAccess(url, QUrl("https://example.org/"), false);
        NavigationGuard::hasSuspiciousIdn(url); NetworkPolicy::blocks(url, true); DownloadPolicy::allowsExternalOpen(input);
        if (timer.elapsed() > 2000) fail(QString("pathological input took %1 ms").arg(timer.elapsed()), input.left(60));
    }
    qInfo() << "FUZZ_PROPERTIES" << iterations << "generated URLs," << heavy.size() << "pathological inputs; slowest generated call" << slowest << "ms; failures so far" << failures << "; canary-bearing inputs QUrl rejects (cut at ? or #):" << canaryInvalidUrls;

    // ---- B. differential with the engine
    QMap<QByteArray, int> hits;
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost)) return 2;
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&] {
        while (auto socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [&hits, socket] {
                const auto request = socket->property("request").toByteArray() + socket->readAll();
                socket->setProperty("request", request);
                if (!request.contains("\r\n\r\n") || socket->property("handled").toBool()) return;
                socket->setProperty("handled", true);
                const auto parts = request.left(request.indexOf("\r\n")).split(' ');
                if (parts.size() > 1) ++hits[parts[1]];
                const QByteArray body("<!doctype html><title>f</title>");
                socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });
    QWebEngineProfile profile;
    QWebEnginePage page(&profile);
    const int samples = 250;
    int reached = 0, missedByGuard = 0;
    for (int i = 0; i < samples; ++i) {
        const QString spelling = loopbackSpelling(rng);
        const QString path = QString("/f%1").arg(i);
        const QString spelled = QString("http://%1:%2%3").arg(spelling).arg(server.serverPort()).arg(path);
        QEventLoop loop; QTimer timeout; timeout.setSingleShot(true);
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        auto connection = QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, &QEventLoop::quit);
        timeout.start(4000);
        page.load(QUrl(spelled));
        loop.exec();
        QObject::disconnect(connection);
        if (hits.value(path.toUtf8()) > 0) {
            ++reached;
            if (!NavigationGuard::isPrivateNetwork(QUrl(spelled))) { ++missedByGuard; fail("engine reaches loopback but guard says public", spelled); }
        }
    }
    qInfo() << "FUZZ_DIFFERENTIAL" << samples << "random loopback spellings; engine reached the fixture for" << reached << "; classified public despite reaching loopback:" << missedByGuard;
    if (reached < samples / 4) fail("differential is vacuous: too few spellings reached the fixture", QString::number(reached));
    if (failures) { qCritical() << "FUZZ_TEST FAIL:" << failures << "failure(s)"; return 1; }
    qInfo() << "FUZZ_TEST PASS:" << iterations << "generated URLs and" << samples << "engine-checked spellings; no crash, bounded time, idempotent sanitising, no surviving canary, consistent classification";
    return 0;
}
