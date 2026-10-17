# CYBOU Cloud Services — Compatibility Matrix & Operational Guide

> **Версия продукта:** `0.3.0-alpha.1` — engineering alpha, не квалифицированный релиз. Старые тесты и названия `0.8` не доказывают безопасность текущей сборки. [VERSIONING](VERSIONING.md) · [RELEASE_CRITERIA](RELEASE_CRITERIA.md) · [SECURITY_VERIFICATION_FRAMEWORK](SECURITY_VERIFICATION_FRAMEWORK.md).

*Репозиторий: cybou-fr/CybouBrowser · Редакция: 17 октября 2026 · CYBOU Browser 0.3.0-alpha.1 (code prototype; compatibility NOT release-verified)*

---

**Status notice:** отдельные локальные прогоны и наличие API в Qt WebEngine не доказывают вход/2FA/видеозвонки на реальных аккаунтах всех поставщиков. Таблицы ниже описывают *заявленные точки входа и сценарии проверки*, не сертификацию совместимости или безопасности. Release GO/NO-GO определяется [SECURITY_VERIFICATION_FRAMEWORK](SECURITY_VERIFICATION_FRAMEWORK.md) и [RELEASE_CRITERIA](RELEASE_CRITERIA.md).

## 1. Обзор архитектуры

CYBOU Cloud реализован как автономный, безопасный модуль C++/QML без сторонних расширений, userscripts, фоновых демонов и закрытых SDK (согласно [ADR 0006](ADR/0006-no-third-party-extensions.md) и [ADR 0007](ADR/0007-selectable-cloud-services.md)).

### Компоненты системы
1. **`CloudProviderCatalog` (`src/cloud/CloudProviderCatalog.h/.cpp`)**:
   - Статический каталог поддерживаемых провайдеров, проверенных официальных HTTPS-адресов и ролевых карт.
   - Поддерживаемые экосистемы: `proton` (по умолчанию), `google`, `microsoft`, `infomaniak`, `zoho`, `nextcloud`, `apple`, `none` (отключено).
   - Поддерживаемые роли: `mail`, `calendar`, `files`, `docs`, `sheets`, `presentations`, `meetings`.
   - Полное соответствие возможностям провайдеров: исключены фиктивные службы (у Proton нет Презентаций, у Apple iCloud нет веб-встреч).
   - Проверка безопасности URL: метод `isVerifiedServiceUrl(url, customNextcloudHost)` проверяет точное соответствие хостов и путей официальным адресам.
2. **`CloudServiceManager` (`src/cloud/CloudServiceManager.h/.cpp`)**:
   - Хранение предпочтений: выбор глобального провайдера (`defaultProvider`), индивидуальные переопределения ролей (`roleOverrides`), пользовательский URL для Nextcloud (`nextcloudBaseUrl`).
   - Строгая валидация входных данных: `setDefaultProvider()` и `setRoleOverride()` отклоняют неизвестные идентификаторы и несовместимые с ролью провайдеры.
   - Фильтрация провайдеров для настроек: метод `providersForRole(role)` возвращает только те экосистемы, которые действительно поддерживают данную службу.
   - Санитизация Nextcloud URL (`sanitizeNextcloudUrl`): поддержка только HTTPS, удаление учётных данных (userinfo), query-параметров, фрагментов и замыкающих слэшей.
   - Устойчивость к повреждению настроек: проверка `schemaVersion == 1`, обработка синтаксических ошибок JSON и автоматический сброс к безопасным значениям по умолчанию (`proton`).
   - Быстрый статус почты: метод `isMailAvailable()` для условного отображения кнопки «Почта».
   - Разрешение ролей с поддержкой каскадного наследования (`inherit`) и отключения отдельных сервисов (`disabled`).
3. **Интерфейс настроек (`qml/CloudSettingsSection.qml`)**:
   - Интегрирован в `SettingsPanel.qml`.
   - Селектор экосистемы по умолчанию со всеми 7 провайдерами.
   - Поле настройки Nextcloud HTTPS-сервера с подсказкой и индикатором валидности.
   - Аккордеон тонкой настройки каждой роли с динамическими списками только поддерживаемых провайдеров.
4. **App Launcher на новой вкладке (`qml/CloudAppLauncher.qml` & `CloudAppTile.qml`)**:
   - Расположен в правом верхнем углу `NewTabPage.qml` над Alpine Dawn card, не нарушая центрированную геометрию карточки.
   - Кнопка прямого перехода «Почта» (✉ Courrier / Почта) — скрывается, если почта отключена или требует настройки.
   - Сетка 3×3 открытия всплывающего окна (Dark Glass Popup) со списком доступных сервисов и кнопкой перехода в настройки.
   - Закрытие popup при уходе со страницы или клике вне области.
   - Плитка `CloudAppTile`: поддержка клавиатурной навигации (Tab, Return, Space, фокусная обводка), подзаголовок провайдера, метка «À configurer» (Требует настройки) со ссылкой в настройки, двухстрочный перенос текста без обрезки.
5. **Навигация и безопасность**:
   - Запуск приложений осуществляется через стандартный `window.navigate(url)` в обычных вкладках `BrowserTab.qml`.
   - Применяются стандартные политики безопасности: HTTPS-first, блокировка трекеров, NavigationGuard (защита приватной сети от публичных сайтов), Control Bar permissions.

---

## 2. Каталог провайдеров и матрица сервисов

| Роль | Proton (Default) | Google Workspace | Microsoft 365 | Infomaniak | Zoho | Nextcloud Hub (Self-hosted) | Apple iCloud |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Mail** | `mail.proton.me` | `mail.google.com` | `outlook.live.com/mail` | `mail.infomaniak.com` | `mail.zoho.com` | `{base}/apps/mail/` | `icloud.com/mail` |
| **Calendar** | `calendar.proton.me` | `calendar.google.com` | `outlook.live.com/calendar` | `calendar.infomaniak.com` | `calendar.zoho.com` | `{base}/apps/calendar/` | `icloud.com/calendar` |
| **Files** | `drive.proton.me` | `drive.google.com` | `onedrive.live.com` | `kdrive.infomaniak.com` | `workdrive.zoho.com` | `{base}/apps/files/` | `icloud.com/iclouddrive` |
| **Docs** | `drive.proton.me/docs` | `docs.google.com/document` | `office.live.com/start/Word.aspx` | `kdrive.infomaniak.com/app/docs` | `writer.zoho.com` | `{base}/apps/richdocuments/` | `icloud.com/pages` |
| **Sheets** | `drive.proton.me/sheets` | `docs.google.com/spreadsheets` | `office.live.com/start/Excel.aspx` | `kdrive.infomaniak.com/app/docs` | `sheet.zoho.com` | `{base}/apps/richdocuments/` | `icloud.com/numbers` |
| **Presentations** | *Не поддерживается* | `docs.google.com/presentation` | `office.live.com/start/PowerPoint.aspx` | `kdrive.infomaniak.com/app/docs` | `show.zoho.com` | `{base}/apps/richdocuments/` | `icloud.com/keynote` |
| **Meetings** | `meet.proton.me` | `meet.google.com` | `teams.live.com` | `kmeet.infomaniak.com` | `meeting.zoho.com` | `{base}/apps/spreed/` | *Не поддерживается* |

---

## 3. Требования совместимости и WebEngine Gate

### 3.1. Proton
- **Требования веб-приложения:** WebCrypto API, JavaScript ES6+, localStorage, sessionStorage, Secure Cookies.
- **Статус в CYBOU:** Поддерживается в Qt WebEngine (Chromium core).
- **Media Permissions:** При использовании Proton Meet микрофон и камера запрашиваются через стандартный `QWebEnginePermission` диалог CYBOU и отображаются на Control Bar.

### 3.2. Google Workspace
- **Авторизация (OAuth/Google Sign-In):** Стандартная веб-навигация. CYBOU использует стандартный Chromium User-Agent без маскировки под сторонние расширения.
- **Meet & Drive:** WebRTC, WebSockets, File System Access API работают штатно.

### 3.3. Microsoft 365
- **Авторизация (Microsoft Entra / Live ID):** Многофакторная аутентификация (FIDO2 / WebAuthn, SMS, Authenticator) обрабатывается движком Chromium.
- **Office Web Apps (Word, Excel, PowerPoint):** Canvas/WebGL рендеринг документов поддерживается аппаратно.

### 3.4. Infomaniak & Zoho
- **Суверенитет данных:** Европейский провайдер Infomaniak (Швейцария) и Zoho Office Suite полностью поддерживаются с официальными точками входа и SSL-валидацией.

### 3.5. Nextcloud Hub (Self-hosted)
- **Удаление demo-ссылок:** Убраны все временные ссылки на `try.nextcloud.com`.
- **Конфигурация:** До задания пользователем персонального URL приложения отображают статус «À configurer» (Требует настройки), клик по плитке открывает панель настроек.
- **Валидация адреса:** Принимаются только валидные HTTPS URL (например, `https://cloud.mycompany.com`). Ошибочные схемы (`http://`, `file:`, `javascript:`) блокируются.
- **NavigationGuard:** Обращения внешних публичных страниц к локальным IP (RFC 1918) блокируются. Прямой переход пользователя на собственный локальный HTTPS-сервер Nextcloud выполняется штатно.

---

## 4. Верификация и тестирование

Реализация покрыта автоматическими тестами:
- **`CloudCoreTests.h` (в составе `cybou_tests --security-test`):**
  - `testDefaultProviderIsProton`: проверка провайдера по умолчанию.
  - `testNoFictitiousProtonPresentations`: проверка отсутствия вымышленных презентаций у Proton.
  - `testInfomaniakProvider`: проверка полной интеграции сервисов Infomaniak (kDrive, kMeet).
  - `testZohoProvider`: проверка ролей Zoho (Writer, Sheet, Show, Meeting).
  - `testRoleResolutionGoogle` & `testRoleResolutionMicrosoft`: проверка ролей Google и Microsoft.
  - `testRoleResolutionNextcloudUnconfigured`: проверка статуса `requiresConfiguration` при пустом URL.
  - `testRoleResolutionNextcloudConfigured`: сборка URL при настроенном HTTPS base URL.
  - `testNextcloudUrlSanitization`: санитизация HTTPS, удаление credentials, параметров и слэшей.
  - `testRoleOverride`: валидные переопределения отдельных ролей.
  - `testRejectInvalidInputs`: строгий отказ при попытке передать несуществующие ID или несовместимые роли.
  - `testDisabledProviderAndRoles`: проверка отключения провайдера (`none`) и отдельных ролей (`disabled`).
  - `testResolvedServicesList`: генерация упорядоченного списка для UI.
  - `testMailAvailableCheck`: проверка скрытия почты при отключении/ненастроенном провайдере.
  - `testCorruptedSettingsRecovery`: устойчивость к повреждённым файлам конфигурации.
- **Регрессионные тесты:**
  - `--surface-test` (окна, отрисовка, оверлеи) — PASS.
  - `--smoke-test` (навигация, сессии, профили) — PASS.
  - `--controlbar-test` (11 иконок, разрешения, геометрия Control Bar) — PASS.
