# CYBOU — архитектура Observable Browsing

Текущий код C++20 / Qt 6 WebEngine / Qt Quick QML. **Расположение Control Bar описано только в [CONTROL_BAR_UX](CONTROL_BAR_UX.md);** здесь — потоки данных и границы возможностей.

## Permission pipeline

`WebEngineView.onPermissionRequested` → C++ `PermissionCoordinator` → `SiteContext` (view/profile weak refs, tab UID, top origin, requester origin, navigation epoch) → checked request-ID decision → `ControlBarController` observations → `CapabilityModel` → `ControlBarInline` / `CapabilityPopover`.

Pending request принадлежит coordinator; `QWebEnginePermission` не выдаётся автоматически и не сохраняется для будущих посещений. Coordinator реализует `PermissionLease` для выданных прав; QML осуществляет замену document/view при отзыве контекста. Это пока не доказанная возможность остановить активный hardware stream отдельно от документа. Controller хранит tab/origin JS/storage/popup exceptions и ограниченный журнал событий в памяти; `QSettings` хранит другие обычные browser preferences/data.

**Известный gap:** explicit deny записывается относительно top origin, а проверка `shouldDeny(permission)` может обращаться к requester origin. Это задача согласования scopes, а не основание заявлять автоматический grant.

## Network pipeline

`RequestObserver` перехватывает запросы `QWebEngineUrlRequestInterceptor`, сначала применяя запреты небезопасного транспорта и credentials, затем ограниченный `TrackerBlocker`. Формируется mutex-protected bounded queue событий с sanitized origin/firstParty, resource type, time, block reason/decision. `activityAvailable` объединяет частые обновления, UI получает модель через `ControlBarController`; interceptor не обращается к QML и не пишет сетевые метаданные на диск.

Сетевая выборка **profile-scoped**. `firstParty` и resource type сами по себе не устанавливают tab ID или результат успешной загрузки.

Для явного HTTP-исключения существует `HttpNavigationLease` в выделенном off-the-record профиле для точного адреса с ограниченным same-origin HTTP content. Для явного tracker-only исключения — `TrackerExceptionLease` в отдельном off-the-record профиле: оно ограничено документом и first-party origin и пропускает совпадающие запросы из небольшого tracker list. UI показывает информацию и revoke в **существующей Network hover-панели**. Оба режима не являются процессной или маршрутизирующей изоляцией.

## Неприкосновенная граница: никакой extension platform

В CYBOU **не существует допускаемой архитектуры сторонних расширений**. Не строить WebExtensions/CRX runtime, `QWebEngineExtensionManager` install/load flow, plugin store/side-loading или privileged userscript execution. Нативные функции вдохновлённые лучшими расширениями пишутся и сопровождаются **в C++/Qt/QML самого CYBOU**, с теми же границами permissions/network/profile, что у остальных функций. Никакая партнёрская интеграция, включая Proton, не может расширить эти полномочия.

Новый сторонний executable, background helper или динамическая библиотека не добавляется ради удобства без прямого продуктового решения и review происхождения, версии, лицензии, sandbox, patch cadence и альтернатив в имеющемся стеке. Сам Qt WebEngine/Chromium остаётся необходимой отдельно контролируемой внешней зависимостью. JavaScript сайтов не переносить в привилегированный browser-owned контекст. Детальный нормативный контракт: [ADR 0006](ADR/0006-no-third-party-extensions.md).

## Будущий каталог облачных сервисов (решение принято; кода пока нет)

Cloud services — **provider-neutral catalogue**, устроенный как независимая от `searchEngine` пользовательская настройка. Значение по умолчанию `proton`, равноправные `google`, `microsoft`, `infomaniak`, `zoho`, `nextcloud`, отключение `none`; Apple iCloud и file-only сервисы — следующий этап. Проектируемые `CloudProviderCatalog` и `CloudProviderPreferences` — **часть нашего C++/QML**, статические неисполняемые descriptors официальных HTTPS-веб-приложений; выбранные ярлыки выводятся на Alpine Dawn и открываются как обычные вкладки.

Нет browser extensions, injected scripts, companion CLI, облачного proxy и доступа к токенам пользователя. Выбор не меняет поисковик, сеть/permissions, cookies или существующие вкладки. Для собственного Nextcloud — явно указанный пользователем HTTPS origin и проверка схемы. Это **план реализации, не описание существующего API**. Полный контракт: [ADR 0007](ADR/0007-selectable-cloud-services.md), запрет стороннего кода: [ADR 0006](ADR/0006-no-third-party-extensions.md).

## Будущие компоненты — ещё не implemented

| Компонент | Ответственность |
| --- | --- |
| `EffectivePermissionPolicy` | Согласование top/requesting-origin deny без ослабления request-ID контекста |
| Media lifecycle control | Проверка реального stop/revoke и UX продолжительных сессий |
| `RuleEngine` / `RuleSetStore` | Версионированные ads/tracker rule snapshots, atomic update/rollback, bounded lookup |
| `SiteFilterPolicy` / `PolicyTransaction` | Независимые temporary ads/tracker policies, точный context/Undo без глобального выключения |
| `ActivityAggregator` | Достоверные permission/popup/network события со scope `tab`, `profile` или `unknown` |
| `NavigationGuard` | Локальные URL/IDN/redirect/private-network проверки |
| `RestrictedContextFactory` | JS-off до первого документа и ограниченный browsing context |

План и критерии: [SECURITY_UX_EVOLUTION](SECURITY_UX_EVOLUTION.md), [ROADMAP](ROADMAP.md), [TEST_PLAN](TEST_PLAN.md). Наличие строк в этой таблице **не означает**, что реализации уже существуют.
