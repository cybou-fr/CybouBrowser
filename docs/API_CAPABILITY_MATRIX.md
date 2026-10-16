# CYBOU — матрица текущих возможностей

**Единый нормативный источник:** [CAPABILITY_CONTRACT](CAPABILITY_CONTRACT.md). Этот файл — краткий обзор для аудита, а не историческая спецификация с прежними UX-решениями.

| Группа | Фактический механизм | Предел наблюдения и контроля |
| --- | --- | --- |
| Camera / Mic / Location / Notifications / Clipboard | `QWebEnginePermission` через C++ `PermissionCoordinator`; конкретный pending grant, context-scoped revoke | Grant не означает активное устройство; полный media stop не доказан |
| Screen | Unsupported native capture | Chooser не реализован; право не выдаётся |
| JS / localStorage | `WebEngineSettings`, toggle и reload | Не наблюдаем весь JS, IndexedDB и service workers |
| Popups | `newWindowRequested`, user-gesture gate | Не все подавленные Chromium попытки видимы |
| Cookies | Global third-party profile filter | Не per-site решение из текущей карточки |
| Network | HTTPS strict, ограниченный tracker list, подтверждённые blocked decisions, отдельные HTTP/tracker exceptions | Metadata относятся к профилю; нет полного Ad/Tracker classifier, tab attribution, proxy isolation |
| UI | 11 capability icons в одной навигационной строке, interactive glass hover panels | DPI и реальные device/platform проверки требуют отдельной приёмки |

**Реализация ≠ релизная проверка.** Подробные доказательства: [VALIDATION](VALIDATION.md), [TEST_PLAN](TEST_PLAN.md). Первоначальные устаревшие отчёты хранятся отдельно в исходных исторических документах `docs/controlbar-source/`, `docs/omnibox-source/`, `docs/v08-source/` и не переопределяют [PRODUCT_VISION](PRODUCT_VISION.md).
