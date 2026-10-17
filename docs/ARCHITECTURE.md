# CYBOU — текущая архитектура

**0.3.0-alpha.1 · C++20 / Qt 6 WebEngine / QML · публичного релиза нет.**

## Исполняемые части

- `cybou` — GUI production executable, `src/main.cpp` и QML module `Cybou`.
- `cybou_tests` — отдельный тестовый executable с локальными fixtures.
- `src/browserstore.h` — настройки, профили, локальное browser-state.
- `qml/Main.qml` — вкладки, навигация, соединение C++ контроллеров и browser-owned интерфейса.
- `qml/BrowserTab.qml` — WebEngineView и page lifecycle.
- `qml/NewTabPage.qml` — Alpine Dawn с компактным floating title, glass search и Cloud App Launcher.

## Существующие контуры

**Permissions:** QWebEnginePermission → SiteContext (tab/view/profile, top/requester origins, navigation epoch) → C++ PermissionCoordinator → ControlBarController → разрешение только текущего native request. Реальная остановка hardware media пока требует проверки.

**Network:** profile request interceptor, NavigationGuard, HTTPS, RuleEngine (Ad/Tracker), TrackerExceptionRegistry, RequestObserver и ActivityAggregator. Сетевые события на уровне профиля не доказывают конкретную вкладку.

**HTTP exception:** exact-address consent и отдельный off-the-record профиль, не переносит обычную сессию. **Tracker exception:** разрешение фильтра по origin в *текущем* профиле с cookies/session, привязанное к вкладке-владельцу; не путать эти механизмы.

**Cloud:** `src/cloud/CloudProviderCatalog.*` и `CloudServiceManager.*`, `qml/CloudSettingsSection.qml`, `CloudAppLauncher.qml`, локальные SVG. Proton default и per-role overrides; запускаются обычные HTTPS-веб-приложения, без токенов и native integration с аккаунтами.

## Границы

Нет сторонней системы расширений/привилегированных userscripts ([ADR 0006](ADR/0006-no-third-party-extensions.md)). Qt/Chromium и системные библиотеки — внешние зависимости, подлежащие проверке.

Реальный sandbox, OS/hardware behavior, DNS rebinding, WebRTC и полная cloud login compatibility не квалифицированы. [SECURITY_MODEL](SECURITY_MODEL.md) · [TEST_PLAN](TEST_PLAN.md).

## Temporary-profile native ownership

`BrowserStore` accepts an optional C++ `TemporaryProfileFactory` at construction. The default path creates the same off-the-record `QQuickWebEngineProfile` as before; there is no QML setter, certificate file option or command-line trust switch in the production app. A factory returns both a profile and its native owning QObject, supporting Qt's `WebEngineProfilePrototype` ownership without deleting a borrowed profile twice. Temporary creation still requires off-the-record state, AskEveryTime permissions, the configured interceptor and an exact HTTP lease.

On release, the lease is revoked and the recorded owner is scheduled for deletion. Default profiles own themselves; prototype-backed profiles are destroyed by the prototype. Observer/temporary-owner bookkeeping is removed when the profile dies. The transport test alone supplies a prototype with a loopback-only test CA, using public Qt QML APIs and preserving the production UI revoke path. The production executable does not supply that factory or test trust.
