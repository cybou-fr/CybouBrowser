#pragma once
#include <QCoreApplication>
#include <QEventLoop>
#include <QProcess>
#include <QRegularExpression>
#include <QTimer>
#include <QDebug>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include "EngineHardening.h"

// CVE-2026-87491 stopgap: the WebAssembly-off switch (kDisableWasmSwitch).
// Pure flag-composition checks first, then the engine itself in two child processes because the V8 flag is process-wide.
inline bool runDisableWasmFlagTests() {
    const QByteArray wasm = kDisableWasmSwitch;
    if (!kDisableWasmDefault) { qCritical() << "WASM FAIL: stopgap not on by default"; return false; }
    if (engineHardeningFlags("").contains("--js-flags")) { qCritical() << "WASM FAIL: switch present while disabled"; return false; }
    const auto on = engineHardeningFlags("", true, true);
    if (on.count(wasm) != 1 || !on.contains(kStrictWebRtcSwitch) || !on.contains("BlockInsecurePrivateNetworkRequests")) {
        qCritical() << "WASM FAIL: enabled flags" << on; return false;
    }
    if (engineHardeningFlags(on, true, true).count("--js-flags") != 1) { qCritical() << "WASM FAIL: not idempotent"; return false; }
    const auto explicitFlags = engineHardeningFlags("--js-flags=--max-lazy", false, true);
    if (explicitFlags.contains("jitless") || !explicitFlags.contains("--js-flags=--max-lazy")) {
        qCritical() << "WASM FAIL: explicit --js-flags overridden" << explicitFlags; return false;
    }
    qInfo() << "WASM_FLAGS PASS: on by default, single switch when enabled, idempotent, respects explicit --js-flags";
    return true;
}

// Child: reports what a page sees as the WebAssembly global under the engine flags of this process.
inline int runDisableWasmChild() {
    QWebEngineProfile profile; // off-the-record
    QWebEnginePage page(&profile);
    QString seen = "timeout";
    QEventLoop loop; QTimer timeout; timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, [&](bool ok) {
        if (!ok) { seen = "load-failed"; loop.quit(); return; }
        page.runJavaScript(QStringLiteral("typeof WebAssembly + ':' + (()=>{try{return String(new WebAssembly.Module(new Uint8Array([0,97,115,109,1,0,0,0])) instanceof Object)}catch(e){return 'error'}})()"),
                           [&](const QVariant &v) { seen = v.toString(); loop.quit(); });
    });
    timeout.start(60000);
    page.setHtml(QStringLiteral("<!doctype html><title>wasm</title>"), QUrl("https://cybou.test/"));
    loop.exec();
    qInfo().noquote() << "wasmGlobal" << seen;
    return 0;
}

// Parent: control child (switch off) must compile a minimal module; stopgap child must see no WebAssembly at all.
inline int runDisableWasmTest() {
    if (!runDisableWasmFlagTests()) return 1;
    QString seen[2];
    for (int disabled : {0, 1}) {
        QProcess child;
        child.setProcessChannelMode(QProcess::MergedChannels);
        child.start(QCoreApplication::applicationFilePath(), {QString("--wasm-child=%1").arg(disabled)});
        if (!child.waitForFinished(120000)) { child.kill(); child.waitForFinished(5000); qCritical() << "WASM FAIL: child timeout" << disabled; return 1; }
        const QString output = QString::fromUtf8(child.readAll());
        const auto match = QRegularExpression("wasmGlobal (\\S+)").match(output);
        if (child.exitStatus() != QProcess::NormalExit || child.exitCode() != 0 || !match.hasMatch()) {
            qCritical().noquote() << "WASM FAIL: child" << disabled << "exit" << child.exitCode() << "output tail:" << output.right(600);
            return 1;
        }
        seen[disabled] = match.captured(1);
        qInfo() << "WASM_CHILD" << (disabled ? "disabled" : "control") << seen[disabled];
    }
    if (seen[0] != "object:true") { qCritical() << "WASM FAIL: control cannot compile WebAssembly, the test proves nothing" << seen[0]; return 1; }
    if (!seen[1].startsWith("undefined")) { qCritical() << "WASM FAIL: WebAssembly still exposed with the stopgap on" << seen[1]; return 1; }
    qInfo() << "WASM_TEST PASS: control compiles a module; with the stopgap the WebAssembly global is absent";
    return 0;
}
