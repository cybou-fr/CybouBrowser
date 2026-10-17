# CYBOU Cloud Services — Compatibility Matrix & Operational Guide

*Репозиторий: cybou-fr/CybouBrowser · Редакция: 17 октября 2026 · CYBOU Browser v0.8*

---

## 1. Обзор архитектуры

CYBOU Cloud реализован как автономный, безопасный модуль C++/QML без сторонних расширений, userscripts, фоновых демонов и закрытых SDK (согласно [ADR 0006](ADR/0006-no-third-party-extensions.md) и [ADR 0007](ADR/0007-selectable-cloud-services.md)).

### Компоненты системы
1. **`CloudProviderCatalog` (`src/cloud/CloudProviderCatalog.h/.cpp`)**:
   - Статический каталог поддерживаемых провайдеров, проверенных официальных HTTPS-адресов и ролевых карт.
   - Поддерживаемые экосистемы: `proton` (по умолчанию), `google`, `microsoft`, `nextcloud`, `icloud`, `none` (отключено).
   - Поддерживаемые роли: `mail`, `calendar`, `files`, `docs`, `sheets`, `presentations`, `meetings`.
2. **`CloudServiceManager` (`src/cloud/CloudServiceManager.h/.cpp`)**:
   - Хранение предпочтений: выбор глобального провайдера (`defaultProvider`), индивидуальные переопределения ролей (`roleOverrides`), пользовательский URL для Nextcloud (`nextcloudBaseUrl`).
   - Разрешение ролей с поддержкой каскадного наследования (`inherit`) и отключения отдельных сервисов (`disabled`).
   - Экспорт списка разрешённых сервисов в QML (`resolvedServices`, `mailUrl()`).
3. **Интерфейс настроек (`qml/CloudSettingsSection.qml`)**:
   - Интегрирован в `SettingsPanel.qml`.
   - Селектор экосистемы по умолчанию, поле Nextcloud URL с валидацией HTTPS, аккордеон тонкой настройки каждой из 7 ролей с кнопкой сброса к значениям по умолчанию.
4. **App Launcher на новой вкладке (`qml/CloudAppLauncher.qml` & `CloudAppTile.qml`)**:
   - Расположен в правом верхнем углу `NewTabPage.qml` над Alpine Dawn card, не нарушая центрированную геометрию карточки.
   - Кнопка прямого перехода «Почта» (✉ Courrier).
   - Сетка 3×3 открытия всплывающего окна (Dark Glass Popup) со списком доступных сервисов и кнопкой перехода в настройки.
5. **Навигация и безопасность**:
   - Запуск приложений осуществляется через стандартный `window.navigate(url)` в обычных вкладках `BrowserTab.qml`.
   - Применяются стандартные политики безопасности: HTTPS-first, блокировка трекеров, NavigationGuard (защита приватной сети от публичных сайтов), Control Bar permissions.

---

## 2. Каталог провайдеров и матрица сервисов

| Роль | Proton (Default) | Google Workspace | Microsoft 365 | Nextcloud Hub (Self-hosted) | Apple iCloud |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Mail** (Почта) | `mail.proton.me` | `mail.google.com` | `outlook.live.com/mail` | `/apps/mail` | `icloud.com/mail` |
| **Calendar** (Календарь) | `calendar.proton.me` | `calendar.google.com` | `outlook.live.com/calendar` | `/apps/calendar` | `icloud.com/calendar` |
| **Files** (Файлы) | `drive.proton.me` | `drive.google.com` | `onedrive.live.com` | `/apps/files` | `icloud.com/iclouddrive` |
| **Docs** (Документы) | `drive.proton.me/docs` | `docs.google.com/document` | `office.live.com/start/Word.aspx` | `/apps/richdocuments` | `icloud.com/pages` |
| **Sheets** (Таблицы) | `drive.proton.me/sheets` | `docs.google.com/spreadsheets` | `office.live.com/start/Excel.aspx` | `/apps/richdocuments` | `icloud.com/numbers` |
| **Presentations** (Презентации) | `drive.proton.me` (fallback) | `docs.google.com/presentation` | `office.live.com/start/PowerPoint.aspx` | `/apps/richdocuments` | `icloud.com/keynote` |
| **Meetings** (Встречи) | `meet.proton.me` | `meet.google.com` | `teams.live.com` | `/apps/spreed` (Talk) | `icloud.com` (fallback) |

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

### 3.4. Nextcloud Hub (Self-hosted)
- **Валидация адреса:** Принимаются только валидные HTTPS URL (например, `https://cloud.example.com`). Ошибочные схемы (`http://`, `file:`, `javascript:`) блокируются.
- **NavigationGuard:** Обращения внешних публичных страниц к локальным IP (RFC 1918) блокируются. Прямой переход пользователя на собственный локальный HTTPS-сервер Nextcloud выполняется штатно.

---

## 4. Верификация и тестирование

Реализация покрыта автоматическими тестами:
- **`CloudCoreTests.h` (в составе `cybou_tests --security-test`):**
  - `testDefaultProviderIsProton`: проверка провайдера по умолчанию.
  - `testRoleResolutionProton`: проверка всех 7 ролей для Proton.
  - `testRoleResolutionGoogle`: проверка всех 7 ролей для Google.
  - `testRoleResolutionMicrosoft`: проверка всех 7 ролей для Microsoft.
  - `testRoleResolutionNextcloud`: проверка относительных путей Nextcloud и кастомного base URL.
  - `testRoleOverride`: проверка переопределения отдельных ролей (например, Google для Mail при дефолтном Proton).
  - `testDisabledProviderAndRoles`: проверка отключения провайдера (`none`) и отдельных ролей (`disabled`).
  - `testResolvedServicesList`: проверка генерации списка сервисов для UI.
  - `testNextcloudUrlSanitization`: валидация пользовательского HTTPS URL Nextcloud.
- **Регрессионные тесты:**
  - `--surface-test` (окна, отрисовка, оверлеи) — PASS.
  - `--smoke-test` (навигация, сессии, профили) — PASS.
  - `--controlbar-test` (11 иконок, разрешения, геометрия Control Bar) — PASS.
