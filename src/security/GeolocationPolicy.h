#pragma once
#include <QtGlobal>

// Web geolocation is refused on Windows until the Qt Positioning WinRT plugin is safe to use.
// Measured on Qt 6.12.0 (docs/VALIDATION.md, "Geolocation plugin crash"): a stand-alone program that only
// creates, starts and destroys the default position source crashes in roughly a third of the processes
// (access violation in qtposition_winrt / Qt6Core while a WinRT callback runs), the same crash that
// terminated the browser test runs. A page can reach that code by asking for the location, so the
// request is denied before the source is created. Build with -DCYBOU_ALLOW_GEOLOCATION=ON to test the plugin.
namespace GeolocationPolicy {
#if defined(Q_OS_WIN) && !defined(CYBOU_ALLOW_GEOLOCATION)
inline constexpr bool available = false;
#else
inline constexpr bool available = true;
#endif
}
