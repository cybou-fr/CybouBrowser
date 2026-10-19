#pragma once
#include <QDebug>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QRandomGenerator>
#include <QTimer>
#include <QtPositioning/QGeoPositionInfoSource>

// Create / use / delete the default position source many times without stopping it first: the pattern that
// crashed the stock WinRT plugin in 27 of 30 processes (docs/VALIDATION.md). Passing means the process is still
// alive after every cycle and the source in use is CYBOU's own plugin.
//   --geolocation-test [cycles=1000] [maxSpinMs=120]
inline int runGeolocationStress()
{
    int cycles = 1000;
    const QStringList args = QCoreApplication::arguments();
    const int at = args.indexOf(QStringLiteral("--geolocation-test"));
    if (at >= 0 && at + 1 < args.size() && args.at(at + 1).toInt() > 0)
        cycles = args.at(at + 1).toInt();
    const int maxSpin = (at >= 0 && at + 2 < args.size() && args.at(at + 2).toInt() > 0) ? args.at(at + 2).toInt() : 120;
    int updates = 0, errors = 0, wrongSource = 0, noSource = 0;
    QElapsedTimer clock;
    clock.start();
    for (int i = 0; i < cycles; ++i) {
        auto *source = QGeoPositionInfoSource::createDefaultSource(nullptr);
        if (!source) { ++noSource; continue; }
        if (source->sourceName() != QLatin1String("cybou")) ++wrongSource;
        QObject::connect(source, &QGeoPositionInfoSource::positionUpdated, source, [&](const QGeoPositionInfo &) { ++updates; });
        QObject::connect(source, &QGeoPositionInfoSource::errorOccurred, source, [&](QGeoPositionInfoSource::Error) { ++errors; });
        if (i % 3 == 0) source->requestUpdate(2000); else { source->setUpdateInterval(1000); source->startUpdates(); }
        QEventLoop spin;
        QTimer::singleShot(QRandomGenerator::global()->bounded(maxSpin), &spin, &QEventLoop::quit);
        spin.exec();
        delete source; // request possibly still in flight
    }
    QEventLoop tail;
    QTimer::singleShot(1500, &tail, &QEventLoop::quit);
    tail.exec();
    const bool pass = noSource == 0 && wrongSource == 0;
    qInfo().noquote() << "GEOLOCATION_STRESS_TEST" << (pass ? "PASS" : "FAIL") << "cycles" << cycles << "updates" << updates << "errors" << errors
                      << "noSource" << noSource << "wrongSource" << wrongSource << "ms" << clock.elapsed();
    return pass ? 0 : 2;
}
