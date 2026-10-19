#pragma once
#include <QObject>
#include <QtPositioning/qgeopositioninfosourcefactory.h>

// Qt Positioning plugin that replaces the stock WinRT one (priority 2000 > 1000), so the Chromium location provider
// in QtWebEngine, which asks for the default source, gets this one. The stock plugin subscribes COM event handlers
// that capture the source's raw `this` and can still run after the source is deleted (docs/VALIDATION.md,
// "Geolocation plugin crash"). This one registers no WinRT event handler at all: it asks Windows for a position
// with GetGeopositionAsync and polls the returned operation from the owner's own event loop, so nothing can call
// back into a destroyed source.
class CybouPositionFactory : public QObject, public QGeoPositionInfoSourceFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.position.sourcefactory/6.0" FILE "cybou_position.json")
    Q_INTERFACES(QGeoPositionInfoSourceFactory)
public:
    QGeoPositionInfoSource *positionInfoSource(QObject *parent, const QVariantMap &parameters) override;
    QGeoSatelliteInfoSource *satelliteInfoSource(QObject *, const QVariantMap &) override { return nullptr; }
    QGeoAreaMonitorSource *areaMonitor(QObject *, const QVariantMap &) override { return nullptr; }
};
