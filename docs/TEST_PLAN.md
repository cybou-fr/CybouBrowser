# CYBOU — план проверки

**Правило:** статус PASS только после фактического запуска с сохранённым результатом. Этот документ описывает проверки, а не объявляет их пройденными. Ранее полученные результаты — в [VALIDATION](VALIDATION.md).

## Локальные команды

Windows Release (Qt 6.12 / MSVC 2022, C++20):

```powershell
cmake --build build --config Release --parallel 4
./build/Release/cybou_tests.exe --security-test
./build/Release/cybou_tests.exe --controlbar-test
./build/Release/cybou_tests.exe --transport-test
./build/Release/cybou_tests.exe --tracker-test
./build/Release/cybou_tests.exe --surface-test
./build/Release/cybou_tests.exe --smoke-test
```

Test organization и fixtures должны быть отделены от обычного профиля пользователя. Встроенные loopback HTTP fixtures разрешаются только тестовым запуском; production HTTPS strict сохраняется. После существенного изменения permission/view/profile lifecycle прогонять **все** смежные suites.

## Release gate: запрет сторонних расширений (план проверки)

Нормативный контракт — [ADR 0006](ADR/0006-no-third-party-extensions.md). После каждого изменения Qt WebEngine, CMake/deployment или партнёрской интеграции:

- проверить исходники и артефакты сборки на extension loader/install/update, `QWebEngineExtensionManager`, WebExtensions/CRX/Manifest packages, privileged userscript injection и неявные плагины;
- проверить отсутствие extension store, интерфейса side-loading, фоновых extension-процессов и скачивания исполняемых дополнений из браузера;
- проверить состав бинарной поставки: новые CLI/внешние исполняемые файлы, dynamic libraries, версии, лицензии, происхождение и необходимость; Qt/Chromium и системные зависимости учитываются отдельно;
- проверять, что «позаимствованные» возможности реализованы нативно кодом CYBOU и не добавляют скрытых привилегий партнёрам.

**Это критерии приёмки, а не заявление о уже выполненном аудите runtime.** Обычный web JavaScript не является разрешением установить привилегированное расширение.

## Cloud Services acceptance (planned, not run)

Нормативный [ADR 0007](ADR/0007-selectable-cloud-services.md). Для реализации проверить:

- Proton является default при отсутствии сохранённого выбора; Google, Microsoft 365 (Office 365), Infomaniak, Zoho, Nextcloud и `none` можно выбрать из списка; настройка не меняет search engine.
- После смены выбранного провайдера перестраиваются только будущие облачные ярлыки; существующие вкладки, история, cookies, авторизация, session restore и настройки permissions/HTTPS/tracker policy сохраняются.
- До клика по веб-сервису браузер не делает фонового запроса к его сайту; нет auto-login, prefetch, токенов авторизации в настройках или незаметных коммерческих redirects.
- Каждая роль Mail/Files/Calendar/Docs/Meet ведёт на правильный официальный HTTPS endpoint; отсутствующая в конкретном тарифе/сервере роль не выдаётся за доступную. Nextcloud endpoint задаётся пользователем, отклоняет не-HTTPS/credentials/опасные URL.
- Нет сторонних расширений, companion CLI и privileged injected scripts. Провайдер не получает исключительных разрешений и может быть отключён полностью.
- Регрессия Alpine Dawn/Control Bar: одна навигационная строка и **все 11 интерактивных значков неизменны**. Проверка FR/RU, клавиатуры, малой ширины и DPI; нормальный/приватный режим.
- Проводить отдельные реальные проверки входа и базовых действий в поддерживаемой Qt WebEngine версии для каждой официальной веб-экосистемы; статус `PASS/FAIL/SKIP` по продукту, а не по факту отображения landing page.

## Функциональная регрессия

| Направление | Что проверять |
| --- | --- |
| **Control Bar** | Все 11 иконок в **одной** строке на 820×560, 1024×768, 1360×900, 1920×1080; Ctrl+L/Escape не скрывают иконки, URL заполняет остаток ширины; separator перед Downloads/Menu |
| **Glass hover** | Наведение на Location → интерактивная карточка → переход курсора в неё → переключение на JavaScript без stale content; закрытие вне области; кнопки действуют, клик по странице доходит до Chromium |
| **Keyboard и accessibility** | Tab, arrows, Space/Enter, focus, accessible name/state; FR/RU, DPI 100/150/200%, reduced motion, contrast |
| **Native permissions** | Реальный request, отказ без pregrant, подтверждение правильного ID, stale ID/tab/profile/epoch, reset создаёт view без новой вкладки, independent grants, revoke и очистка permissions |
| **HTTP transport** | HTTPS strict, exact URL/query, single-use main navigation, same-origin/firstParty assets, cross-origin HTTP/ws deny, credential-URL deny, stale token, peer-tab isolation, reload/revoke/profile destruction |
| **Tracker-only exception** | Реальная blocked fixture до consent, passed tracker-rule после consent; действие по top-level origin в общем профиле с сохранением сессии и cookies (ADR 0005); владение привязано к вкладке-владельцу (TrackerExceptionRegistry); синхронное правдивое отображение во всех вкладках этого же origin; отсутствие переноса на другие origin; снятие исключения при отзыве или закрытии/навигации вкладки-владельца; факт pass не означает успешную загрузку |
| **Other surfaces** | Tab drag, unused strip window drag/double-click, bookmarks/history/downloads/settings drawers в workspace под header, обычный GlassToolTip у chrome, download review и PDF export |

## Новые release gates — запланированы, ещё не пройдены

1. **Iframe:** top origin A и requester iframe B; явный запрет родителя действует на вложенный request; отдельный requester deny; нормальный и приватный профиль; старый ID никогда не подтверждается.
2. **Реальные media:** mic, camera, combined audio/video, продолжительный видеозвонок, системные разрешения, background tab, sleep/resume, навигация и ручной отзыв; оценить UX и реальное прекращение capture. Не считать grant доказательством записи.
3. **Engine:** поддерживаемая версия Qt/Chromium, проверяемый sandbox runtime, небезопасные flags в release, регрессия после обновления, проверка подписанной доставки.
4. **Ad/Tracker RuleEngine:** реальная классификация по правилам, обновление/rollback, memory/CPU burst, ложные блокировки login/payment/video/CDN/CAPTCHA.
5. **Независимые site actions:** две вкладки одного origin и третья другого; Ads и Trackers разделены; context guard, restore defaults, Undo, no global HTTPS/permission relaxation.
6. **ActivityAggregator:** effective policy ≠ request ≠ grant ≠ confirmed block, profile sample не выдаётся за tab counter; память и время событий ограничены.
7. **NavigationGuard / Restricted browsing:** IDN, redirects, IPv4/IPv6 private network и DNS rebinding; JS-off *до* первого документа; негативные fixtures против ложных тревог.
8. **UI preservation:** одна строка с 11 иконками и интерактивным hover на всех размерах/DPI/языках после каждого функционального обновления.

Фактический результат каждого прогона фиксировать как PASS/FAIL/SKIP с commit SHA, платформой, Qt/Chromium runtime и ссылками на локальный лог, а ручные tests — отдельно. Не приписывать старому тестовому прогону результат новой реализации. Подробные изменения — в [SECURITY_UX_EVOLUTION](SECURITY_UX_EVOLUTION.md), текущая карта возможностей — [CAPABILITY_CONTRACT](CAPABILITY_CONTRACT.md).
