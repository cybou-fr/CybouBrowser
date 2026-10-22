# ADR 0007 — выбор облачных сервисов

**В исходниках 0.4.0-alpha.1 реализованы:** `CloudProviderCatalog`, `CloudServiceManager`, `CloudSettingsSection.qml`, `CloudAppLauncher.qml` и локальные SVG. **Полное тестирование входов/2FA и безопасности на конкретной Windows сборке не проведено.**

## Модель выбора

- Базовый провайдер: **Proton**. Каталог включает Google, Microsoft 365, Infomaniak, Zoho, Nextcloud и дополнительные провайдеры; `none` отключает cloud launcher.
- Семь ролей: `mail`, `calendar`, `files`, `docs`, `sheets`, `presentations`, `meetings`.
- Для каждой роли: `inherit` (общая экосистема), явный provider override либо `disabled`; неподдерживаемые роли не подменяются.
- Индивидуальные overrides **сохраняются при переключении базового провайдера**. Явный reset очищает их; поисковик и сохранённые вкладки при этом не меняются.
- Пользовательский self-hosted Nextcloud требует HTTPS base URL и sanitation; доступные функции зависят от сервера.

## Фактический Cloud App Launcher

`NewTabPage.qml` размещает справа сверху ссылку **Courrier/Mail** на effective mail provider и кнопку девяти точек. По клику раскрывается отдельная панель Cloud Glass 2.0: локальные SVG-иконки, текущий набор сервисов и официальные веб-ссылки. Это не часть браузерной Control Bar.

Помимо role-based приложений, каталог содержит дополнительные **обычные веб-ссылки**: Proton Pass web, Lumo, Wallet, аккаунт/информация по VPN и Authenticator; Google Keep, Microsoft To Do, OneNote. Информационная ссылка на VPN не означает наличие встроенного VPN, ссылка на Pass — автозаполнения паролей. Никаких сторонних расширений или исполняемых помощников.

## Политика безопасности

- Сайты Proton, Microsoft, Google и остальные не получают privileged origins, специальных разрешений или исключений из HTTPS.
- Cloud catalog не читает почту, документы, session cookies и OAuth-токены; пользователь входит в обычном WebEngine.
- Никакого remote extension manager, side-loading и внешнего CLI ([ADR 0006](0006-no-third-party-extensions.md)).
- Партнёрская выручка не должна скрытно подменять целевые официальные URL.
- Поставщик по умолчанию — **удобство**, не allowlist и не гарантированная совместимость.

## Не выполненная квалификация

Login/2FA/SSO, редиректы, cookies/WebCrypto, запуск video meetings, Cloud UX/keyboard/DPI, Nextcloud на собственном сервере, отсутствие фоновых запросов до клика, real permission regressions.

[CLOUD_COMPATIBILITY_MATRIX](../CLOUD_COMPATIBILITY_MATRIX.md) · [TEST_PLAN](../TEST_PLAN.md).
