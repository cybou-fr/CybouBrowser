# Архитектура наблюдаемого веба

WebEngine permissionRequested → C++ PermissionCoordinator → ControlBarController observations → CapabilityModel → QML icons.
URL interceptor → bounded mutex queue (256, sanitized origins) → coalesced queued activityAvailable → GUI controller → model. Network events remain profile-scoped. Interceptor never invokes QML or writes disk. notificationPending collapses bursts until sample drains the notification flag.

C++ PermissionCoordinator owns the pending native request, SiteContext and independent per-request PermissionLease deadlines. A single-shot timer schedules the nearest deadline; resolve also checks absolute expiry. QML owns view lifetime and destroys/replaces the requested WebEngineView on revokeDocument, keeping tab position, pinned and mute state. A sensitive capability reset recreates the same URL in that slot. Button press captures the exact request ID; release cannot grant another request. Confirmed document replacement retires the old epoch; failed navigation retains its safety deadline. Controller owns tab/origin in-memory behavior exceptions and bounded permission observations. QSettings retains normal browser preferences/data, not controller origin grants.

Expiry refresh timer is single-shot (60 seconds), not 250 ms network polling. Model updates changed rows only. Future activity aggregator and typed policy transactions are described as planned in ROADMAP.
