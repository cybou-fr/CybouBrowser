# Архитектура наблюдаемого веба

WebEngine permissionRequested → Main.qml request coordinator → ControlBarController observations → CapabilityModel → QML icons.
URL interceptor → bounded mutex queue (256, sanitized origins) → coalesced queued activityAvailable → GUI controller → model. Network events remain profile-scoped. Interceptor never invokes QML or writes disk. notificationPending collapses bursts until sample drains the notification flag.

QML owns pending permission and view lifetime. Timer denies unanswered requests. View owns grant deadline and unloads its document on expiry. Controller owns tab/origin in-memory behavior exceptions and bounded permission observations. QSettings retains normal browser preferences/data, not controller origin grants.

Expiry refresh timer is single-shot (60 seconds), not 250 ms network polling. Model updates changed rows only. Separate future C++ coordinator/aggregator and typed epoch transactions are described as planned in ROADMAP.
