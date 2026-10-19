#pragma once

// Web geolocation is served by CYBOU's own Qt Positioning plugin (src/geo/CybouPositionPlugin.cpp), which replaced
// the stock WinRT plugin that could crash the process (docs/VALIDATION.md, "Geolocation plugin crash").
// Build with -DCYBOU_DISABLE_GEOLOCATION=ON to refuse every geolocation request instead.
namespace GeolocationPolicy {
#if defined(CYBOU_DISABLE_GEOLOCATION)
inline constexpr bool available = false;
#else
inline constexpr bool available = true;
#endif
}
