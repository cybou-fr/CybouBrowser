// Minimal reproduction for the Qt Positioning WinRT plugin lifetime defect: create the default position
// source, start updates, destroy it (optionally without stopping first), repeat. A crash kills the process,
// so run.sh runs many short processes and counts abnormal exits. No CYBOU code is involved.
//   geo-repro [cycles=50] [mode=delete|stop-then-delete] [spinMs=15]
#include <QGuiApplication>
#include <QDebug>
#include <cstdio>
#include <string>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>
#include <QRandomGenerator>
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
// Writes geo_repro_<pid>.dmp next to the executable on an unhandled exception so the crash site can be read.
static LONG WINAPI writeDump(EXCEPTION_POINTERS *info) {
    fprintf(stderr, "GEO_REPRO_EXCEPTION code=0x%08lX address=%p thread=%lu\n", info->ExceptionRecord->ExceptionCode, info->ExceptionRecord->ExceptionAddress, GetCurrentThreadId());
    fflush(stderr);
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring file = path;
    file = file.substr(0, file.find_last_of(L"\\/") + 1) + L"geo_repro_" + std::to_wstring(GetCurrentProcessId()) + L".dmp";
    HANDLE h = CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei{GetCurrentThreadId(), info, FALSE};
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), h, MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo), &mei, nullptr, nullptr);
        CloseHandle(h);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif
#include <QTimer>

int main(int argc, char *argv[]) {
    // QGuiApplication on the windows platform initialises OLE as a single-threaded apartment, like the browser;
    // QCoreApplication did not reproduce the crash.
#ifdef _WIN32
    SetUnhandledExceptionFilter(writeDump);
    // COM/RPC dispatch swallows exceptions raised inside a callback and ends the process itself, so the
    // unhandled-exception filter may never run: also look at access violations as they are raised.
    AddVectoredExceptionHandler(1, [](EXCEPTION_POINTERS *info) -> LONG {
        static volatile LONG written = 0;
        if (info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && InterlockedExchange(&written, 1) == 0)
            writeDump(info);
        return EXCEPTION_CONTINUE_SEARCH;
    });
#endif
    QGuiApplication app(argc, argv);
    const int cycles = argc > 1 ? QString(argv[1]).toInt() : 50;
    const QString mode = argc > 2 ? QString(argv[2]) : QStringLiteral("delete");
    const int spinMs = argc > 3 ? QString(argv[3]).toInt() : 300;
    int created = 0, noSource = 0, updates = 0, errors = 0;
    for (int i = 0; i < cycles; ++i) {
        auto *source = QGeoPositionInfoSource::createDefaultSource(nullptr);
        if (!source) { ++noSource; continue; }
        ++created;
        QObject::connect(source, &QGeoPositionInfoSource::positionUpdated, &app, [&](const QGeoPositionInfo &) { ++updates; });
        QObject::connect(source, &QGeoPositionInfoSource::errorOccurred, &app, [&](QGeoPositionInfoSource::Error) { ++errors; });
        source->setUpdateInterval(1000);
        source->startUpdates();
        QEventLoop spin; QTimer::singleShot(spinMs + QRandomGenerator::global()->bounded(spinMs + 1), &spin, &QEventLoop::quit); spin.exec();
        if (mode == QLatin1String("stop-then-delete")) source->stopUpdates();
        delete source; // WinRT callbacks may still be in flight on their own thread here
    }
    // Let late callbacks arrive after the last destruction.
    QEventLoop tail; QTimer::singleShot(400, &tail, &QEventLoop::quit); tail.exec();
    qInfo().noquote() << "GEO_REPRO_DONE" << "mode" << mode << "cycles" << cycles << "created" << created << "noSource" << noSource << "updates" << updates << "errors" << errors;
    return 0;
}
