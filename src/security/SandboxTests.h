#pragma once
#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QTimer>
#include <QDebug>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include "EngineHardening.h"
#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#include <winternl.h>
#endif

// CYB-SBX-001: is the renderer really sandboxed? Reads what Windows holds for each QtWebEngineProcess.exe descendant:
// its --type, the integrity level of its token, membership of a job object, and whether its command line disables the
// sandbox. A second child started with QTWEBENGINE_DISABLE_SANDBOX=1 is the negative control: its renderer must show
// a different (medium) integrity, which proves the measurement can tell a sandboxed renderer from an unsandboxed one.

#ifdef Q_OS_WIN
namespace sandboxprobe {
inline QString integrityOf(HANDLE process) {
    HANDLE token = nullptr;
    if (!OpenProcessToken(process, TOKEN_QUERY, &token)) return QStringLiteral("unreadable");
    DWORD size = 0;
    GetTokenInformation(token, TokenIntegrityLevel, nullptr, 0, &size);
    QByteArray buffer(int(size), 0);
    QString level = QStringLiteral("unreadable");
    if (size && GetTokenInformation(token, TokenIntegrityLevel, buffer.data(), size, &size)) {
        const auto label = reinterpret_cast<TOKEN_MANDATORY_LABEL *>(buffer.data());
        const DWORD rid = *GetSidSubAuthority(label->Label.Sid, *GetSidSubAuthorityCount(label->Label.Sid) - 1);
        level = rid < SECURITY_MANDATORY_LOW_RID ? QStringLiteral("untrusted")
              : rid < SECURITY_MANDATORY_MEDIUM_RID ? QStringLiteral("low")
              : rid < SECURITY_MANDATORY_HIGH_RID ? QStringLiteral("medium") : QStringLiteral("high");
    }
    CloseHandle(token);
    return level;
}
inline QString commandLineOf(HANDLE process) {
    using Query = LONG (NTAPI *)(HANDLE, ULONG, PVOID, ULONG, PULONG);
    static const auto query = reinterpret_cast<Query>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess"));
    if (!query) return {};
    ULONG size = 0;
    query(process, 60 /* ProcessCommandLineInformation */, nullptr, 0, &size);
    if (!size) return {};
    QByteArray buffer(int(size), 0);
    if (query(process, 60, buffer.data(), size, &size) < 0) return {};
    const auto text = reinterpret_cast<UNICODE_STRING *>(buffer.data());
    return QString::fromWCharArray(text->Buffer, text->Length / sizeof(wchar_t));
}
// Every QtWebEngineProcess.exe below this process, with what Windows says about it.
inline QJsonArray engineProcesses() {
    QJsonArray result;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return result;
    QMultiHash<DWORD, PROCESSENTRY32W> children;
    PROCESSENTRY32W entry{sizeof(entry)};
    for (bool ok = Process32FirstW(snapshot, &entry); ok; ok = Process32NextW(snapshot, &entry)) children.insert(entry.th32ParentProcessID, entry);
    CloseHandle(snapshot);
    QList<DWORD> pending{GetCurrentProcessId()};
    while (!pending.isEmpty()) {
        const DWORD parent = pending.takeFirst();
        for (const auto &child : children.values(parent)) {
            pending.append(child.th32ProcessID);
            const QString exe = QString::fromWCharArray(child.szExeFile);
            if (exe.compare(QStringLiteral("QtWebEngineProcess.exe"), Qt::CaseInsensitive) != 0) continue;
            HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, child.th32ProcessID);
            if (!process) { result.append(QJsonObject{{"pid", int(child.th32ProcessID)}, {"type", "unopenable"}}); continue; }
            const QString commandLine = commandLineOf(process);
            const auto type = QRegularExpression(QStringLiteral("--type=([\\w-]+)")).match(commandLine);
            BOOL inJob = FALSE;
            IsProcessInJob(process, nullptr, &inJob);
            result.append(QJsonObject{{"pid", int(child.th32ProcessID)},
                                      {"type", type.hasMatch() ? type.captured(1) : QStringLiteral("unknown")},
                                      {"integrity", integrityOf(process)}, {"job", bool(inJob)},
                                      {"noSandboxFlag", commandLine.contains(QStringLiteral("--no-sandbox"))}});
            CloseHandle(process);
        }
    }
    return result;
}
}
#endif

// Child: loads a page so a renderer exists, then reports the engine processes as one JSON line.
inline int runSandboxChild() {
    qInfo().noquote() << QString::fromLatin1(engineConfigLine());
    QWebEngineProfile profile;
    QWebEnginePage page(&profile);
    QEventLoop loop; QTimer timeout; timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, [&](bool) { QTimer::singleShot(1500, &loop, &QEventLoop::quit); });
    timeout.start(60000);
    page.setHtml(QStringLiteral("<!doctype html><title>sandbox</title><script>document.title='rendered'</script>"), QUrl("https://cybou.test/"));
    loop.exec();
#ifdef Q_OS_WIN
    qInfo().noquote() << "SANDBOX_PROCESSES" << QJsonDocument(sandboxprobe::engineProcesses()).toJson(QJsonDocument::Compact);
#endif
    return 0;
}

// Parent: product child must have every renderer sandboxed; the control child (sandbox disabled) must not.
inline int runSandboxTest() {
#ifndef Q_OS_WIN
    qCritical() << "SANDBOX_TEST BLOCKED: process inspection is implemented for Windows only";
    return 1;
#else
    QJsonArray seen[2];
    for (int control : {0, 1}) {
        QProcess child;
        child.setProcessChannelMode(QProcess::MergedChannels);
        auto env = QProcessEnvironment::systemEnvironment();
        if (control) env.insert(QStringLiteral("QTWEBENGINE_DISABLE_SANDBOX"), QStringLiteral("1"));
        else env.remove(QStringLiteral("QTWEBENGINE_DISABLE_SANDBOX"));
        child.setProcessEnvironment(env);
        child.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--sandbox-child")});
        if (!child.waitForFinished(120000)) { child.kill(); child.waitForFinished(5000); qCritical() << "SANDBOX_TEST FAIL: child timeout" << control; return 1; }
        const QString output = QString::fromUtf8(child.readAll());
        const auto line = QRegularExpression(QStringLiteral("SANDBOX_PROCESSES (\\[.*\\])")).match(output);
        if (child.exitCode() != 0 || !line.hasMatch()) { qCritical().noquote() << "SANDBOX_TEST FAIL: child" << control << "output tail:" << output.right(600); return 1; }
        if (!control && !output.contains(QStringLiteral("unsafe=none"))) { qCritical().noquote() << "SANDBOX_TEST FAIL: product child ran with unsafe engine settings" << output.left(300); return 1; }
        seen[control] = QJsonDocument::fromJson(line.captured(1).toUtf8()).array();
        qInfo().noquote() << "SANDBOX_CHILD" << (control ? "control(sandbox disabled)" : "product") << line.captured(1);
    }
    auto renderers = [](const QJsonArray &all) { QJsonArray out; for (const auto &p : all) if (p.toObject()["type"] == "renderer") out.append(p); return out; };
    const auto product = renderers(seen[0]), control = renderers(seen[1]);
    if (product.isEmpty() || control.isEmpty()) { qCritical() << "SANDBOX_TEST FAIL: no renderer process found" << product.size() << control.size(); return 1; }
    for (const auto &value : product) {
        const auto p = value.toObject();
        const QString level = p["integrity"].toString();
        if ((level != "untrusted" && level != "low") || !p["job"].toBool() || p["noSandboxFlag"].toBool()) {
            qCritical() << "SANDBOX_TEST FAIL: product renderer not sandboxed" << p; return 1;
        }
    }
    bool controlDiffers = false;
    for (const auto &value : control) {
        const QString level = value.toObject()["integrity"].toString();
        if (level == "medium" || level == "high") controlDiffers = true;
    }
    if (!controlDiffers) { qCritical() << "SANDBOX_TEST FAIL: control renderer looks sandboxed too, the measurement proves nothing" << control; return 1; }
    qInfo() << "SANDBOX_TEST PASS: every product renderer runs at" << product[0].toObject()["integrity"].toString()
            << "integrity inside a job object without --no-sandbox; with the sandbox disabled the renderer runs at"
            << control[0].toObject()["integrity"].toString();
    return 0;
#endif
}
