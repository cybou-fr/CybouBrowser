# CYBOU — модель безопасности

> **Версия продукта:** `0.3.0-alpha.1` — engineering alpha, не квалифицированный релиз. Старые тесты и названия `0.8` не доказывают безопасность текущей сборки. [VERSIONING](VERSIONING.md) · [RELEASE_CRITERIA](RELEASE_CRITERIA.md) · [SECURITY_VERIFICATION_FRAMEWORK](SECURITY_VERIFICATION_FRAMEWORK.md).

**Источник правил безопасности.** Текущая реализация описана ниже, планы отделены и находятся в [SECURITY_UX_EVOLUTION](SECURITY_UX_EVOLUTION.md). Технические подробности см. в [ARCHITECTURE](ARCHITECTURE.md).

## Граница доверия

Любая веб-страница и встроенный iframe недоверен. Browser-owned UI принимает событие `QWebEnginePermission`, но разрешение выдаёт только C++ `PermissionCoordinator` для конкретного ещё действительного native request. Контекст `SiteContext` содержит слабые ссылки на view/profile, `tabUid`, top origin, requesting origin и navigation epoch; resolve проверяет совпадение контекста и ID. Фоновые, неподдерживаемые, невалидные и устаревшие запросы отклоняются.

Профили используют `AskEveryTime` как **внутреннюю политику Qt**, без сохранения положительных разрешений origin на диск и без пользовательского ASK-диалога. Отсутствие реакции заканчивается отказом. Controller не может выдавать pregrant посредством `queryPermission`. Новые политики поведения JS/localStorage/popups хранятся в памяти в рамках вкладки и origin, старые `controlPolicies` удаляются. Обычные browser data (история, cookies, session, storage) от этого не исчезают.

## Жизненный цикл разрешений

Каждому запросу присваивается идентификатор, который кнопка фиксирует при нажатии; отпускание не должно разрешать другой запрос. Разрешения привязываются к документу и контексту. При завершении/отзыве контекста текущая реализация может уничтожать `WebEngineView`, а не независимо останавливать активный media stream. **Grant не доказывает физическую запись**, а удаление записи о permission не доказывает прекращение уже идущего потока без проверки движка.

**Текущий статус:** в C++ реализован общий EffectivePermissionPolicy для top origin и requesting origin, есть записанные локальные тесты iframe. Это **не заменяет** полноценной adversarial/runtime-проверки на текущем candidate, hardware/OS и всей применимой OWASP-матрицы. См. [THREAT_MODEL](THREAT_MODEL.md), [TEST_PLAN](TEST_PLAN.md).

## Транспорт и фильтрация

Production HTTPS strict не имеет глобального разрешающего opt-out. HTTP по явному согласию может открываться только для **точного адреса** в отдельном off-the-record профиле; same-origin HTTP subresources с подходящим `firstParty` допускаются только в соответствующем временном контексте. Повторная навигация/отзыв завершают исключение; оно не наследует обычные cookies/cache и не позволяет странице автоматически повторить переход в обычном профиле. TLS errors отклоняются; WebSocket на незащищённой схеме не становится автоматически разрешённым.

Нынешний `TrackerBlocker` использует ограниченный список доменов. Реализован **tracker-only** opt-in в Network hover-карточке: исключение действует строго в рамках top-level origin в профиле (сохраняя сессию и cookies), привязано к времени жизни и origin вкладки-владельца (`TrackerExceptionRegistry`), правдиво отображается во всех вкладках этого же origin и не переносится на другие origin. Отзыв или уход вкладки-владельца снимает исключение (см. [ADR 0005](ADR/0005-origin-scoped-tracker-exceptions.md)). HTTP и tracker exceptions не объединяются. Это ещё не полноценный раздельный ads/tracker rule engine.

`RequestObserver` фиксирует ограниченные metadata (origin, first-party origin, resource type, result и reason), без path/query/headers/body. Атрибуция относится к **профилю**, не к достоверно определённой вкладке. Разрешённый запрос не равен доказанной загрузке ресурса.

## Запрет расширений и доверие к зависимостям

**Сторонние browser extensions запрещены как класс**, включая CRX/WebExtensions/MV3, side-loading, extension manager/store, privileged userscripts и встроенные «партнёрские» расширения. Коммерческий договор и цифровая подпись не повышают доверие к стороннему исполняемому коду. Встроенный browser chrome должен оставаться под контролем исходников CYBOU. См. обязательный [ADR 0006](ADR/0006-no-third-party-extensions.md).

Необходимый Qt/Chromium runtime, системные библиотеки и исполняемый сайтами JavaScript — **другие границы доверия**, которые существуют независимо от запрета расширений. Их следует минимизировать, изолировать и обновлять. Никаких необязательных внешних CLI/фоновых агентов в поставке без отдельного решения владельца проекта и анализа происхождения, лицензий, прав, обновлений и угроз. Правила фильтрации являются данными, а не исполняемым кодом.

## Граница облачных сервисов (первоначальная реализация существует; не release-qualified)

Будущий выбор облачного провайдера **не формирует доверенные сайты**: Proton, Google Workspace, Microsoft 365, Infomaniak, Zoho и Nextcloud получают **те же** WebEngine permissions, HTTPS и network-фильтры, что остальные сайты. Каталог не перехватывает почту, файлы, учётные данные, cookies или OAuth-токены; не запускает сторонний native executable, не устанавливает расширений и не делает фоновый запрос к выбранному провайдеру до явной навигации.

Сохранённый выбор provider ID — локальная настройка UI, **не передача аккаунта**. Собственный Nextcloud URL допускает только валидный HTTPS origin, введённый пользователем; он не становится privileged origin. Контракт: [ADR 0007](ADR/0007-selectable-cloud-services.md); extension ban: [ADR 0006](ADR/0006-no-third-party-extensions.md). Эти инварианты следует проверять до выпуска, функционал существует частично в коде, но перечисленные release invariants ещё не доказаны как полный набор.

## Что не гарантируется

Нельзя заявлять о полном антифишинге, мониторинге каждого действия JavaScript/DOM, активной записи только из grant, полной блокировке рекламы или независимой network/process/proxy изоляции. Separate WebEngine profile означает разделение данных профиля, не гарантирует иной renderer/process/network route. При выпуске требуется проверять actual Qt/Chromium runtime, sandbox, патчи и реальные media/iframe сценарии.

**Будущие работы:** [SECURITY_UX_EVOLUTION](SECURITY_UX_EVOLUTION.md); **проверки:** [TEST_PLAN](TEST_PLAN.md), [VALIDATION](VALIDATION.md).
