#include "CybouPositionPlugin.h"

#include <QtCore/QDateTime>
#include <QtCore/QTimer>
#include <QtCore/QTimeZone>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfo>
#include <QtPositioning/QGeoPositionInfoSource>

#include <windows.h>
#include <wrl.h>
#include <wrl/wrappers/corewrappers.h>
#include <windows.devices.geolocation.h>
#include <windows.foundation.h>
#include <cmath>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Devices::Geolocation;
using namespace ABI::Windows::Foundation;

namespace {

using PositionOperation = IAsyncOperation<Geoposition *>;

// One Geolocator for the whole process. It is created on first use and deliberately never released: no source
// owns it, so deleting any number of sources cannot tear it down underneath Windows.
IGeolocator *sharedLocator()
{
    static IGeolocator *locator = [] {
        IGeolocator *l = nullptr;
        if (FAILED(RoActivateInstance(HStringReference(RuntimeClass_Windows_Devices_Geolocation_Geolocator).Get(),
                                      reinterpret_cast<IInspectable **>(&l))))
            return static_cast<IGeolocator *>(nullptr);
        return l;
    }();
    return locator;
}

bool readOptional(IReference<double> *ref, double *value)
{
    if (!ref)
        return false;
    const bool ok = SUCCEEDED(ref->get_Value(value));
    ref->Release();
    return ok;
}

bool readPosition(IGeoposition *position, QGeoPositionInfo *out)
{
    ComPtr<IGeocoordinate> coord;
    if (FAILED(position->get_Coordinate(&coord)))
        return false;
    ComPtr<IGeocoordinateWithPoint> withPoint;
    if (FAILED(coord.As(&withPoint)))
        return false;
    ComPtr<IGeopoint> point;
    if (FAILED(withPoint->get_Point(&point)))
        return false;
    BasicGeoposition pos;
    if (FAILED(point->get_Position(&pos)))
        return false;

    bool geoidAltitude = false;
    ComPtr<IGeoshape> shape;
    if (SUCCEEDED(point.As(&shape)) && shape) {
        AltitudeReferenceSystem system;
        geoidAltitude = SUCCEEDED(shape->get_AltitudeReferenceSystem(&system)) && system == AltitudeReferenceSystem_Geoid;
    }
    out->setCoordinate(geoidAltitude ? QGeoCoordinate(pos.Latitude, pos.Longitude, pos.Altitude)
                                     : QGeoCoordinate(pos.Latitude, pos.Longitude));

    DOUBLE accuracy;
    if (SUCCEEDED(coord->get_Accuracy(&accuracy)))
        out->setAttribute(QGeoPositionInfo::HorizontalAccuracy, accuracy);

    double value = 0;
    IReference<double> *ref = nullptr;
    if (SUCCEEDED(coord->get_AltitudeAccuracy(&ref)) && readOptional(ref, &value))
        out->setAttribute(QGeoPositionInfo::VerticalAccuracy, value);
    ref = nullptr;
    if (SUCCEEDED(coord->get_Speed(&ref)) && readOptional(ref, &value) && std::isfinite(value))
        out->setAttribute(QGeoPositionInfo::GroundSpeed, value);
    ref = nullptr;
    if (SUCCEEDED(coord->get_Heading(&ref)) && readOptional(ref, &value) && std::isfinite(value) && value >= 0 && value < 360)
        out->setAttribute(QGeoPositionInfo::Direction, value);

    DateTime stamp;
    if (SUCCEEDED(coord->get_Timestamp(&stamp)) && stamp.UniversalTime > 0) {
        // 100 ns ticks since 1601-01-01 UTC.
        const qint64 msSince1601 = stamp.UniversalTime / 10000;
        out->setTimestamp(QDateTime::fromMSecsSinceEpoch(msSince1601 - 11644473600000LL, QTimeZone::UTC));
    } else {
        out->setTimestamp(QDateTime::currentDateTimeUtc());
    }
    return true;
}

class CybouPositionSource : public QGeoPositionInfoSource
{
public:
    explicit CybouPositionSource(QObject *parent) : QGeoPositionInfoSource(parent)
    {
        poll_.setInterval(50);
        connect(&poll_, &QTimer::timeout, this, [this] { pollOperation(); });
        next_.setSingleShot(true);
        connect(&next_, &QTimer::timeout, this, [this] { requestPosition(); });
        timeout_.setSingleShot(true);
        connect(&timeout_, &QTimer::timeout, this, [this] { fail(UpdateTimeoutError); });
        QGeoPositionInfoSource::setUpdateInterval(minimumUpdateInterval());
    }
    ~CybouPositionSource() override
    {
        // Nothing is registered with Windows that could call back: dropping our reference to the operation is enough.
        poll_.stop();
        next_.stop();
        timeout_.stop();
        pending_.Reset();
    }

    QGeoPositionInfo lastKnownPosition(bool) const override { return last_; }
    PositioningMethods supportedPositioningMethods() const override { return AllPositioningMethods; }
    int minimumUpdateInterval() const override { return 1000; }
    Error error() const override { return error_; }

    void setUpdateInterval(int msec) override
    {
        QGeoPositionInfoSource::setUpdateInterval(msec < minimumUpdateInterval() ? minimumUpdateInterval() : msec);
    }

    void startUpdates() override
    {
        error_ = NoError;
        updating_ = true;
        requestPosition();
    }
    void stopUpdates() override
    {
        updating_ = false;
        next_.stop();
    }
    void requestUpdate(int timeout) override
    {
        if (timeout != 0 && timeout < minimumUpdateInterval()) {
            fail(UpdateTimeoutError);
            return;
        }
        error_ = NoError;
        single_ = true;
        timeout_.start(timeout == 0 ? 2 * 60 * 1000 : timeout);
        requestPosition();
    }

private:
    void fail(Error e)
    {
        error_ = e;
        single_ = false;
        timeout_.stop();
        emit errorOccurred(e);
    }

    void requestPosition()
    {
        if (pending_ || (!updating_ && !single_))
            return;
        IGeolocator *locator = sharedLocator();
        if (!locator || FAILED(locator->GetGeopositionAsync(&pending_))) {
            pending_.Reset();
            updating_ = false;
            fail(UnknownSourceError);
            return;
        }
        poll_.start();
    }

    void pollOperation()
    {
        if (!pending_) {
            poll_.stop();
            return;
        }
        ComPtr<IAsyncInfo> info;
        AsyncStatus status = AsyncStatus::Started;
        if (FAILED(pending_.As(&info)) || FAILED(info->get_Status(&status)))
            status = AsyncStatus::Error;
        if (status == AsyncStatus::Started)
            return;
        poll_.stop();

        ComPtr<IGeoposition> position;
        HRESULT hr = status == AsyncStatus::Completed ? pending_->GetResults(&position) : E_FAIL;
        HRESULT errorCode = S_OK;
        if (status != AsyncStatus::Completed && info)
            info->get_ErrorCode(&errorCode);
        if (info)
            info->Close();
        pending_.Reset();

        QGeoPositionInfo found;
        if (SUCCEEDED(hr) && position && readPosition(position.Get(), &found)) {
            last_ = found;
            const bool wanted = single_ || updating_;
            single_ = false;
            timeout_.stop();
            if (wanted)
                emit positionUpdated(found);
            if (updating_)
                next_.start(updateInterval() > 0 ? updateInterval() : minimumUpdateInterval());
        } else {
            updating_ = false; // repeating a refused request every second would only repeat the refusal
            fail(errorCode == E_ACCESSDENIED ? AccessError : UnknownSourceError);
        }
    }

    QTimer poll_, next_, timeout_;
    ComPtr<PositionOperation> pending_;
    QGeoPositionInfo last_;
    Error error_ = NoError;
    bool updating_ = false;
    bool single_ = false;
};

} // namespace

QGeoPositionInfoSource *CybouPositionFactory::positionInfoSource(QObject *parent, const QVariantMap &)
{
    return new CybouPositionSource(parent);
}
