# CYBOU — 11 возможностей Control Bar

**0.3.0-alpha.2. Код присутствует; полный security gate не пройден.**

Каждый контроль постоянно виден в одной навигационной строке ([CONTROL_BAR_UX](CONTROL_BAR_UX.md)). Effective policy ≠ pending request ≠ grant ≠ confirmed block ≠ hardware use.

| Иконка | Существующий механизм | Ограничение |
| --- | --- | --- |
| Camera | Native video permission и действие на живой request | Grant ≠ запись или подтверждённая остановка всех streams |
| Microphone | Native audio/combined permission | Combined audio/video требует согласования |
| Screen | Request распознаётся | Capture chooser не интегрирован; не выдавать fake Allow |
| Location | Native geolocation permission | Physical location provider не проверен |
| Notifications | Native permission | Desktop delivery не интегрирована |
| Clipboard | WebEngine permission | Не все операции наблюдаемы |
| JavaScript | WebEngineSettings toggle, reload/Undo | Не наблюдает весь JS |
| Popups | newWindowRequested, user-gesture policy | Не все подавленные попытки видимы |
| Cookies | Third-party cookie policy | Профильный, не per-site scope |
| Storage | localStorageEnabled toggle | Не управляет полностью IndexedDB/Cache/SW |
| Network | HTTPS, interceptor, categorized rules, HTTP consent, tracker origin exception | Profile-level события, неполный filter coverage |

Grant возможен только при совпадении request ID/контекста, top/requester origins, epoch и profile. Temporary HTTP использует отдельный профиль, tracker exception — **текущий профиль на origin**. UI не выдаёт неподтверждённое действие за реальную блокировку.

[SECURITY_MODEL](SECURITY_MODEL.md) · [TEST_PLAN](TEST_PLAN.md).
