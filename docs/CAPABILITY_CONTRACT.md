# CYBOU — контракт возможностей панели

Этот документ описывает **то, что реально поддерживается**, а не план релиза. Расположение и взаимодействия определяет [CONTROL_BAR_UX](CONTROL_BAR_UX.md); новая безопасность и UX — [ROADMAP](ROADMAP.md).

**Состояния:** effective policy, pending request, granted permission, confirmed blocked event, recent event и unknown — независимые факты. `Allow` не доказывает работу камеры/микрофона. Событие перехватчика на профиле не подтверждает конкретную вкладку. Если нет достоверного API, UI говорит об ограничении.

| Кнопка | Источник и текущее действие | Область / честные ограничения |
| --- | --- | --- |
| **Camera** | Native `MediaVideoCapture` request; клик отвечает на конкретный pending request или сбрасывает контекст | Grant связан с документом; неизвестно, ведётся ли реальная запись |
| **Mic** | `MediaAudioCapture` / combined audio-video; действующий запрос, сброс | Combined request может относиться к двум кнопкам; нет датчика активности потока |
| **Screen** | Тип permission распознаётся; capture chooser не интегрирован | Unsupported request отклоняется; fake Allow не показывать |
| **Location** | `Geolocation` permission request, явное решение, сброс | Реальный location provider и точность не подтверждены |
| **Notifications** | Native permission request, явное решение | Desktop delivery не интегрирована |
| **Clipboard** | `ClipboardReadWrite`, pending/решение | Не каждый clipboard access наблюдаем |
| **JavaScript** | `settings.javascriptEnabled`, direct toggle, reload/Undo | Текущий view, политика по tab+origin; нет inventory DOM/JS |
| **Popups** | `onNewWindowRequested`, user gesture по умолчанию, allow/deny для новых окон | Скрытые внутренние попытки Chromium не подсчитываются |
| **Cookies** | Third-party cookie filter | Глобальная настройка профилей, **не per-site** |
| **Storage** | `settings.localStorageEnabled`, direct toggle, reload/Undo | Только localStorage, не полный IndexedDB/Cache/Service Workers control |
| **Network** | Interceptor, HTTPS strict, event categories; HTTP consent/revoke и tracker-only exception/revoke в интерактивной карточке | События и counts profile-scoped; отдельный tracker профиль без обычных cookies; ни полного adblock, ни независимых Ads/Trackers policies пока нет |

## Правила действий

- Permission выдаётся только в ответ на живой запрос для той же вкладки, профиля, requesting/top origins и navigation epoch; произвольного site allow нет. `AskEveryTime` остаётся внутренним механизмом Qt. Отсутствие ответа на pending request приводит к отказу.
- Сброс чувствительной capability без pending request может пересоздать текущий URL в новой `WebEngineView` без добавления вкладки.
- JS/storage/popups меняют поведение, а не переписывают уже исполненные скрипты; Reload/Undo требуют исходный tab+URL+epoch. Нельзя запускать Undo на другой вкладке.
- Сетевые temporary opt-ins действуют в отдельном контексте и могут пересоздать/закрыть документ; это UX-ограничение следует честно показывать в карточке, не утверждая безусловный бесшовный возврат.
- Настройку защиты всех профилей нельзя представлять как site-scoped действие. Недоступные Screen или полноценные Ads/Trackers controls не изображать активными.

Следующие изменения (iframe inheritance, verified media revoke, ad/tracker RuleEngine, ActivityAggregator) отмечены как **planned** в [SECURITY_UX_EVOLUTION](SECURITY_UX_EVOLUTION.md). Реальные локальные проверки: [VALIDATION](VALIDATION.md).
