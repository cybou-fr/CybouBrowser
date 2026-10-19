# Qt Positioning WinRT plugin: use-after-free when a position source is deleted (Qt 6.12.0, Windows)

Draft bug report for bugreports.qt.io (component: Qt Positioning). Not yet submitted.

## Summary

Deleting a `QGeoPositionInfoSource` created by the `winrt` plugin while updates are running can crash the
process. The plugin registers `PositionChanged` and `StatusChanged` handlers on the `Geolocator` with a raw
`this` and does not unregister them in the destructor. Windows delivers an already queued callback into the
main (STA) thread after the object is freed.

## Environment

- Qt 6.12.0, MSVC 2022 x64, Windows 11 (10.0.26300 and 10.0.26100 build lab).
- `QGuiApplication` (single-threaded apartment). A `QCoreApplication` did not reproduce it.

## Reproduction

`tools/geo-repro` (no other code): loop `createDefaultSource` / `setUpdateInterval(1000)` / `startUpdates()`,
spin the event loop 200-400 ms, `delete source` without calling `stopUpdates()`, 20 cycles per process.

| Plugin | stop then delete | plain delete |
|---|---|---|
| stock 6.12.0 | 0 of 30 processes crash | 27 of 30 |
| stock, earlier run | - | 12 of 40 (+1 hang) |
| with the patch below | 0 of 30 | 13 of 30 |

## Analysis

Under page heap (`!heap -p -enable -full`) the first access violation is, in the stock plugin:

- `QGeoPositionInfoSourceWinRT::onStatusChanged` writing `d->positionStatus` (`args->get_Status(&d->positionStatus)`)
  after the private object was freed, called from `RPCRT4!Invoke` / `combase!ReentrantSTAInvokeInApartment`
  inside the main thread's message loop;
- `QGeoPositionInfoSourceWinRT::nativePositionUpdate` emitted on the freed object (`QMetaObject::activate`, `doActivate`).

Cause: `startHandler()` registers both handlers with `Callback<...>(this, &...::onXxx)`, and `~QGeoPositionInfoSourceWinRT()`
only calls `CoUninitialize()`; `stopHandler()` (which removes both handlers) runs only from `stopUpdates()`.
`stopUpdates()` before delete is crash-free in all runs.

## Patch tried (insufficient)

`tools/qt-winrt-positioning-fix/0001-...patch`: handlers capture a shared guard object instead of `this`; the destructor
clears the owner, waits (bounded) for a running handler and calls `stopHandler()`. It removes both access violations above
under page heap, but the unmodified-timing repro still crashes in 13 of 30 processes. The remaining crashes have these
tops: `QCoreApplication::compressEvent` called from `qt_fast_timer_proc` (multimedia timer thread), `doActivate` /
`QObjectPrivate::maybeSignalConnected` from `setError` in `startUpdates`/`init`, and `Geolocation!...get_MonitoringScope`
called from `init`. They are not explained; coarse timers did not help (28 of 40). A proper fix probably needs the
source to be created and destroyed with an explicit, synchronous teardown of the WinRT objects.

## Workaround used by CYBOU

CYBOU replaced the plugin with its own (`src/geo/CybouPositionPlugin.cpp`): one process-wide `Geolocator` that is never
released, no event handlers, `GetGeopositionAsync` per request polled from the source's own event loop. 3 x 1000
create/use/delete cycles: no crash.

## Notes before submitting

- Modified Qt sources are LGPL: do not ship the patched plugin without complying; CYBOU does not ship it.
- Attach `tools/geo-repro` and the stock-plugin dump summaries (`tools/security/minidump_summary.py`).
