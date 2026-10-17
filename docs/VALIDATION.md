# CYBOU — зарегистрированная локальная проверка

**Назначение:** краткий индекс реально сохранённых результатов. Сведения относятся к указанным версиям и снимкам разработки, а **не к независимому security audit**. Изменение документации не означает повторного запуска тестов.

## Исходная среда

Windows / MSVC 2022 / Qt 6.12.0. Записанные runtime identifiers: Qt WebEngine 6.140.0, Chromium 140.0.7339.225, runtime-reported security patch 152.0.7977.83. Эти строки не удостоверяют отсутствие актуальных уязвимостей.

## Зафиксированные результаты предыдущих локальных прогонов

| Область | Ранее записанный результат | Материал и ограничения |
| --- | --- | --- |
| **Windows Release / C++20** | PASS, Release build и security suite | Build records предыдущего агента; нет независимой сборки в этом документальном проходе |
| **Основная Control Bar** | PASS, размеры 820×560 / 1024×768 / 1360×900 / 1920×1080, URL editing, 11 buttons inline | [Observable browsing](design/observable-browsing/) и локальные логи; исходные версии без glass hover являются историческими |
| **Native PermissionCoordinator** | PASS: identity checks, dual-clock monotonic deadline, request queue (`MaxQueuedRequests = 16`), active tab priority, atomic cancellation on `forgetView`/`documentLoaded`/`revokeAll`, independent grants, view replacement | Тестовые наборы `--security-test` и `--controlbar-test` пройдены на 100% локально; действительные hardware capture и iframe не проверены |
| **Glass hover Control Bar** | PASS, карточка появляется при hover, остаётся доступной при входе курсора, меняется для соседней иконки | [glass-hover.stderr.log](design/glass-hover/glass-hover.stderr.log), [визуальные captures](design/glass-hover/); реальная DPI-интеграция требует отдельного gate |
| **HTTP temporary context** | В прежнем отчёте PASS: exact-address consent, cross-origin rejection, отдельный off-the-record профиль, смена tab/reload/revoke в ранее проверенной реализации | Отчёт этапа `feat(transport)`, visuals в [transport](design/transport/); не доказывает proxy/process isolation |
| **Chrome and surfaces** | PASS: tab drag, title gestures, GlassToolTip, drawers под header при тестовых размерах | [surfaces.stderr.log](design/glass-surfaces/surfaces.stderr.log) и [скриншоты](design/glass-surfaces/); Windows multi-monitor manual testing отдельно |
| **Smoke / downloads / PDF** | PASS в соответствующих записанных integrated harness run | Статус ограничен протестированными fixtures |
| **CYBOU Cloud Hardening** | PASS: Nextcloud self-hosted, Infomaniak, Zoho, input validation, corrupted settings recovery, dynamic role filtering, keyboard nav, overflow prevention | Тестовые наборы `--security-test` (14 тестов CloudCoreTests), `--surface-test`, `--controlbar-test`, `--smoke-test` пройдены со статусом exit code 0 |
| **RuleEngine & Ads/Trackers (S1.1 + S1.2)** | PASS: версионированный RuleEngine (версия 1), разделение Ad vs Tracker, иммунитет к CDN/платежам/SSO/CAPTCHA, независимые исключения для рекламы и трекеров в `TrackerExceptionRegistry`, Network hover-карточке и QML | Все 5 тестовых наборов `--security-test`, `--tracker-test`, `--controlbar-test`, `--surface-test`, `--smoke-test` пройдены со статусом exit code 0 |

**RuleEngine и независимые Ads / Trackers (S1.1 + S1.2):** Реализован `RuleEngine` с типизацией правил (Реклама vs Трекеры), контрольной суммой целостности и иммунитетом к ложным срабатываниям (Stripe, PayPal, Cloudflare CDN, jsDelivr, unpkg, Google/Microsoft SSO, reCAPTCHA, hCaptcha). В `TrackerExceptionRegistry` и карточке Network добавлены независимые временные исключения отдельно для рекламы и трекеров с гранулярным отзывом (Undo) без закрытия вкладки и потери сессии. Локальные тесты `runTrackerLeaseTests` и `--tracker-test` пройдены на 100%.

**CYBOU Cloud Services (ADR 0007 & Cloud Hardening):** Полная реализация каталога (Proton, Google, Microsoft, Infomaniak, Zoho, Nextcloud, Apple iCloud), удаление demo-ссылок Nextcloud с переходом на персональный HTTPS сервер пользователя и статус «À configurer», строгая валидация входных параметров C++, динамические списки в QML, доступность с клавиатуры и адаптивный оверфлоу вкладок. Все 14 специализированных тестов `CloudCoreTests` пройдены на 100%.

**Tracker-only exception (ADR 0005):** Реализовано исключение по top-level origin в общем профиле с сохранением сессионного контекста (cookies, storage); владение привязано к вкладке через потокобезопасный `TrackerExceptionRegistry`. Локальный прогон `--tracker-test` пройден со статусом PASS (exit code 0).

## Известные пробелы

- Запрет на top origin и проверка requester origin для стороннего iframe могут расходиться; нужна cross-origin regression.
- Реальный mic/camera/combined capture, прекращение потока, работа после sleep/resume и продолжительные звонки не проверены.
- Отзыв permission/temporary context в текущем коде способен разрушить документ и прервать работу.
- Персональные иконки могут смешивать brandColor с семантикой policy; нельзя выдавать grant за реальную hardware activity.
- Малый suffix-list и tracker-only opt-in не равны полноценной раздельной рекламе/трекерам; network events относятся к профилю, не гарантированно к вкладке.
- Linux/macOS distribution, runtime sandbox и безопасность обновления движка, NavigationGuard/RestrictedBrowsing и proxy/process isolation не подтверждены.

## Правила дальнейшей фиксации

Каждый результат должен содержать дату, фактический commit SHA, версии движка, команду, exit code, лог, ручные замечания и проверенные размеры/DPI. В [ROADMAP](ROADMAP.md) менять `planned` на `implemented/tested` только после соответствующего кода и фактической проверки. Новые сценарии — в [TEST_PLAN](TEST_PLAN.md); ограниченный tracker context — в [ADR 0004](ADR/0004-temporary-tracker-exceptions.md).

**Редакторский проход:** обновлены только документы; никакие исходники, бинарники, тестовые harness или журналы с первичными результатами не изменены.
