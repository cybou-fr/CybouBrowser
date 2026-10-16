# CYBOU Browser — Observable Browsing / Direct Capability Control

> **Историческое предложение (исходный аудит 16.10.2026, коммит `0be4370`).** Сведения ниже о C++17, незавершённом PermissionCoordinator, временном HTTP и первоначальных вариантах Control Bar описывают **старую базу**, а не нынешний код. Сегодня используется C++20, независимые permission leases, HTTP-профиль и **11 кнопок в одной строке с интерактивными стеклянными hover-панелями**. Не применять исторические макеты вместо этих решений. Источник актуального состояния: [ROADMAP](ROADMAP.md), [VALIDATION](VALIDATION.md), [SECURITY_UX_EVOLUTION](SECURITY_UX_EVOLUTION.md), [CONTROL_BAR_UX](CONTROL_BAR_UX.md).

**Статус:** план реализации, не отчёт об уже выполненных изменениях.  
**Аудит:** `cybou-fr/CybouBrowser`, `main`, commit `0be4370ec65d13340aa0c2b80b7940d41d87292f`, 16.10.2026.  
**Технология:** C++17, Qt 6.9+/6.12 WebEngine, Qt Quick/QML.  
**Правило разработки:** небольшие последовательные коммиты непосредственно в `main`, без создания веток. Каждый этап сопровождается локальной сборкой, регрессионными тестами и скриншотами. GitHub Actions не являются обязательным условием выполнения плана.

---

## 0. Обязательное определение продукта

**CYBOU — браузер наблюдаемого и управляемого веба.**

Вместо того чтобы делать адресную строку и настройки центром взаимодействия, CYBOU постоянно показывает пользователю **что пытается делать страница, какие возможности ей доступны, какие запрещены и что браузер реально заблокировал**. Каждая доступная возможность имеет собственную видимую кнопку: она одновременно индикатор политики, индикатор достоверно наблюдаемого события и непосредственный элемент управления. Не требуются штатные ASK-диалоги, отдельные страницы настроек для обычных действий или постоянное доверие сайту.

**Основной цикл:** Observe → Restrict → Control → Revoke.

### Неизменяемые продуктовые инварианты

1. **Capability-first UI.** Главный визуальный приоритет — постоянно видимая Control Bar; URL — компактный вспомогательный элемент, доступный в полном виде по клику и `Ctrl+L`.
2. **No persistent positive trust.** Никаких «Всегда разрешать» и сохранённых ALLOW для сайта. Открытие ранее посещённого сайта не восстанавливает право на микрофон, камеру, геолокацию или иные чувствительные возможности.
3. **No ASK as a user-facing policy.** На панели нет состояния «Спросить» и штатного модального permission prompt. Внутреннее состояние Qt/Chromium может ожидать решения, но UI отображает лишь фактическую недоступность права и отдельный обнаруженный запрос. При отсутствии реакции запрос отклоняется по безопасной политике.
4. **Direct control.** Один клик меняет доступную обратимую политику. Если сайт прямо сейчас запросил разрешение, один клик по соответствующей подсвеченной иконке разрешает *именно этот* запрос, если это поддерживается и запрос ещё действителен. Никаких автоматических предразрешений на все будущие запросы.
5. **Temporary by construction.** Разрешающее исключение ограничено запросом, контекстом, вкладкой и/или временем настолько, насколько позволяет реальный API. При навигации, закрытии вкладки и завершении допустимого периода оно отзывается. Активные медиа-потоки не объявляются остановленными без доказательства.
6. **Truthful observability.** Политика / попытка / разрешение / блокировка / действительное использование — независимые измерения. Никаких ложных «камера записывает» на основании одного grant. Никаких пер-таб счётчиков на основании профильно-общей выборки.
7. **Secure transport by default.** Публичный HTTP запрещён до явного временного разрешения; HTTPS-First может использоваться для автоматического апгрейда. Исключение не становится постоянным доверием ко всему домену. Для локального HTTP-тестирования — отдельный явно включённый режим разработки или изолированный тестовый профиль.
8. **Transparent blocking.** Блокирование рекламы и трекеров включено по умолчанию. На панели — раздельные состояния и счётчики только тогда, когда движок реально различает категории. Временное смягчение фильтра для сайта не отключает защиту других сайтов.
9. **Risk-adaptive restrictions.** При обоснованных признаках риска CYBOU может применять ограничения, включая JS-off *до* загрузки документа. Прокси/изоляция — отдельная проверяемая возможность, а не синоним безопасности; неизвестность сама по себе не доказывает злонамеренность.
10. **Honest limitations.** Не заявлять «наблюдаем все действия веб-страницы». Не выдавать сетевой interceptor за контроль всего JavaScript/DOM. Не утверждать, что браузер предотвращает все взломы или phishing.

### Различать типы постоянства

Запрещены **постоянные разрешающие привилегии для origin**, но обычные данные браузера (закладки, история, сессия, first-party cookies, локальное хранилище) могут сохраняться согласно отдельной ясно описанной политике приватности. Не путать «нет постоянного доверия» с «браузер ничего не хранит».

### Неприемлемые решения

- Спрятать основные 11+ возможностей за тремя категориальными кнопками.
- Отдать адресной строке больше места ценой исчезновения Control Bar.
- Вернуть «Always allow» даже как экспертную функцию.
- Считать цветовую подсветку доказательством работающего устройства.
- По клику на значок сайта отключать трекер-блокирование глобально.
- Включать бездоказательный «proxy mode» без гарантии маршрутизации всех связей изолированного контекста.

---

## 1. P0 — Документация как единый источник истины (первый коммит)

**Принцип:** сначала договориться о продукте и точных контрактах, затем исправлять код. Документация не должна заявлять будущие функции реализованными.

### 1.1. Создать документы

| Документ | Назначение и обязательное содержание |
|---|---|
| `docs/PRODUCT_VISION.md` | Определение CYBOU, пользовательские сценарии, инварианты выше, non-goals, сравнение с обычным браузером, что делает CYBOU уникальным. Основная версия на русском; после утверждения — поддерживаемая английская/французская версия для сайта. |
| `docs/SECURITY_MODEL.md` | Default deny для чувствительных возможностей; HTTPS; временные grant/lease; origin/requester/tab/profile; модель угроз; что не гарантируется; риски компрометации ранее знакомого сайта. |
| `docs/CAPABILITY_CONTRACT.md` | Матрица для каждой кнопки: наблюдаемый сигнал, default, действие клика, реальное Qt API, область, срок, отзыв, ограничения, подтверждение состояния, тесты. |
| `docs/CONTROL_BAR_UX.md` | Постоянно видимая панель; независимые layers `policy/request/event`; семантика цветов и анимаций; hover=tooltip; click=action; focus, keyboard, DPI, адаптивная ширина, поведение адреса, отдельные Downloads/Menu. |
| `docs/ARCHITECTURE.md` | Потоки данных Event→C++ Model→QML, ownership, thread safety, permission leases, network policy, layout компонентов; диаграммы и границы Qt WebEngine. |
| `docs/ROADMAP.md` | P0–P4, зависимости, цели/приёмка каждого этапа; *implemented / in progress / planned / unsupported*. |
| `docs/TEST_PLAN.md` | Автоматические, интеграционные и ручные проверки с доказательствами; обновление `--security-test`, `--smoke-test`, `--controlbar-test`. |
| `docs/ADR/0001-capability-first-ui.md` | Почему отказались от 3-button UX, ASK/Always Allow, централизованного URL-first UI и постоянного доверия. |
| `docs/ADR/0002-permission-lifecycle.md` | Выбранный permission policy, область, lease expiration, принудительное уничтожение WebEngineView при опасном сбое. |
| `docs/ADR/0003-http-and-isolation.md` | Строгий HTTP-контракт, local dev policy, принципы risk mode, невозможность объявлять обычный Qt proxy пер-вкладочной изоляцией. |

Не обязательно сразу создавать десятки пустых документов: в **первом docs-коммите** полностью заполнить PRODUCT_VISION, SECURITY_MODEL, CAPABILITY_CONTRACT, CONTROL_BAR_UX и ROADMAP; остальные создавать по мере начала соответствующей реализации.

### 1.2. Изменить существующие файлы

- `README.md`: первое предложение — про Observable Browsing и прямую панель управления, а не Dark Glass/Alpine Dawn; разделить `Implemented today` и `Target product`; не называть статический список полноценной ad-blocking системой.
- `docs/API_CAPABILITY_MATRIX.md`: актуализировать с исторической версии 0.5 до проверенного main 0.8, сохранить отдельную историю ограничений, добавить колонку *достоверное наблюдение*.
- `docs/CONTROL_BAR_INTEGRATION.md`, `docs/OMNIBOX_071_INTEGRATION.md`, папки `docs/controlbar-source`, `docs/omnibox-source`, `docs/v08-source`: пометить как **исторические отчёты/спецификации**, а не действующее продуктовое ТЗ. Ссылки на новый нормативный документ.
- `docs/design`: текущие старые макеты могут остаться как история; завести новые acceptance screenshots интерфейса после реальной сборки.
- Для `cybou.fr` позднее адаптировать утверждённый product pitch, без обещаний несуществующей защиты.

**Приёмка:** любой разработчик или агент может ответить, что делает один клик по каждой иконке, когда заканчивается разрешение, зачем нужен URL и что реально обнаруживается/блокируется, не обращаясь к чату.

---

## 2. Состояние кода на audited main и явные разрывы с продуктом

| Сейчас (доказуемое по исходнику) | Место | Новое обязательство |
|---|---|---|
| `persistentPermissionsPolicy: WebEngineProfile.StoreOnDisk` | `qml/Main.qml:620–624` | Не сохранять положительные native permission grants. Проверить Qt `AskEveryTime`/явную lease-обвязку на реальном движке. |
| `askPermission()` открывает `permissionDialog` | `qml/Main.qml:220–255`, `:1019` | Permission event→Control Bar, no modal; таймаут на отклонение запроса. |
| UI `Demander`, `Toujours autoriser` | `qml/CapabilityPopover.qml:118–137` | Убрать из пользовательского управления, запретить persist-allow. |
| `setPermissionPolicy(mode=1)` может `permission.grant()` для persistent Qt типов | `src/controlbar/ControlBarController.cpp:80–89` | Разрешать только действительный запрос/lease; не писать постоянное Allow. |
| QSettings хранит JS/storage/popups per-origin | `ControlBarController.cpp:29–58` | Разрешающие исключения для сайта по умолчанию временные; отделить предпочтения пользователя для глобальных безопасных defaults. |
| `m_timer.start(250)` и сборка модели опросом | `ControlBarController.cpp:8–12` | Event-driven обновления, coalescing и bounded event buffer, без тяжёлой работы в interceptor. |
| У всех камер/микрофонов одни цвета объектов, не политика | `qml/ControlBarIcon.qml:10–33` | Политика и события визуально независимы: neutral/allow/deny/request/confirmed-use. |
| Hover открывает общий `CapabilityPopover` через таймеры | `qml/ControlBarInline.qml` | Tooltip строго текущей иконки, защита от stale context, click=действие. |
| В EDIT исчезает **вся** Control Bar | `qml/Main.qml:851–857` | Адрес компрессируется; панель остаётся видимой в обычном режиме. Для `Ctrl+L` допускается временный фокус, но после ухода из него всё возвращается автоматически. |
| WidthBudget включает жёсткую формулу `6*30 + 7*spacing + 160` | `qml/Main.qml:855` | Реальные layout constraints, отдельный контейнер Control Bar. |
| `httpsOnly` default false, `httpsFirst` true | `src/browserstore.h:104–145` | Публичный HTTP deny-until-explicit-allow; отдельная узкая временная capability. |
| HTTPS fallback whitelist хранит host в `m_httpFallbackHosts` до конца процесса | `src/browserstore.h:134–142` | Контекстное ограничение на конкретную навигацию, без повторного доверия origin. |
| `TrackerBlocker` — короткий hardcoded suffix-list | `src/security/TrackerBlocker.h` | Версионируемые правила, классификация рекламы/трекинга, обновление/подпись, тесты false positives. |
| `RequestObserver` блокирует до формирования sample; счётчик atomic общий | `src/controlbar/RequestObserver.h` | Событие `Blocked` и подтверждённая firstParty/site attribution без путаницы вкладок. |
| Карточка Network переключает глобальный `browserStore.blockTrackers` | `qml/CapabilityPopover.qml:143–156` | Контекстное временное послабление для сайта, global defaults остаются. |
| `PolicyBanner` хранит `prevMode` как int, но получает boolean; обращается к `currentView` | `qml/Main.qml:863–866`, `:928–941` | Типизированная транзакция с `profile`, `tabUid`, `origin`, `key`, `before/after`, epoch; Undo применяется к исходному контексту. |
| `onNavigationRequested` проверяет `isMainFrame` при применении JS | `qml/Main.qml:989–997` | Сохранить исправление, добавить тест cross-origin iframe. |

**Важно:** анализ исходников ≠ подтверждение поведения на реальных сайтах. Текущие тесты задокументированы в репозитории, но в этом аудите сборка Windows не запускалась.

---

## 3. P0 — Корректная security-модель, прежде чем переписывать UI

**Файлы:** `qml/Main.qml`, `src/controlbar/ControlBarController.{h,cpp}`, `src/browserstore.h`, `src/controlbar/RequestObserver.h`, `src/security/NetworkPolicy.h`, новые `src/security/PermissionCoordinator.*`, `src/security/PermissionLease.*`, `src/security/SiteContext.*`, `src/security/PolicyTransaction.*`.

### 3.1. Контекст

`SiteContext { profileId, tabUid, topOrigin, requestingOrigin, navigationEpoch }`.

Requesting origin может отличаться от top origin (iframe); нельзя разрешать неизвестному третьему origin на основании доверия к открытому домену. Навигация и закрытие вкладки инвалидируют leases и pending events атомарно с точки зрения UI.

### 3.2. Политика ≠ попытка ≠ использование

Представление состояния:

- `EffectivePolicy`: `Blocked`, `TemporarilyAllowed`, `Unavailable`, `ReloadRequired` (по типам).
- `ObservedActivity`: `Idle`, `RequestPending`, `AttemptBlocked`, `AccessGranted`, `ActivityConfirmed`, `Unknown`.
- `LeaseStatus`: `None`, `Pending`, `Active`, `Expired`, `Revoking`, `Revoked`, `RevocationUnverified`.

Qt внутренне может иметь `Ask`, но **в UI его нет**. На кнопку действует явное разрешение/запрет. Если запрос поступил, значок загорается независимо от блокирующей базовой политики. Если запрос остался без решения, он должен безопасно отклониться. При простом Allow через WebEngine API не объявлять устройство остановленным после revoke.

### 3.3. Temporary grant

- Зафиксировать время и точную привязку конкретного запроса Qt; при клике проверить валидность request + origin + tab + epoch.
- Никаких `StoreOnDisk`, постоянных grant и Always Allow в профильном state.
- Использовать Qt `AskEveryTime` как одну из проверяемых реализационных основ; само имя не появляется в UI. Режим имеет документированные нюансы; решить после тестирования реальных native permission types.
- Если тип API не поддерживает заранее предоставленный grant, до реального запроса иконка не должна обещать уже выданный доступ. Можно исследовать краткую «готовность к следующему запросу», но выпускать её только после анализа угроз и UX.
- Завершение при навигации/закрытии вкладки/истечении lease, а для действующих media stream — реалистичная процедура остановки/уничтожения view, если Qt не умеет подтвердить немедленную остановку.
- Политика запрета и безопасные глобальные предпочтения могут сохраняться. Разрешающий positive grant — нет.

### 3.4. Fix transaction correctness

- Исправить передачу boolean previous state в int mode для Undo.
- Привязать Undo, Reload и любые delayed actions к зафиксированному `SiteContext`, не `currentView`.
- Отделить write success, current effective state и needsReload.
- Тест: изменение политики в A→переключение на B→Undo должно вернуть A и никак не изменить B.

**Приёмка P0:** после перезапуска никакой ранее разрешённый сайт не получает чувствительную возможность автоматически; после временного разрешения/завершения lease политика возвращается к базовой; нет кросс-вкладочных изменений.

---

## 4. P1 — Постоянно видимая Control Bar и минимальный URL

**Файлы:** `qml/Main.qml`, `qml/AdaptiveAddress.qml`, `qml/ControlBarInline.qml`, `qml/ControlBarIcon.qml`, `qml/CapabilityPopover.qml`, `qml/ControlBarOverflow.qml`, `qml/CybouTheme.qml` и новые `qml/NavigationBar.qml`, `qml/CapabilityBar.qml`, `qml/CapabilityTooltip.qml`, `qml/InlinePermissionAction.qml`.

### 4.1. Layout

- Отдельная визуальная Control Bar, **не** общий ряд с Downloads и Menu; 11 существующих категорий видимы, добавить отдельные Ads и Trackers только после независимой backend-классификации (не рисовать фиктивные две кнопки на один переключатель).
- Предпочтителен отдельный тонкий ряд для возможностей под компактной навигацией: окно не жертвует функцией ради длинного URL. Рассмотреть однострочное размещение для широких окон и компактную 2-строчную адаптацию на узком размере 820 px; не превращать всё в `···` и не прятать активные запросы.
- Адрес сокращён до hostname / пиктограммы состояния, полный URL виден по клику/`Ctrl+L`. При потере фокуса/клике вне ввода адрес сворачивается и панель полностью видима. Система подсказок не теряет выбранный элемент на blur.
- Размеры исходно: навигация 36–40 logical px, capability row 34–38 px; DPI 100–200%; минимальные tappable areas около 34–40 px; проверить скриншотами, не навязывать цифры без фактического замера.

### 4.2. Interaction contract

- Hover: маленький tooltip по текущему key (`name`, default/actual policy, последний подтверждённый запрос, scope), без открытия тяжёлой общей карточки.
- Click: одна из двух допустимых прямых операций: смена политики или `grant` текущему pending request. Если необходим осмысленный выбор из нескольких опций, только небольшой anchored popover, никогда системный ASK dialog.
- Отмена / возврат в безопасный default — одна кнопка на видимом значке. Опасные изменения доступности JS могут требовать reload; показывать встроенный unobtrusive Undo/Reload, точно соответствующий исходному сайту.
- Цвет отвечает на policy (нейтральный/blocked/temporary allowance), а amber halo/pulse отдельно на `pending request`; `recent event` отдельная отметка, `actual activity` только при доказательстве.
- При смене вкладки, origin или `navigationEpoch` сразу инвалидировать старый hover, popup и события.
- UI не перехватывает события Chromium прозрачным оверлеем поверх viewport.
- Для доступности: keyboard/Tab, ARIA-like accessible names QML, tooltips, reduced motion, цвет + графема, FR/RU.

**Приёмка P1:** все реальные возможности доступны с главной панели с ≤1 кликом для простого действия; запрос микрофона виден сразу; hover никогда не показывает чужую карточку; Download и Menu независимы от размеров адреса.

---

## 5. P1/P2 — Наблюдаемость и правдивые события

**Файлы:** `RequestObserver.h`, `ControlBarController.*`, новые `src/activity/SiteActivityModel.*`, `src/activity/ActivityEvent.h`, `src/activity/ActivityAggregator.*`.

Единая событийная шина:

```text
WebEngine permissionRequested ──► PermissionCoordinator ──► ActivityAggregator
WebEngine popup/navigation ─────► Event adapters ────────► ActivityAggregator
URL RequestInterceptor ─────────► bounded lock-free/short lock queue ─► Aggregator
                                                                 ▼
                                                  SiteActivityModel (Qt GUI)
                                                                 ▼
                                                    Control Bar QML
```

Данные события: `timestamp, category, action, outcome, origin, topOrigin, profileId, tabUid? , source, confidence`.

- Не хранить сетевые тела, cookies, headers, query, path.
- Интерсептор Qt может быть вызван вне GUI thread; нельзя обновлять QML там и нельзя блокировать сеть тяжёлой синхронизацией/дисковыми правилами.
- Для HTTP request в Qt нельзя всегда достоверно связать событие с конкретной вкладкой. Отображать подтверждённую site-based attribution на основании `firstPartyUrl` / `initiator` с маркировкой уверенности, иначе профильную сводку.
- Блокированные запросы записывать как события (сейчас код выходит до sample), статистику разделять по фильтру и скоупу.
- В UI правило `Unknown` лучше ложной зелёной отметки.
- Тайминги: UI update при событии без фиксированной задержки 250 ms poll; coalesce burst в 16–100 ms без потери агрегатных счётчиков.

**Приёмка:** внешний запрос к заблокированному домену даёт ровно измеренное событие `Blocked`; UI не показывает request как active mic; разные вкладки не смешиваются там, где есть подтверждённая привязка.

---

## 6. P2 — Строгая сеть: HTTPS, реклама, трекеры

### 6.1 HTTP

- Сделать публичный HTTP `Denied` до явного разрешения.
- Текущий `httpsFirst=true`, `httpsOnly=false` недостаточен; `HTTPS-first` — апгрейд, а не гарантия отсутствия HTTP.
- На ошибке HTTPS — browser-owned ненавязчивый блок-экран/баннер, а не автоматический переход к HTTP. Один явный клик может дать временное исключение ровно для попытки/ограниченного контекста.
- Никакого host-wide HTTP fallback на весь процесс: удалить/переосмыслить `m_httpFallbackHosts`.
- Для loopback-dev и офлайн-тестов — явное исключение test-only, без открытия LAN в production по умолчанию.
- Показывать и управлять HTTP в сетевой части Control Bar только как реально применённой транспортной политикой.

### 6.2 Ads/Trackers

- `TrackerBlocker.h` перевести на подписанные или контролируемые версионируемые списки правил; определять категории ads/tracker отдельно, если классификация надёжна.
- В C++ policy: global defensive defaults immutable for website UI; temporary override bound to top origin/tab with expiration; после закрытия, навигации или окончания периода — вернуть глобальную фильтрацию.
- Обновление правил должно быть атомарным для interceptor; без регулярных сетевых скачиваний в hot path.
- Фильтровать `resourceType` и first-party/third-party корректно (public suffix / eTLD+1 там, где требуется), не считать `host.endsWith()` полноценной моделью сайтов.
- Тестовые сценарии: магазин с платежом, OAuth, SSO, embedded media, Google Meet, CDN; различать false positive и настоящую блокировку.
- Запрещено показывать «12 трекеров заблокировано на этой вкладке», если есть лишь профильная `atomic blockedCount`.

**Приёмка:** `ads` и `trackers` имеют честные отдельные состояния или объединены в одну реальную capability; временное послабление не изменяет глобальную настройку других сайтов и исчезает в установленный момент.

---

## 7. P3 — Риск-адаптивная политика и прокси (после рабочих P0–P2)

- Создать `RiskSignals` и `RiskDecision` с объяснимыми конкретными причинами: доказанный вредоносный домен из проверяемых источников, подозрительная схема перенаправлений, повышенные попытки получить чувствительные полномочия и т.п. Не делать «впервые увидел сайт» синонимом злонамеренности.
- Обязательно различать `KnownMalicious` и `Uncertain`; риск-решение не должно базироваться на одном непроверенном эвристическом признаке.
- При повышенном риске применять JS-off **до** начала документа. После уже выполненных скриптов требуется безопасная повторная загрузка/новый контекст; переключатель не делает выполненный ранее JS безвредным.
- Прокси — *отдельная исследовательская задача*. Qt WebEngine наследует proxy из Qt Network, штатной проверенной per-tab proxy политики в текущем коде нет. Нельзя менять application proxy на лету и утверждать, что только опасная вкладка ушла через прокси.
- Для действительно изолированной проксированной сессии рассматривать отдельный браузерный процесс с независимым сетевым профилем/контекстом и строгими запретами прямого выхода. Подтвердить route/DNS/WebRTC leak tests, fail-closed, isolation cleanup, IPC security. До доказательства этого — только блокирование/ограничение JS и capabilities без маркировки «изолирован».
- Не выключать просто JavaScript на всех незнакомых сайтах без причины: нарушится совместимость и пользователь перестанет доверять решениям браузера.

**Приёмка:** UI может объяснить конкретный risk signal, реальную заблокированную capability и доказательство применения; proxy label появляется только после подтверждённых leak-тестов.

---

## 8. Тестирование (обязательное после каждого шага)

### 8.1 Интеграционные тесты

- `--security-test`: `StoreOnDisk` removal / no persistent grants after process restart; explicit HTTP permission, lack of host-wide retention; JS-off before main-frame navigation; normal cookie/browser storage distinction; permission-expiry cases.
- `--controlbar-test`: 11+ visible capabilities, no hidden 3-button UI, address collapse on outside click, hover tooltip always correct, request color vs blocked state separate, DPI/window sizes 820×560/1024×768/1360×900/1920×1080, mouse & keyboard, FR/RU.
- `--smoke-test`: pending mic/geolocation notifications fixtures, timeouts, tab close/switch, top-origin vs iframe request, undo/reload original tab, real blocked network event, expired ads/tracker exception, privacy separation.
- Браузер после тестов не оставляет `ALLOW` для чувствительных типов в QSettings / списке persistent permissions.

### 8.2 Обязательные ручные проверки на реальном движке

1. Google Meet: просьба mic/camera подсвечивает только соответствующую capability, нет модального ASK, grant only current request, inactive or revoked accurately labeled.
2. Карты: геолокация, отказ/разрешение, процессный рестарт.
3. Онлайн-магазин: блокируемые рекламные/аналитические запросы, временное ослабление защиты только для checkout origin, автоматический rollback.
4. HTTP-only public site: заблокирован до явного разрешения; при следующем посещении снова заблокирован.
5. Cross-origin iframe: не выдаёт полномочия iframe по полномочиям top origin; не подменяет JS настройки основного документа.
6. Смена вкладки во время запроса и во время открытия popup; никакого действия по устаревшему context.
7. Сборки с актуальным Qt WebEngine: sandbox, сертификаты, профильность, отсутствие глобальных утечек.

**Важно:** изолированная test fixture на `127.0.0.1` использует HTTP; обновляя строгую политику, сохранить безопасный *test-only* механизм обхода, не ослабляя production.

---

## 9. Последовательность коммитов непосредственно в main

| № | Коммит | Состав | Gate |
|---:|---|---|---|
| 01 | `docs: define observable browsing vision and invariants` | PRODUCT_VISION, SECURITY_MODEL, CAPABILITY_CONTRACT, UX, README, ADR, ROADMAP | Больше нет двух противоречащих «источников истины» |
| 02 | `test: encode capability safety contracts` | Дополнить тестовые fixtures и исходный baseline | Документированные expected failures для будущих фич |
| 03 | `fix(security): remove persistent permission grants` | Profile policy, Controller, migration of old saved grants | После рестарта никакого persistent ALLOW |
| 04 | `feat(security): request-scoped permission coordinator` | PermissionCoordinator, lease, cancel/expire/grant | Нет модалок; запрос виден, по умолчанию denied |
| 05 | `fix(policy): scoped transactions and undo` | PolicyTransaction, original context, enum conversion | Cross-tab Undo не влияет на чужой сайт |
| 06 | `refactor(ui): extract compact address and capability chrome` | NavigationBar, CapabilityBar, distinct Download/Menu | Все важные иконки постоянно видимы |
| 07 | `feat(ui): direct action icons and event highlight` | click contract, tooltip-only hover, exact colors | Один клик, никаких stale popovers |
| 08 | `feat(activity): event-driven site activity model` | Request/permission events, bounded aggregation | Нет ложных claims и смешанных счётчиков |
| 09 | `feat(transport): explicit temporary HTTP allowance` | deny-until-opt-in, no host session allowlist | Повторный HTTP без нового consent запрещён |
| 10 | `feat(blocking): proper ads tracker controls` | Rule engine, categories, transient scoped override | Другие вкладки сохраняют защиту |
| 11 | `feat(risk): pre-navigation restricted mode` | Risk signals, JS-off pre-navigation, risk explanations | Risk state доказуем и тестируется |
| 12 | `spike(isolation): per-site proxy architecture` | Отдельный эксперимент, без ложных production promises | Решение только после route/leak tests |
| 13 | `test: end-to-end product contract and release gates` | Manual+automated tests, screenshots, performance | Чёткое описание реально реализованного v1 |

Всё делать **в main** небольшими последовательными коммитами без веток. Приоритет — локальные тесты (CI может появиться позже). Каждый коммит — работоспособный код или самостоятельная документация; крупные механизмы могут разбиваться на более мелкие технические коммиты.

---

## 10. Матрица критериев выпуска CYBOU v1

**MUST:**

- У пользователя на виду полный набор реализованных capabilities, а не три групповых кнопки.
- Значок реально реагирует на подтверждённые события страницы; policy и activity различимы без чтения длинных текстов.
- Разрешение чувствительной возможности не появляется автоматически, не сохраняется на диск и не выживает безусловно после выхода/нового визита.
- Прямое управление по одному клику в рамках честных ограничений API.
- Нет модальных ASK permission dialogs.
- HTTP по умолчанию не открывается без явно временной политики; нет silent fallback.
- Блокировки рекламы/трекеров включены; то, что UI показывает как *blocked*, действительно предотвратилось interceptor'ом.
- Тесты `--security-test`, `--controlbar-test`, `--smoke-test` проходят локально, а реальные ограничения перечислены в README.
- No misleading activity indication, no global protection-disable from per-site click.

**SHOULD:**

- Полезные site-level counters и классификация Ads/Trackers.
- Проверенные UX сценарии совместимости и временное ослабление фильтра для конкретного сайта.
- Полное описание risk rules, слабых сигналов и false positives.

**LATER / RESEARCH:**

- Автоматическая пер-сайтовая прокси-изоляция, мониторинг всех JS действий, мониторинг реального медиапотока (если технически возможно), обновляемый репутационный threat feed.

**Никогда не обещать:** «видим абсолютно всё», «отмена разрешения гарантированно остановила поток», «HTTP-прокси обеспечивает песочницу», «публичный браузер безопасен без обновлений Chromium».

---

## 11. Проверенные ограничения Qt (ссылки)

- Permission persistence `AskEveryTime / StoreInMemory / StoreOnDisk`: https://doc.qt.io/qt-6.10/qml-qtwebengine-webengineprofile.html
- `permissionRequested` и `grant/deny/reset`: https://doc.qt.io/qt-6.10/qml-webenginepermission.html
- Request info `firstPartyUrl`, `initiator`, `resourceType`: https://doc.qt.io/qt-6/qwebengineurlrequestinfo.html
- Interceptor задерживает запрос до возврата: https://doc.qt.io/qt-6/qwebengineurlrequestinterceptor.html
- Прокси через Qt Network application proxy, не per-tab: https://doc.qt.io/qt-6.8/qtwebengine-overview.html#proxy-support

Прежде чем утверждать безопасность, проверить точное поведение Qt 6.12 в установленной сборке пользователя, особенно персистентные permissions, iframe и media capture.
