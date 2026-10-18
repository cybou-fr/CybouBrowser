#pragma once
#include <memory>
#include <QEventLoop>
#include <QFile>
#include <QTimer>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include <QtWebEngineCore/QWebEngineProfileBuilder>
#include "HttpsPnaProbe.h" // ProbeTlsServer: TLS fixture with per-request hit counters

// ASVS 3.4.1 browser analogue: the engine must remember Strict-Transport-Security and upgrade a
// later http:// navigation to the same host. The fixture is TLS-only, so an un-upgraded http://
// request cannot succeed and never reaches the request counters. Engine decision only (no app
// interceptor); resolver rules map cybou-source.test to loopback in tests/test_main.cpp.
inline int runHstsTest() {
#if QT_VERSION < QT_VERSION_CHECK(6, 10, 0)
    qCritical() << "HSTS_TEST BLOCKED: profile-local CA requires Qt 6.10+"; return 2;
#else
    QFile rootFile("tests/security/fixtures/tls/test-ca-cert.pem"), leafFile("tests/security/fixtures/tls/pna-cert.pem"), keyFile("tests/security/fixtures/tls/localhost-key.pem");
    if (!rootFile.open(QIODevice::ReadOnly) || !leafFile.open(QIODevice::ReadOnly) || !keyFile.open(QIODevice::ReadOnly)) { qCritical() << "HSTS_TEST FAIL: fixtures (run from repo root)"; return 2; }
    const QSslCertificate root(rootFile.readAll()), leaf(leafFile.readAll());
    const QSslKey key(keyFile.readAll(), QSsl::Rsa);
    if (root.isNull() || leaf.isNull() || key.isNull()) return 2;
    QMap<QByteArray, int> hits;
    ProbeTlsServer server;
    server.certificate = leaf; server.key = key; server.hits = &hits;
    if (!server.listen(QHostAddress::LocalHost)) return 2;
    const quint16 port = server.serverPort();
    const auto count = [&](const char *path) { return hits.value(QByteArray::number(port) + " GET " + path); };

    struct Result { bool loaded = false; QString url; };
    // One fresh off-the-record profile per scenario so HSTS state never leaks between scenarios.
    auto visit = [&](QWebEnginePage &page, const QString &url) {
        Result result; QEventLoop loop; QTimer timeout; timeout.setSingleShot(true);
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        auto connection = QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, [&](bool ok) { result.loaded = ok; loop.quit(); });
        timeout.start(15000); page.load(QUrl(url)); loop.exec();
        QObject::disconnect(connection);
        result.url = page.url().toString();
        return result;
    };
    auto makeProfile = [&] {
        QWebEngineProfileBuilder builder; builder.setAdditionalTrustedCertificates({root});
        return std::unique_ptr<QWebEngineProfile>(builder.createProfile(QString()));
    };
    const QString https = QString("https://cybou-source.test:%1").arg(port), http = QString("http://cybou-source.test:%1").arg(port);
    auto fail = [](const char *what) { qCritical() << "HSTS_TEST FAIL:" << what; return 1; };

    // 1. Header present: first HTTPS visit records HSTS, later http:// is upgraded to https.
    server.extraHeaders = "Strict-Transport-Security: max-age=3600\r\n";
    {
        auto profile = makeProfile(); QWebEnginePage page(profile.get());
        QObject::connect(&page, &QWebEnginePage::certificateError, &page, [](QWebEngineCertificateError e) { e.rejectCertificate(); });
        if (!visit(page, https + "/a").loaded || count("/a") != 1) return fail("HTTPS visit with STS header");
        const auto upgraded = visit(page, http + "/b");
        if (!upgraded.loaded || count("/b") != 1 || !upgraded.url.startsWith("https://")) { qCritical() << upgraded.loaded << upgraded.url << count("/b"); return fail("http:// was not upgraded after STS"); }
        qInfo() << "HSTS_CASE PASS: http:// upgraded to https after Strict-Transport-Security, final URL" << upgraded.url;
    }
    // 2. Control: a fresh profile has no HSTS memory, so plain http:// to a TLS-only port fails.
    {
        auto profile = makeProfile(); QWebEnginePage page(profile.get());
        const auto plain = visit(page, http + "/c");
        if (plain.loaded || count("/c") != 0) return fail("control: fresh profile upgraded or reached the server");
        qInfo() << "HSTS_CASE PASS: control - no HSTS memory in a fresh profile, plain http:// does not reach the TLS-only server";
    }
    // 3. Control: the same HTTPS visit without the header does not create HSTS state.
    server.extraHeaders.clear();
    {
        auto profile = makeProfile(); QWebEnginePage page(profile.get());
        QObject::connect(&page, &QWebEnginePage::certificateError, &page, [](QWebEngineCertificateError e) { e.rejectCertificate(); });
        if (!visit(page, https + "/d").loaded || count("/d") != 1) return fail("HTTPS visit without header");
        const auto plain = visit(page, http + "/e");
        if (plain.loaded || count("/e") != 0) return fail("control: upgrade happened without an STS header");
        qInfo() << "HSTS_CASE PASS: control - HTTPS visit without the header creates no HSTS state";
    }
    qInfo() << "HSTS_TEST PASS: engine honours STS for same-host http:// within a profile; controls show it is the header, per profile";
    return 0;
#endif
}
