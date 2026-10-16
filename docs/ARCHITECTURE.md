# Архитектура наблюдаемого веба

WebEngine permissionRequested → C++ PermissionCoordinator → ControlBarController observations → CapabilityModel → QML icons.
URL interceptor → bounded mutex queue (256, sanitized origins) → coalesced queued activityAvailable → GUI controller → model. Network events remain profile-scoped. Interceptor never invokes QML or writes disk. notificationPending collapses bursts until sample drains the notification flag.

C++ PermissionCoordinator owns the pending native request, SiteContext and independent per-request PermissionLease deadlines. A single-shot timer schedules the nearest deadline; resolve also checks absolute expiry. QML owns view lifetime and destroys/replaces the requested WebEngineView on revokeDocument, keeping tab position, pinned and mute state. A sensitive capability reset recreates the same URL in that slot. Button press captures the exact request ID; release cannot grant another request. Confirmed document replacement retires the old epoch; failed navigation retains its safety deadline. Controller owns tab/origin in-memory behavior exceptions and bounded permission observations. QSettings retains normal browser preferences/data, not controller origin grants.

Expiry refresh timer is single-shot (60 seconds), not 250 ms network polling. Model updates changed rows only. Future activity aggregator and typed policy transactions are described as planned in ROADMAP.

HTTP exceptions use a dedicated off-the-record profile per view, with an interceptor-owned mutex-protected HttpNavigationLease. The exact main-frame URL is consumed once; same-origin HTTP resources require a matching firstParty origin. A native 60-second timer and absolute deadline enforce expiry. QML consent captures tab UID/navigation epoch. Subsequent navigation, revoke or expiry replaces the view with a blank normal-profile view; profile deletion is deferred until delegate teardown. No page-controlled navigation is replayed with normal-profile cookies.

TrackerExceptionLease similarly owns an exact document navigation and a 60-second deadline in a separate profile. Tracker exceptions apply only to matching firstParty origins, after the initial document request; transport/credential checks precede any exception. Main.qml maps both kinds of temporary profiles by tab UID and tears them down through one view replacement path. Network samples add translated resource labels, categories and explicit filter decisions. The model exposes bounded per-category rows and profile-sample counts. A one-second UI countdown runs only while the current tab has a tracker exception; native expiry remains independent of QML timers.

## Дальнейшая архитектурная эволюция (новые компоненты пока не реализованы)

Сохранить существующие `ControlBarInline.qml`, интерактивный `CapabilityPopover.qml` и сетевую визуализацию; менять реальные источники политики/данных и их контракты, не делать ещё одну панель.

| Будущий компонент | Обязанности и boundaries |
| --- | --- |
| `EffectivePermissionPolicy` | Совместная проверка пользовательского explicit deny по top origin и requester iframe до C++ grant; без автоматического pregrant |
| `PermissionSessionUX` | Изучить native media revoke на реальном устройстве; UX предупреждения/завершения или ограниченного явного продления только при доказанном fail-closed |
| `RuleEngine` + `RuleSetStore` | Immutable snapshots index/rules ad/tracker с проверяемым источником, целостностью, bounded lookup в interceptor, атомарным update/rollback |
| `SiteFilterException` + `PolicyTransaction` | Независимые Ads/Trackers изменения scoped по tab UID, top origin, profile, navigation epoch, TTL; проверяемые Undo/reload и expiry, не глобальные отключения |
| `ActivityAggregator` | Подтверждённые permission/popup/transport/network events с явной областью `tab`/`profile`/`unknown`, ограниченные памятью и временем; без query/paths/headers |
| `NavigationGuard` | URL/IDN/redirect/private-network checks до опасного перехода; нет внешней передачи полного URL по умолчанию |
| `RestrictedContextFactory` | JS-off до первой загрузки при достаточных risk signals; ограничения permissions/popups/downloads; separate profile не равен separate process |

Порядок принятия решения: **обязательные network denies HTTPS/URL credentials** → независимые verified tracker/ad rules с временными site exceptions → результат block/allow с подтверждаемой причиной → bounded sanitized event → QML model/hover. Интерцептор не обращается к QML и не компилирует крупные правила на горячем пути. Внутренний `firstParty` текущего request сам по себе не устанавливает вкладку. Измерять CPU/memory/latency при burst, ложные срабатывания и отмену действий после смены контекста.

Текущая система уже включает `PermissionCoordinator`, `ControlBarController`, `TrackerBlocker`, `RequestObserver` и **`TrackerExceptionLease`** — строго ограниченный tracker-only доступ в отдельном 60-секундном профиле. Новый `RuleEngine` и независимые Ads/Tracker политики по-прежнему не реализованы. Развивать существующий контракт исключения, а не создавать второй несовместимый механизм. Этапы описаны в [SECURITY_UX_EVOLUTION](SECURITY_UX_EVOLUTION.md) и [ROADMAP](ROADMAP.md).
