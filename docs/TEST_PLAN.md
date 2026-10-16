# CYBOU — план проверки

**Правило:** статус PASS только после фактического запуска с сохранённым результатом. Этот документ описывает проверки, а не объявляет их пройденными. Ранее полученные результаты — в [VALIDATION](VALIDATION.md).

## Локальные команды

Windows Release (Qt 6.12 / MSVC 2022, C++20):

```powershell
cmake --build build --config Release --parallel 4
./build/Release/cybou.exe --security-test
./build/Release/cybou.exe --controlbar-test
./build/Release/cybou.exe --transport-test
./build/Release/cybou.exe --tracker-test
./build/Release/cybou.exe --surface-test
./build/Release/cybou.exe --smoke-test
```

Test organization и fixtures должны быть отделены от обычного профиля пользователя. Встроенные loopback HTTP fixtures разрешаются только тестовым запуском; production HTTPS strict сохраняется. После существенного изменения permission/view/profile lifecycle прогонять **все** смежные suites.

## Функциональная регрессия

| Направление | Что проверять |
| --- | --- |
| **Control Bar** | Все 11 иконок в **одной** строке на 820×560, 1024×768, 1360×900, 1920×1080; Ctrl+L/Escape не скрывают иконки, URL заполняет остаток ширины; separator перед Downloads/Menu |
| **Glass hover** | Наведение на Location → интерактивная карточка → переход курсора в неё → переключение на JavaScript без stale content; закрытие вне области; кнопки действуют, клик по странице доходит до Chromium |
| **Keyboard и accessibility** | Tab, arrows, Space/Enter, focus, accessible name/state; FR/RU, DPI 100/150/200%, reduced motion, contrast |
| **Native permissions** | Реальный request, отказ без pregrant, подтверждение правильного ID, stale ID/tab/profile/epoch, reset создаёт view без новой вкладки, independent grants, revoke и очистка permissions |
| **HTTP transport** | HTTPS strict, exact URL/query, single-use main navigation, same-origin/firstParty assets, cross-origin HTTP/ws deny, credential-URL deny, stale token, peer-tab isolation, reload/revoke/profile destruction |
| **Tracker-only exception** | Реальная blocked fixture до consent, passed tracker-rule после consent, отдельный OTR профиль без обычных cookies/cache, peer-tab isolation, нет HTTP exception mixing, stale token, reload/revoke/profile teardown; факт pass не означает успешную загрузку |
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
