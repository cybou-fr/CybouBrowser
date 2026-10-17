# CYBOU Browser — roadmap и проверяемый статус

**Срез кода:** `main`, изменения `51559ca` (Control Bar и surfaces) и `9f7b698` (tracker exception), 17.10.2026. Позднейшие изменения документации не равны изменениям кода.

**Статусы:** **implemented** — существует код; **tested (recorded)** — ранее сохранён результат локального теста; **unverified** — не подтверждено на реальных сценариях; **planned** — ещё нет реализации. Детали и журналы — в [VALIDATION](VALIDATION.md).

## Реализовано в текущем коде

| Возможность | Статус и границы |
| --- | --- |
| **Control Bar** | **implemented, tested (recorded):** 11 иконок в одной навигационной строке, адрес занимает остаток ширины, hover открывает интерактивные стеклянные карточки; test harness на четырёх размерах при DPR 1 |
| **Browser surfaces** | **implemented, tested (recorded):** отдельные GlassToolTip для навигации, drawers под шапкой, управление вкладками и окном |
| **Чувствительные permissions & Iframe матрица (S0.1)** | **implemented, tested (recorded):** C++ `PermissionCoordinator`, единый `EffectivePermissionPolicy`, request ID, `SiteContext`, fail-closed ожидание, независимые leases, `AskEveryTime`; FIFO-очередь запросов (`MaxQueuedRequests = 16`) с приоритетом активной вкладки и атомарной отменой (`revokeAll`/`forgetView`/`documentLoaded`); расширенная интеграционная матрица cross-origin iframe (каскадный top-deny, изолированный requester-deny, составные разрешения, UI-различение с кнопкой «Bloquer le cadre») |
| **Мягкий отзыв media (S0.2)** | **implemented, tested (recorded):** отзыв прав на медиа (`PermissionCoordinator::revokeLease`) прекращает захват потоков без уничтожения документа, обрыва активных звонков и перезагрузки вкладки |
| **Transport** | **implemented, tested (recorded):** production HTTPS strict; exact-address HTTP consent в отдельном off-the-record профиле, последующее завершение/отзыв без наследования обычных cookies |
| **RuleEngine и независимые Ads / Trackers (S1.1 + S1.2)** | **implemented, tested (recorded):** типизированный версионированный `RuleEngine` с классификацией Ad vs Tracker, иммунитет к false-positive (критические CDN, платежи Stripe/PayPal, SSO-авторизация, CAPTCHA); независимые временные исключения для рекламы и трекеров на уровне origin в `TrackerExceptionRegistry` и карточке Network |
| **ActivityAggregator и семантика цветов (S1.3)** | **implemented, tested (recorded):** строгое разделение `EffectivePolicy` и `ObservedEvent`, правдивый скоуп (tab vs profile), разграничение индикаторов активности от постоянных разрешений |
| **Navigation Guard и PNA (S2.1)** | **implemented, tested (recorded):** блокировка обращений публичных страниц в локальную сеть/localhost (Private Network Access / RFC 1918) и детекция IDN-гомографических атак в `NavigationGuard` |
| **Restricted Browsing (S2.2)** | **implemented, tested (recorded):** запуск подозрительных сайтов (IDN-гомографы, опасные сетевые порты) в защищённом режиме: JS-off до загрузки документа, автозапрет sensitive permissions, отключение localStorage и popups, предупреждающий баннер безопасности с кнопками безопасного выхода и явного включения JS |
| **Cloud Services (ADR 0007 / Cloud Glass 2.0)** | **implemented, tested (recorded):** выбираемая облачная экосистема (Proton, Google, Microsoft, Infomaniak, Zoho, Nextcloud, Apple iCloud), расширенный каталог экосистемы (Proton Pass, Lumo AI, Proton Wallet, Proton VPN portal, Google Keep, OneNote, To Do), FrostedGlassSurface с динамическим размытием обоев Alpine Dawn, 13 векторных SVG-иконок без сетевых запросов, 3-колоночный скроллируемый лаунчер без вложенных рамок и мешающих тултипов, Nextcloud self-hosted с валидацией HTTPS, строгая валидация и восстановление настроек |
| **Network events** | **implemented:** ограниченная очередь, sanitized origin, blocked reason и type, только scope профиля; нет проверенного per-tab attribution |
| **Windows** | **tested (recorded):** C++20 Release, Qt 6.12/MSVC, security/controlbar/surface/transport suites; Linux/macOS и release qualification остаются unverified |

## Постоянное архитектурное ограничение — ПРИНЯТО

**Никаких сторонних расширений.** Отсутствие Chrome/WebExtensions/CRX, магазина, side-loading и privileged third-party userscripts — принципиальная спецификация продукта, а не незавершённая функция. Сильные возможности популярных расширений можно изучать и реализовывать собственным C++/Qt/QML-кодом. Партнёрства (в том числе Proton) не дают исключений. Внешние executables, CLI и фоновые компоненты минимизируются, а обязательные Qt/Chromium зависимости учитываются и поддерживаются. Нормативное решение: [ADR 0006](ADR/0006-no-third-party-extensions.md).

**Release gate (план проверки):** инвентаризация исполняемых зависимостей и подтверждение отсутствия extension loader/store/install/update, privileged userscripts и внешних executable helpers в обязательной поставке. Пока не отмечено `tested`.

## Cloud Services — РЕАЛИЗОВАНО И ПРОВЕРЕНО (ADR 0007 / Cloud Glass 2.0)

**Режим выбираемой облачной экосистемы и Cloud Glass 2.0** — выбор общего провайдера (`proton` по умолчанию, Google Workspace, Microsoft 365, Infomaniak, Zoho, Nextcloud Hub, Apple iCloud; `none` для выключения), независимая настройка для каждой из 7 ролей (Mail, Calendar, Files, Docs, Sheets, Presentations, Meetings), а также полноценная поддержка расширенных веб-сервисов экосистемы:
- **Proton:** Proton Mail, Proton Calendar, Proton Drive, Proton Docs, Proton Sheets, Proton Meet, а также отдельные веб-приложения: Proton Pass (`pass.proton.me`), Lumo AI (`lumo.proton.me`), Proton Wallet (`wallet.proton.me`), портал Proton VPN (`account.proton.me/u/0/vpn`) и информация об Authenticator (`proton.me/pass/authenticator`).
- **Google & Microsoft:** поддержка Google Keep, Microsoft To Do и OneNote без конфликта с ролями.
- **Cloud Glass 2.0:** переработан `CloudAppLauncher.qml` с использованием `FrostedGlassSurface.qml` (настоящее размытие обоев NewTabPage в координатах окна, световой блик, тонкая окантовка 1.5px), устранение визуального шума «карточки внутри карточек» в `CloudAppTile.qml` (прозрачный контейнер в покое, мягкая подсветка `#1838BDF8` при наведении, кольцо фокуса клавиатуры `#38BDF8`), удаление перекрывающих тултипов, переход на 13 локальных векторных SVG-иконок (`assets/cloud/`) без сетевых утечек.
- **Self-hosted Nextcloud:** требование предварительной настройки (`requiresConfiguration`), проверка персонального HTTPS base URL и санитизация адреса.
- **Безопасность запуска:** C++ валидация URL (`resolveAppUrl`) и верификация хостов (`isVerifiedServiceUrl`). Статус: **implemented, tested (recorded)**.

## Далее — приоритетный порядок

| ID | Приоритет | Задача | Статус | Критерий приёмки |
| --- | --- | --- | --- | --- |
| S0.1 | P0 | Одинаковое применение explicit deny к top origin и вложенным requester iframe (`ControlBarController::shouldDenyOrigin`) | **implemented, tested** | Двухoriginные fixtures, stale ID/tab/profile/epoch тесты |
| S0.2 | P0 | Достоверный и пригодный для звонков отзыв media без разрушения документа (`PermissionCoordinator::revokeLease`) | **implemented, tested** | Прекращение media track без reload/crash документа, сохранение остальных leases |
| S0.3 | P0 | Поддерживаемый патченный Qt/Chromium, sandbox/release flags и безопасная доставка обновлений | **planned** | Runtime-проверка, регрессия и проверка обновлений |
| S1.1 | P1 | Версионированный ad/tracker RuleEngine с проверкой целостности и false positives | **implemented, tested** | Разделение категорий Ad vs Tracker, иммунитет к CDN/платежам/SSO/CAPTCHA |
| S1.2 | P1 | Независимые временные Ads / Trackers политики **в Network hover-карточке** | **implemented, tested** | Раздельные действия для Ads и Trackers, безопасный Undo, привязка к вкладке и origin |
| S1.3 | P1 | Семантика policy/event и `ActivityAggregator` с правдивым scope | **implemented, tested** | Default != blocked event; grant != recording; profile != tab |
| S2.1 | P2 | Локальный Navigation Guard: IDN/redirect/private-network (PNA) | **implemented, tested** | Блокировка публичных запросов в LAN, детекция гомографов |
| S2.2 | P2 | Restricted Browsing: JS-off до документа и проверяемые ограничения | **implemented, tested** | Before-load tests, корректный отказ и выход |
| S3 | research | Process/proxy isolation | **research** | DNS/WebRTC/route leak tests; никакого рекламного обещания заранее |

## Правила внесения изменений

Все изменения прямо в `main`, **без веток**, после локальных тестов. GitHub Actions пока не является обязательным условием. Никакого редизайна Control Bar, Always Allow, глобального opt-out из карточки сайта или фальшивых событий. Старые интеграционные планы — исторический материал и не переопределяют [PRODUCT_VISION](PRODUCT_VISION.md) и [CONTROL_BAR_UX](CONTROL_BAR_UX.md).

**Детальный план реализации:** [SECURITY_UX_EVOLUTION](SECURITY_UX_EVOLUTION.md). **Проверки:** [TEST_PLAN](TEST_PLAN.md).
