# CYBOU Cloud Services — текущий каталог и проверки совместимости

**Версия: 0.3.0-alpha.2.** Каталог, настройки и Cloud Glass 2.0 реализованы, но **не следует объявлять совместимость с аккаунтами поставщиков подтверждённой**, пока на конкретной сборке не пройдены реальные проверки входа и сценариев.

## Каталог в C++

| Экосистема | Роли и приложения в локальном каталоге |
| --- | --- |
| Proton (default) | Mail, Calendar, Drive, Docs, Sheets, Meet, дополнительные веб-ссылки Pass, Lumo, Wallet, VPN account, Authenticator info |
| Google | Gmail, Calendar, Drive, Docs, Sheets, Slides, Meet; Keep как standalone web link |
| Microsoft 365 | Outlook, Calendar, OneDrive, Word, Excel, PowerPoint, Teams; To Do/OneNote как ссылки |
| Infomaniak | Mail, Calendar, kDrive, Docs, kMeet |
| Zoho | Mail, Calendar, WorkDrive, Writer, Sheet, Show, Meeting |
| Nextcloud | Только при заданном пользователем HTTPS URL; функции зависят от установленных серверных apps |
| Дополнительные | Изменяемая версия локального каталога; фактический список — `src/cloud/CloudProviderCatalog.cpp` |

Формат: общий provider, плюс собственный provider для каждой роли (`inherit` / явный / `disabled`). Это обычные HTTPS-переходы, не API-клиенты и не SSO CYBOU.

## Что ещё проверять

| Проверка | Статус |
| --- | --- |
| Компиляция каталога и resolver | Исходники и тестовые сценарии присутствуют; актуальный лог PASS не приложен |
| Открытие официальных endpoints | Нет полного воспроизводимого test report |
| Proton / Google / Microsoft login и 2FA | **NOT_VERIFIED** |
| Infomaniak / Zoho / Nextcloud login | **NOT_VERIFIED** |
| Docs/Sheets/Files/Calendar workflows | **NOT_VERIFIED** |
| Meet, media permission, revoke в реальном звонке | **NOT_VERIFIED** |
| No background traffic до явного клика | **NOT_VERIFIED** |
| Privacy и отсутствие partner privileges | Требуются негативные тесты и runtime audit |
| FR/RU / keyboard / 820px / DPI | Полная матрица **NOT_VERIFIED** |

Открытие URL само по себе не подтверждает вход, хранение сессии, video capture, документы или безопасность. Не атаковать сайты партнёров без разрешения; использовать собственные test accounts и контролируемые fixtures.

[ADR 0007](ADR/0007-selectable-cloud-services.md) · [TEST_PLAN](TEST_PLAN.md) · [VALIDATION](VALIDATION.md).
