# CYBOU Browser 0.7.1 — revised implementation specification

**Дата аудита:** 2026-10-16  
**Репозиторий:** https://github.com/cybou-fr/CybouBrowser  
**HEAD, проверенный удалённо:** `9a809aaacad28ed587aadcd0d8d8d3e48b6fc4a8` — `Add opt-in strict HTTPS transport policy and network security checks`  
**Стек по репозиторию:** Qt 6.9+, Qt Quick/QML, Qt WebEngine, C++17, CMake; Windows/Qt 6.12.0/MSVC 2022 — текущая документированная конфигурация.  
**Статус:** исправленный план для coding-agent. Аудит исходного кода и документации удалённо; локальная сборка/запуск Windows-тестов в рамках этого аудита **не выполнялись**.

## 0. Решение, имеющее приоритет

Этот документ **заменяет/уточняет** старую UX-часть `CYBOU_BROWSER_0_7_ADAPTIVE_OMNIBOX_FULL_SPEC_RU.md` в случае противоречий. Архитектуру безопасности CYBOU 0.6 сохранять.

Единая строка навигации:

- **BROWSE** (`address.activeFocus == false`): `[Back][Forward][Reload][Home] [site indicator + точный hostname, обрезанный визуально при нехватке ширины] [видимые capability icons][··· только если есть скрытые icons] [Downloads][Main menu]`. Поле адреса может быть компактным; остальная ширина достаётся Control Bar. Никаких плавающих элементов над веб-страницей.
- **EDIT** (`address.activeFocus == true`): `[Back][Forward][Reload][Home] [полный редактируемый URL на всю оставшуюся ширину] [Downloads][Main menu]`. **Скрыть ВСЕ capability-иконки и `···`.** Поле адреса становится максимально широким. Режим не должен завершаться во время ввода, выделения или показа подсказок.
- **NARROW BROWSE**: сократить число видимых capability-иконок, **не** ширину адреса ниже согласованного минимума; спрятанные иконки доступны в `···`, только в BROWSE. Порядок иконок фиксирован. Меню только скрытые, без дублей.
- **НЕТ** четырёх док-положений, position picker, переключателя положения, лишней третьей панели, всплывающего окна выбора позиции. Настройки старой позиции можно оставить миграционно-неактивными для сохранения совместимости, но не показывать в UI.

## 1. Новые факты из свежего `main`

1. `qml/Main.qml` остаётся монолитным (~1043 строки), основная nav row примерно с 656-й строки: `addressContainer`, `TextField id: address`, затем Downloads и главное Menu. Существующий `ControlBar` создаётся отдельно на viewport (`anchors.fill: parent`) около 822-й строки.
2. `qml/ControlBar.qml` (136 строк) отвечает за прежний floating rail, четыре позиции, `Flickable`, hover/pinned card, position picker и тестовую геометрию. Эти части **не переносить** в новую архитектуру.
3. `src/controlbar/ControlBarController.{h,cpp}` содержит `QAbstractListModel`, origin policies, native permission events, request/blocked events и bounded network sample. Сохранить один источник истины. `CapabilityModel.h` содержит 11 capabilities.
4. `qml/ControlBarIcon.qml` сейчас использует светлые SVG с постоянной обводкой `#E7F2FF` и маленькие цветные кружки: непосредственно в светлом browser chrome это будет плохо читаемо. Нужен реальный state-driven tint/аккуратная новая иконография и контраст.
5. `qml/CapabilityPopover.qml` — действующая подробная карточка с native permissions, JS/popups, cookies и ограничениями наблюдения. Нельзя создавать конкурирующий backend или разрешать device access по одному hover.
6. `qml/ControlBarHarness.qml` всё ещё проверяет четыре позиции и picker. Заменить эти проверки, сохранив тесты настоящих permissions, JS, storage, popup, network, privacy.
7. **Новый HEAD `9a809aa`** добавил `httpsOnly`, `src/security/NetworkPolicy.h`, `src/security/SecurityTests.h`, расширил `RequestObserver` и `Main.qml`. Теперь дополнительно запускать `--security-test` и не нарушать opt-in HTTPS-only при миграции омнибокса.
8. `docs/CONTROL_BAR_INTEGRATION.md`, `docs/controlbar-source/*`, `SettingsPanel.qml` и `README.md` пока документируют прежнюю rail. Переобозначить как historical / superseded и обновить активную документацию после реализации.

## 2. Архитектура v0.7.1 — минимально необходимое разделение

Сделать маленькие UI-компоненты и **не раздувать** `Main.qml` новой логикой:

- `qml/AdaptiveAddress.qml` (или эквивалент): `displayHost`, `rawUrl`, `editing`, редактирование URL, clear/escape/enter, связь с suggestions. Сохранить существующий backend поиска и bookmark внутри поля. Если extraction рискует сломать omnibox — на первом шаге сделать `Text` + `TextField` прямо в `addressContainer`, выделить позднее.
- `qml/ControlBarInline.qml`: чистое представление модельных строк, видимые иконки, overflow-кнопка, фиксированный порядок, width budget, keyboard focus. Использовать `controller.model`; без собственного списка разрешений и без QSettings.
- `qml/ControlBarOverflow.qml`: легковесное меню **только скрытых** rows текущей модели; цвет/состояние/недавний запрос, выбор строки открывает **существующую** `CapabilityPopover`. Не переносить сюда browser main menu.
- `qml/CapabilityPopover.qml`: оставить **один** хост подробной карточки для видимых иконок и overflow; внутри root/`Overlay.overlay` расположение рассчитывать от `mapToItem` без обрезания `header`.
- `qml/ControlBarIcon.qml`: визуальное представление policy + отдельное событие, никаких `?` на `about:blank`, доступное клавиатурное имя.
- `qml/Main.qml`: привязки к `currentView`, `ControlBarController`, всплывающим подтверждениям; удаление прежнего floating `ControlBar` со страницы и сохранение existing `behaviorDialog`/native consent.
- `src/controlbar/*`, `src/browserstore.h`, `src/security/*`: по умолчанию **не менять**. Лишь минимальные исправления при воспроизводимом баге, с отдельным тестом.

Схема владения:

```text
Browser Main / currentView / activeProfile
      ├─ active URL (real, canonical) ──> browser backend, history, navigation
      ├─ AdaptiveAddress (DISPLAY ≠ EDIT, no URL mutation when displaying)
      └─ ControlBarController (single source of truth)
                  └─ CapabilityModel (11 items, stable keys)
                         ├─ ControlBarInline (visible prefix)
                         └─ ControlBarOverflow (hidden suffix)
                              └─ SINGLE CapabilityPopover / consent workflow
```

## 3. Критические UX/безопасностные правила

### 3.1. Точный адрес

- **Никогда** не присваивать `address.text = shortHost` для косметического отображения. `address.text` в EDIT всегда содержит полный фактический URL. Compact label — отдельный `Text`, привязанный к `currentView.url`. Полный URL не должен теряться при Tab/Enter, Ctrl+L, Ctrl+C, Ctrl+V, back/forward, hover и switch tab.
- Для HTTPS показывать значок соединения, но **не** «сайт надёжен». HTTP явно помечать как незашифрованное соединение; при включённом HTTPS-only не маскировать заблокированную навигацию.
- В компактном виде показывать **hostname с поддоменом** и нетипичный порт; длинный hostname может визуально обрезаться с ellipsis, но на копирование/редактирование это не влияет. Не пытаться вычислять eTLD+1 вручную. IDN/похожие символы/IPv6/`localhost`/`file:`/`about:blank` проверять специальными тестами; не выдавать фальшивый хост.
- Ctrl+L: получить фокус и выделить **полный** URL. Один клик на compact display — реальный TextField получает фокус. Enter выполняет текущую семантику navigate/suggestions. Escape отменяет **только незафиксированный ввод** (не останавливает загрузку без необходимости).
- На смене вкладки во время EDIT нельзя оставлять чужой raw URL или подсказки. При миграции сохранить исправленную в 0.6 синхронизацию адреса с текущей вкладкой.

### 3.2. Детерминированная ширина

- Сохранить две browser header-строки по 36 logical px и existing `NavButton` 28 px, пока реальные скриншоты не покажут, что нужна коррекция.
- Минимальная полезная ширина адреса в BROWSE: ориентир **160 px**; если окно шире, `Layout.fillWidth` отдаёт ему остаток после максимального числа поместившихся icons. В EDIT Control Bar имеет нулевую effective width; не оставлять невидимого Layout spacing или пустого placeholder.
- `visibleCount` считать из **входного budget**: доступной ширины nav row минус известные фиксированные controls, margins, spacing, `minAddressWidth`, ёмкость overflow и слоты icons. Не выводить `visibleCount` через `addressContainer.width`, если `addressContainer.width` уже зависит от `visibleCount` — это circular binding.
- Если весь набор из 11 иконок помещается: `overflow` отсутствует. Иначе он появляется и содержит только скрытые позиции. На 820 px гарантировать: адрес, Downloads и main Menu не перекрываются; при необходимости оставить 0 иконок и `···`.
- Резервные ширины input targets/spacing и padding считать в **logical px**; реальный test DPI 1.0, 1.25, 1.5, 2.0. После открытия overflow зафиксировать набор hidden items до закрытия, чтобы меню не прыгало, если пришёл request/event.

### 3.3. Фокус и меню

- `editing` основывать на действительном фокусе TextField. **В EDIT ни одной icon и ни `···`.** Изменения событий/политик не переводят EDIT в BROWSE.
- Нажатие `···` возможно **только в BROWSE**; меню и карточки получают фокус независимо от TextField. Никакого автоматического скрытия menu из-за network/model refresh.
- В hover-card: открытие ~170 ms, задержка закрытия ~220 ms при переходе мыши к карточке; click pin; Escape закрывает текущий popup первым, а затем возвращает браузерный фокус. Не перехватывать клики веб-страницы за пределами popup и кнопок.
- Доступность: значок + текстовое состояние, keyboard Tab/Arrow/Enter/Space/Escape, high contrast и reduced motion. Не делать основной смысл зависимым только от цвета.

### 3.4. Достоверность security

- Всегда отделять policy (ALLOW/DENY/ASK/UNAVAILABLE/RELOAD) от event (permission requested, blocked, network observed); зелёная камера ≠ камера пишет; amber ≠ произошла утечка.
- `camera/mic`: ASK/DENY для будущих запросов, explicit grant только на конкретный запрос; завершение уже активного потока при изменении политики не обещать.
- `screen`: Unsupported пока нет рабочего chooser; не показывать работающий toggle.
- `cookies`: политика **глобальна для профилей**, а не per-site; никаких фальшивых per-site counts.
- `storage`: только localStorage setting (не IndexedDB, Cache Storage, service workers), с reload semantics.
- `network`: только bounded profile-scoped sample; никаких claims об объёме/содержимом персональных данных и точной атрибуции ко вкладке.
- **HTTPS-only (новое)**: preserve `SettingsPanel`, `NetworkPolicy`, interceptor behavior для HTTP, ws, localhost/LAN, credentialed URL; обновить тесты и UI-копирайт. Это не кнопка permissions на текущем этапе.

## 4. Порядок маленьких коммитов непосредственно в `main`

**P0 — Current HEAD & baseline gate:** проверить свежий HEAD, status/diff, убедиться в Qt 6.12.0/MSVC; собрать Release; `--smoke-test`, `--controlbar-test`, `--security-test`, сохранить exit codes и build logs. **НЕ** портить рабочую базу ради быстрого скриншота. Закрепить actual baseline + snapshots. Референс UI — только концепт, не доказательство текущего кода.

**P1 — Foundation & BROWSE/EDIT layout:** создать `ControlBarInline` и встроить в единую nav row между адресом и Downloads. Удалить page overlay. BROWSE с видимыми disabled/real icon state, EDIT без icons/overflow. Создать честный compact display hostname отдельно от raw URL. Реализовать `Ctrl+L` и обычный click. Тестовые снимки 1360×900 и 820×560 обоих состояний. **Gate:** нет QML binding loops / flicker; корректно работают input/suggestions.

**P2 — Responsive overflow:** выделить единый список `visibleKeys/hiddenKeys` на основе model и budget, реализовать `ControlBarOverflow`; only hidden items; click/hover/focus; на `about:blank` без 11 '?' и без ложных activity. Полный список доступен при 820px. **Gate:** один controller, нет дубликатов UI/скрытых действий.

**P3 — Shared details & consent:** перенести единственный CapabilityPopover из page overlay в доверенный browser popup host, переустановить anchors/mapToItem; mouse-out, click pin, keyboard, selected row survives `dataChanged`, handles resize and active-tab changes; сохранить `behaviorDialog`, native permissions confirmation и stale-request cancellation. **Gate:** никаких grants при hover, ни одного клика не теряет web view.

**P4 — State icon polish:** runtime tint самой иконки, state shapes/badges/recent activity, disabled state at about:blank, clear contrast on light bar, restrained animation (<200ms), reduced motion, FR/RU strings. Не создавать glow на каждом network event. **Gate:** реальные скриншоты и клавиатурная доступность.

**P5 — Test harness & docs:** удалить/архивировать старые тесты `position picker/four placements`, заменить assertions BROWSE/EDIT/overflow/focus/hostname/blocked UI, build DPI matrix, документировать новые файлы; удалить `ControlBarPositionPicker.qml` из CMake и исходников после проверки references. Старое `ui/controlBar/position` оставить как неиспользуемый migration key либо удалить отдельной доказуемо безопасной миграцией. **Gate:** три набора тестов PASS, screenshots, build logs, no new QML warnings.

**P6 (отдельно, не блокировать 0.7.1 visual milestone)**: cross-origin iframe permission fixture; dedicated third-party cookie fixture, single-page consent queue, better security update strategy, cross-window policy propagation, performance profiling of `ControlBarController::refresh` 250ms timer, Chromium patch/updater/release gates. Не выдавать 0.7.1 за production secure browser.

## 5. Test matrix — обязательные сценарии

| ID | Test | Обязательный результат |
|---|---|---|
| A01 | 1360 px, blur | Порядок `[address][icons][optional overflow][Downloads][Menu]` |
| A02 | 1360 px, mouse focus | `visibleKeys=[]`, overflow hidden, адрес стал максимально широким |
| A03 | Ctrl+L | Точное полное URL выделено; copy/paste не ломает адрес |
| A04 | 820 px, blur | Hostname читаем, скрытые возможности доступны через overflow |
| A05 | 820 px, focus | Ни icons, ни overflow, Downloads/Menu остаются |
| A06 | 11 icons fit | Overflow отсутствует, все icons доступны |
| A07 | 0 icons fit | Только overflow (BROWSE), address minimum сохранён |
| A08 | dropdown | Только скрытые rows в исходном порядке, без дубликатов |
| A09 | network/model refresh while menu open | Меню не прыгает и не закрывается, фокус стабилен |
| A10 | current tab changes | URL, site status и permissions переключаются без stale data |
| A11 | about:blank and file: | Нейтральный/ограниченный UI; нет 11 `?`; никаких фальшивых grants |
| A12 | HTTPS, HTTP, userinfo, punycode/IDN, IPv6, port | Отображение безопасно и не подменяет истинный URL |
| A13 | long path/query URL and suggestions | Raw URL сохранён, suggestions/Enter/Escape соответствуют старым сценариям |
| A14 | hover → details → mouse-out | После задержки popup закрывается, usable pointer corridor |
| A15 | click pin, Escape, click outside | Управление закрытием и keyboard focus без intercept page clicks |
| A16 | permission request & origin switch | Нет implicit grants и stale queued requests |
| A17 | JS/storage/popups policy | Применение после явной reload, per-origin isolation сохранены |
| A18 | Cookies/network activity | Scope соответствует фактам: profile-wide, bounded, sanitized |
| A19 | HTTPS-only ON/OFF (HTTP/ws/localhost/LAN/userinfo) | Security-test и обычная навигация работают, policy не ослаблена |
| A20 | private windows | History/policies/events изолированы; обычные настройки не потеряны |
| A21 | browser regressions | Tabs drag/pin/mute, downloads, PDF, local files, zoom, suggestions PASS |
| A22 | 100/125/150/200% DPI and min size | Нет наложения, эскизная геометрия и клавиатурные targets корректны |
| A23 | FR/RU, reduced motion, contrast, keyboard | Доступно без мыши/цвета/анимации |
| A24 | closing popovers, modal permission, fullscreen | Нет неподходящих наложений и потери фокуса |

Для тестов измерять `headSha`, `navWidth`, `fixedWidth`, `addressWidth`, `editing`, `visibleKeys`, `overflowKeys`, `overflowVisible`, `downloadX`, `menuX`, `actualDpr`, `currentTabId`, `displayHost`, `rawUrl`. **Геометрическая проверка:** при прочих равных `addressWidth(EDIT) > addressWidth(BROWSE)` и `ControlBarWidth(EDIT) == 0`. Контент попапов не выходит за window и не перекрывает критичные контролы. В логах сохранять коды завершения.

## 6. Условие готовности

- Код **совместим с фактическим HEAD**, не стирает HTTPS-only и другие изменения после 0.6.
- Навигационный chrome остаётся 2 × 36 logical px; поверх страницы нет плавающей панели.
- Реальные permissions/JS/popups/cookies/storage/network используют прежний controller и ограничения; hidden/visible entries показывают одно и то же.
- **EDIT = максимальная ширина и полный URL; BROWSE = короткий hostname + иконки + только нужный overflow.**
- Нет ложного статуса веб-камеры/хранилища, непонятных `?` на пустой странице, переходов в неверный origin.
- Локальные `--smoke-test`, `--controlbar-test`, `--security-test` и DPI-сценарии выполнены coding-agent с зафиксированными результатами; НЕ писать PASS без фактического запуска.
- Агент приложил реальные screenshots Browse/Edit/Overflow/Details/Private и список SHAs маленьких коммитов в `main`; никаких visual-reference изображений вместо скриншотов приложения.

## 7. Отчёт агента

Предоставить: HEAD до/после, список точных изменений, команды сборки и тестов + exit codes, test matrix A01–A24 PASS/FAIL/BLOCKED, actual DPR, реальные скриншоты, подтверждённые ограничения Qt, новые security/regression риски. При появлении новой версии `main` повторить diff и обновить предположения, не делать force push. Если baseline не собирается — сначала восстановить существующую работоспособность.

---

Исходная спецификация с визуальным референсом: `CYBOU_BROWSER_0_7_ADAPTIVE_OMNIBOX_FULL_SPEC_RU.md` (обязателен UX intent; этот документ уточняет работу на HEAD и технические риски). Активные исходники: `qml/Main.qml`, `qml/ControlBar.qml`, `qml/ControlBarIcon.qml`, `qml/CapabilityPopover.qml`, `qml/ControlBarHarness.qml`, `src/controlbar/*`, `src/browserstore.h`, `src/security/*`, `CMakeLists.txt`, `qml/SettingsPanel.qml`, `docs/CONTROL_BAR_INTEGRATION.md`.
