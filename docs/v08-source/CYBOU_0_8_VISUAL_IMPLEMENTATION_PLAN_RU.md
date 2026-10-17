> **HISTORICAL DESIGN / AGENT SOURCE — NOT CURRENT RELEASE STATUS.** CYBOU now declares **0.3.0-alpha.1** (unqualified alpha). Any "0.7", "0.8", tested or implemented statements below concern their dated snapshot and do not override [VERSIONING](../VERSIONING.md), [ROADMAP](../ROADMAP.md) or [SECURITY_VERIFICATION_FRAMEWORK](../SECURITY_VERIFICATION_FRAMEWORK.md).

# CYBOU Browser 0.8 — Dark Glass UI + Alpine Dawn

**Статус:** план реализации / coder-agent handoff, не отчёт о внесённых изменениях.  
**Дата проверки:** 2026-10-16.  
**Репозиторий:** `https://github.com/cybou-fr/CybouBrowser`, ветка `main`.  
**Проверенный HEAD:** `3d3922a592124feda0c5029df70fbbf377253e01`, версия приложения **0.7.1**.  
**При новом запуске агента:** проверить `git status`, текущий HEAD и изменения после указанного SHA. Работать небольшими, проверяемыми коммитами **в main**; не создавать ветки, не применять force push.  
**Технологии:** C++17, Qt 6.9+ Quick/Quick Controls/WebEngine, CMake; ранее проверялся Qt 6.12/MSVC x64.

## 1. Цель — что должно получиться

Реализовать утверждённый визуальный язык CYBOU: **весь browser-owned UI** — тёмно-синий, компактный, визуально цельный glassmorphism, включая вкладки, обе строки chrome, навигационные кнопки, адресную строку, Control Bar, overflow, tooltips, библиотеку, настройки и браузерные диалоги. Сторонние веб-страницы сохраняют исходный дизайн. На **новой вкладке** — согласованный пейзаж: снежные пики на рассвете в розово-оранжевом свете, озеро и облака, **без видимого солнца**, без водяных знаков, текста или элементов UI; поверх него — аккуратная стартовая композиция с **оригинальной** иконкой CYBOU из `assets/cybou.png`.

Две схемы адресной строки **УЖЕ реализованы в 0.7.1**. Не переписывать:

- **BROWSE** (нет фокуса): компактное отображение точного hostname, цветные capability-кнопки между адресом и Downloads, непроместившиеся функции в `···`.
- **EDIT** (фокус, Ctrl+L): **все** capability-кнопки и `···` скрыты; полный редактируемый URL получает максимально доступную ширину.

Следовательно, основной scope — **визуальная тема и новый фон**, а не повторная реализация Control Bar.

**Reference hierarchy:**
1. `CYBOU_Adaptive_Control_Bar_Reference_v0_7.png` (в пакете; при интеграции сохранить в `docs/design/`) — композиция, размеры и стеклянные элементы; концепт, не фактический screenshot.
2. `assets/cybou-alpine-dawn-1920x1080.jpg` (в пакете) — основной фон новой вкладки.
3. `assets/cybou-alpine-dawn-1920x1080.png` — lossless-версия того же изображения.
4. Фирменная эмблема только из **репозитория** `assets/cybou.png`, без её перерисовки или замены другой геометрией.

## 2. Факты аудита репозитория (чтобы не переделывать уже сделанное)

- `qml/Main.qml` по-прежнему содержит практически весь browser-owned UI, привязки состояния, кнопку новой вкладки, drawer, consent и диалоги. `ApplicationWindow.color = #F5F7FB` и palette светлая. Верхняя строка вкладок `#E5EBF3`, nav row белая; активная вкладка белая.
- `qml/AdaptiveAddress.qml` — существующее разделение label-only hostname / editable TextField; использует темно-синий текст на светлом фоне.
- `qml/ControlBarInline.qml` — реальная inline Control Bar и `widthBudget`, 11 capability, скрытие в EDIT, overflow, общий popover. **Не создавать вторую модель разрешений.**
- `qml/ControlBarIcon.qml` использует `QtQuick.Effects.MultiEffect` для colorization, так что сменить тему проще, чем переписывать SVG; текущие цвета/подложки ориентированы на светлый chrome.
- `qml/ControlBarOverflow.qml` — сейчас **светлое** меню (`#F5F7FB`), нуждается в полноценном dark-glass restyle.
- `qml/CapabilityPopover.qml` — уже тёмная карточка; надо сделать её согласованной с общими токенами. Нативный modal consent остаётся отдельным безопасным механизмом.
- Стартовая страница — `Rectangle` на `about:blank` в `Main.qml` с однотонным фоном, иконкой из `qrc:/assets/cybou.png`, заголовком, полем поиска и кнопками Favoris/Historique. Для картинки **отдельный WebEngineView не нужен**.
- `qml/SettingsPanel.qml` и большая часть drawer/dialog в Main по-прежнему светлые. `qml/NavButton.qml` с navy текстом на светлой hover-подложке.
- `src/ui/AddressDisplay.h` уже формирует hostname с ACE/punycode, IPv6/портами. Не заменять красивым, но потенциально обманчивым отображением адреса.
- `--smoke-test`, `--controlbar-test`, `--security-test` описаны в `docs/OMNIBOX_071_INTEGRATION.md` как ранее выполненные локальные проверки. **Это не значит, что мы повторно запускали тесты на данном аудите.**
- Последние security additions (HTTPS-only, download opening review) — сохранять без изменений поведения.

## 3. Дизайн-токены: единый источник истины

Ввести `qml/theme/CybouTheme.qml` как singleton (`pragma Singleton` + запись в QML module), либо аналогичный единый объект темы. Не дублировать hex-коды в каждом компоненте. Тема **одна — dark** для v0.8; светлую тему не реализовывать сейчас.

| Назначение | Предложение | Комментарий |
|---|---|---|
| navy base | `#0D2547` | Утверждённый бренд |
| navy deep | `#06172F` | Фон chrome и глубокие области |
| glass surface | `#D0122C50` | Alpha-over-navy с реальной проверкой контраста |
| glass hover | `#603C608B` | Только локальный hover; без яркой заливки всей строки |
| border highlight | `#497DA5CC` | Тонкий 1px outline |
| main text | `#F1F7FF` | Яркий белый с холодным оттенком |
| secondary text | `#B9CBE2` | Подписи и неактивные элементы |
| brand red | `#E11D2E` | Фирменный акцент, прогресс и опасные действия |
| allow | `#31E3A3` | Разрешено (достаточно светлый на navy) |
| deny | `#FF6077` | Запрещено |
| request | `#FFBD5C` | Текущий запрос — дополнительная подсветка, не подмена policy |
| recent | `#76B9FF` | Достоверное недавно наблюдавшееся событие |
| neutral | `#9CB2CE` | Спрашивать / неизвестно |
| unavailable | `#687C99` | Недоступно; отдельный визуальный shape |

Точные alpha/contrast уточнить на реальных screenshots Windows/DPI. Можно сохранить оттенок типа функции как вторичный акцент, но **цвет статуса имеет понятную семантику**: зелёный/красный/янтарный и т.д. Не интерпретировать разрешённый микрофон как «микрофон записывает».

Параметры: navbar/tabbar логически около 36px в текущем приложении; не увеличивать их без тестирования. Радиусы вкладок ~8–10px, адреса ~12px, карточек ~14–16px. Встроенный слот capability 36–40px можно уплотнить после измерений hit-target/accessibility. Иконки ~18–20px. Тонкие, не кислотные тени/подсветка; без бесконечно мерцающего chrome.

**Важно о glassmorphism:** alpha фона задавать в `color` (`#AARRGGBB`), **не** `opacity: 0.6` на всем родителе, иначе поблекнут надписи и иконки. Не обещать реальный backdrop blur через Chromium без технической проверки; для v0.8 достаточно убедительного матового glass (полупрозрачная заливка, градиент/тонкий бордер/блик). Локальный blur только после измерения GPU/cpu. Системные нативные FileDialog не обязаны менять нативный Windows-дизайн.

## 4. Рендеринг фона стартовой страницы

Вместо однотонного `Rectangle` в ветке `about:blank` реализовать `qml/NewTabPage.qml`:

1. `Image` заполняет viewport с `fillMode: Image.PreserveAspectCrop`, `asynchronous: true`, `cache: true`, `smooth: true`, source `qrc:/assets/cybou-alpine-dawn-1920x1080.jpg`.
2. Поверх изображения — **локальная затемняющая полупрозрачная градиентная маска** в центральной области для читаемости белого логотипа/формы поиска, без заметного убийства цветов рассвета.
3. Центр — **оригинальный** `Image.source: "qrc:/assets/cybou.png"` (НЕ генерировать SVG/другой щит), краткое название `CYBOU`, тонкая бело-сине-красная линия/слоган. Кнопки и форма поиска — glass с читаемыми подсказками.
4. Поиск использует существующую `window.navigate(text)` / сигнал `navigateRequested(text)`, а Favorites/History — существующую логику, включая запрет истории в private.
5. При 820×560 центр не должен обрезаться; сделать adaptive размеры/отступы с верхним и нижним safe area. Фон может crop'иться, но композиция изображения должна выглядеть корректно на ultrawide и 4:3; разумное положение focal point немного выше центра.
6. На другой `http/https/file:` вкладке фон **невидим** и не существует дополнительной веб-сети. Не вставлять изображение внутрь untrusted web content и не делать внешних image-запросов.
7. Приватный режим визуально отличим в tabs/chrome меткой и приглушённым/фиолетовым нюансом, но не создаёт ложное впечатление анонимности; сама картинка может оставаться.
8. Первый запуск должен показывать фон сразу, без белого flash; fallback цвет navy (`#06172F`). Существующий favicon/ярлыки и поиск рабочие.

Рекомендация по asset: production JPG 1920×1080 ~0.8 MB, а lossless PNG сохранить в design/reference. Изображение 1920×1080 в данном пакете **масштабировано из исходной генерации 1672×941**, не выдавать за независимо сгенерированные пиксели 1080p. Поддержка WebP возможна, но зависит от Qt imageformats deployment — в первый проход проще JPG. Проверить упаковку qrc и windeployqt.

## 5. Дизайн chrome, вкладок, адреса

- В `Main.qml` преобразовать `ApplicationWindow.color`, palette, верхнюю/нижнюю строки в общие theme tokens; проверить реальную прозрачность и читаемость на фоне нового start page и белых сайтов.
- В `NavButton.qml` перейти к белым/glass glyph, hover highlight, enabled/disabled и фокус-кольцу. Сохранить native window actions: move, edge resize, minimize/maximize/close; красный hover Close.
- Вкладки сделать тёмно-синими полупрозрачными pill/rounded rect; активная — чуть светлее и с тонким контуром; иконки вкладок брать из реальных favicons, favicon fallback — **только assets/cybou.png**. Закреплённые вкладки и mute/audible indicators сохраняют текущую геометрию.
- `AdaptiveAddress.qml`: BROWSE с точным hostname и тёмным glass background + белый текст; EDIT с полным URL и очень заметным, но сдержанным синим focus outline. Сохранить `Ctrl+L`/Escape/select/copy/paste/подсказки, без изменения `AddressDisplay::host()`.
- Не прятать Downloads, Menu, nav controls и окно при EDIT; **скрывать все capability и `···`**, как уже в 0.7.1.
- Разделить UI/модель: изменения цвета не должны приводить к сбросу tabs, policy или Repeater delegates.

### Баг-риск размеров (оптимизация сразу, но отдельным маленьким коммитом)

В `Main.qml` значение `ControlBarInline.widthBudget` рассчитано приблизительно: `navRow.width - 6*30 - 7*spacing -160`. Это предполагает фиксированные размеры nav-кнопок и нижнюю границу адреса, а значит при смене их дизайна может неверно считать видимые иконки. Заменить на **измеряемый layout budget**: известная ширина области navRow минус **фактическая** ширина Back/Forward/Reload/Home, Downloads/Menu, поля адреса с minimum, все margins/spacings. Не делать binding loop, не обновлять количество иконок из 250ms-сетевых событий, не скроллить в toolbar. Зафиксировать snapshot/debug metrics.

## 6. Control Bar, overflow, подробные карточки

- `qml/ControlBarIcon.qml`: текущий MultiEffect уже красит SVG; обновить палитру под темный фон и сделать читаемые светящиеся state/event badges. Целое семейство иконок не должно быть кислотным. `ASK` — спокойный нейтральный, `ALLOW` — зелёный, `DENY` — красный, `UNAVAILABLE` — приглушённый, `REQUEST` — 1–2 мягких импульса янтарным с учётом reduce-motion. `Recent` отдельно. Никаких 11 `?` на `about:blank`.
- `qml/ControlBarOverflow.qml`: заменить светлый background на темно-синий glass; добавить короткое заглавие/контекст сайта, каждая скрытая способность с иконкой + именем + подтверждённым статусом + событием; **не дублировать видимые**; scrollbar внутри, если нужно; не влиять на browser menu `☰`.
- `qml/CapabilityPopover.qml`: единые токены, тонкий border, blur-like shadow, readable text, кнопки без белых Basic defaults. `ALLOW/DENY` опасных разрешений остаётся явным действием; **hover не grant**.
- Не прикреплять popover к `WebEngineView` или чужому DOM, продолжать использовать browser-owned `Overlay.overlay`, в его bounds, не перекрывать область редактирования при Ctrl+L.
- Важный known limitation: screen source chooser пока нет; viewport-screen capture не отображать как рабочий переключатель. Network наблюдения profile-wide, а не точное «что отправила текущая вкладка»; Cookies filter глобальный для профиля.

## 7. Окна, настройки, меню, диалоги

- Сразу после chrome/start page — `SettingsPanel.qml`, `library` drawer в `Main.qml`, tooltip/menus, `suggestionsPopup`, `downloadOpenDialog`, `permissionDialog`, error/connection dialogs. Цель: единая тема во всём **browser-owned** UI, а не тёмная шапка с внезапно белыми диалогами.
- Ввести `GlassSurface.qml` (только подложка/обводка/тень), `GlassButton.qml` (hover/focus/disabled), необязательно `GlassDialog.qml` если он не нарушит имеющийся consent flow. Не переписывать Qt Controls Basic в собственный framework ради красоты.
- Consent modal: сохранить origin, понятные explicit allow/deny, строгий default/Focus на безопасном действии. Специализированный `downloadOpenDialog` должен сохранить security gate после restyle.
- Library/Settings: новые palette/font/divider/scrollbar, hover и акценты, без изменения data cleanup. Увеличить контраст важных объяснений про приватность.
- FR/RU строки и keyboard navigation; в v0.8 не начинать глобальный языковой рефакторинг, но **новые** строки перевести.

## 8. План коммитов в `main`

### P0 — Baseline + закрепление ассетов
- Обновить HEAD и не потерять коммиты других агентов. Проверить локальную сборку текущего HEAD и `--smoke-test`, `--controlbar-test`, `--security-test` с кодами возврата.
- Скопировать `cybou-alpine-dawn-1920x1080.jpg` в `assets/` и добавить в `qt_add_resources`/qrc. Оригинальный `assets/cybou.png` не трогать.
- Зафиксировать два внешних визуальных референса в `docs/design/` или ссылками, без подмены реальных screenshot. **Gate:** resource реально загружается, baseline известен.

### P1 — Настоящая тёмная тема
- Ввести Theme singleton и 1–2 общих glass primitives; перевести ApplicationWindow, tab row, nav row, nav buttons, address container/AdaptiveAddress.
- Рефакторить `Main.qml` **только настолько, насколько требует читаемость**; не перемещать stateful-подсистемы в процессе смены цветов.
- **Gate:** реальные скриншоты BROWSE/EDIT при 1360×900 и 820×560, нет белого/серого chrome.

### P2 — NewTabPage с альпийским рассветом
- Вынести стартовый `about:blank` в `NewTabPage.qml`, вставить background, затемнение, оригинальную иконку, glass search, быстрые действия. Сохранить логику поиска/истории/private.
- **Gate:** выглядит как reference, работает на 820×560, 1360×900 и ultrawide; мгновенно переключается на сайты без wallpaper overlay.

### P3 — Control Bar + overflow polish
- Темизировать ControlBarIcon, Overflow, CapabilityPopover; state/effect правдивые; 2-state semantics не меняются.
- Отдельным точечным шагом заменить приблизительный widthBudget на реальные измерения, не в циклических bindings.
- **Gate:** все 11 функций представлены напрямую/в overflow, hover/input работает, запросы не могут автоматически разрешиться.

### P4 — Остальные browser-owned surfaces
- Перевести SettingsPanel, Library Drawer, search suggestions, native/QML dialogs, download review, tab menus, notifications/status toast.
- **Gate:** при смене режимов и открытии dialog нет случайных белых прямоугольников; secure confirmation сохраняется.

### P5 — Visual QA, accessibility, perf, docs
- Screenshot matrix (реальный исполняемый браузер): NewTab-normal/private, BROWSE/EDIT wide+narrow, Overflow, Hover details, Settings, Downloads, Native permission consent, long URL, 125/150/200% DPI.
- Проверить high-contrast + reduced-motion, keyboard-only Tab/Ctrl+L/Enter/Escape, фокус, click-through, no QML warnings, no header white flash, no resize oscillation.
- Smoke/controlbar/security tests — фактические exit codes/логи. Пакетирование Release с `windeployqt` и офлайн запуск; проверить что embedded background доступен без сети.
- Обновить README, screenshot реальных окон, changelog. **Gate:** визуально совпадает с reference; тесты действительно PASS, без выдуманных результатов.

## 9. Test scenarios (минимум)

| ID | Сценарий | Приёмка |
|---|---|---|
| V01 | Новая вкладка в обычном окне | Именно Alpine Dawn, фирменная иконка; читаемый поиск; нет белой заливки |
| V02 | Новая приватная вкладка | Такой же фон допускается, но различимый private, без истории |
| V03 | Открыть реальный HTTPS-сайт | Тёмный chrome; чужая страница **не изменена**; background не просвечивает в ней |
| V04 | BROWSE | Точный hostname + цветные capability + downloads/menu, без широкой отдельной панели |
| V05 | Ctrl+L/edit | Полный URL с максимальной шириной, 0 capability, 0 `···` |
| V06 | 820×560 | Адрес читабелен, скрытые controls доступны в `···`; нет перекрытий |
| V07 | Длинный IDN/IPv6/URL | Без ошибки hostname/copy/paste/omni focus |
| V08 | Hover/click permission | Только правдивый policy и событие; keyboard Escape, карточка внутри окна |
| V09 | Permission request | Никаких auto-grant по hover; origin/предупреждение читаемы |
| V10 | Privacy profiles | Нет загрязнения normal профиля permission/history |
| V11 | HTTPS-only + download review | Функции остались рабочими после dark restyle |
| V12 | Keyboard/focus/reduced motion | Не ухудшились навигация и accessibility |
| V13 | DPI 1/1.25/1.5/2 | Размеры, клики, текст, иконки/поповеры не ломаются |
| V14 | Offline/deployment | JPG грузится из qrc, нет внешних запросов за wallpaper |
| V15 | `--smoke-test`, `--controlbar-test`, `--security-test` | Все действительно запускались, exit 0, логи и реальные screenshots |

## 10. Что не делать

- Не переделывать backend permissions/профили/сетевой interceptor/HTTPS-only/download security ради UX.
- Не возвращать плавающую панель и position picker.
- Не добавлять дополнительные белые/light темы, AI, пароли, автообновления или новые privacy-функции в этой задаче.
- Не маскировать hostname, не пропускать punycode/port/протокол в реальном редактируемом URL.
- Не делать непрерывного агрессивного glow или дорогостоящего живого blur над WebEngine.
- Не менять цвет содержимого сторонних страниц и не добавлять фон как веб-ресурс.
- Не утверждать прохождение сборки/тестов, если они не запускались.

## 11. Формат отчёта coding-agent

1. Конечный SHA `main` и перечень маленьких коммитов.
2. Список изменённых файлов и обоснование UI-разделения.
3. Полная команда сборки и фактические exit codes трёх suite.
4. Реальные screenshots перечисленных режимов; указать размеры окна/DPR.
5. Честный статус V01–V15 PASS/FAIL/BLOCKED.
6. Реальные замеченные проблемы (в т.ч. производительность и blur), которые перенесены на следующий релиз.

**Первое практическое задание агенту:** сделать P0 + P1 + P2 и показать **два настоящих скриншота CYBOU** — новую вкладку с рассветом и любую обычную HTTPS-страницу с новым navy glass chrome. До этого не тратить время на тотальный restyle всех диалогов.
