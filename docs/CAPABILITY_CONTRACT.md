# Контракты возможностей

Политика и событие независимы. Allow означает временную выдачу права, не использование устройства. Pending всегда сопровождается requesting origin в tooltip. Срок ожидания 15 с, срок каждого grant до 60 с; самый ранний deadline уничтожает WebEngineView и заменяет её пустой. Все события памяти ограничены 256 записями.

| Кнопка | Наблюдение/API | Default | Клик | Область / отзыв / ограничения |
|---|---|---|---|---|
| Camera | permissionRequested, MediaVideoCapture | недоступна | grant только pending; иначе сброс отказа и новая view на том же URL | документ вкладки; навигация/60 с; работа сенсора не наблюдается |
| Mic | MediaAudioCapture / MediaAudioVideoCapture | недоступен | как camera; комбинированный запрос выдаёт оба | объект исходного запроса; неизвестна активность |
| Screen | DesktopVideoCapture | недоступен | сведения | chooser не интегрирован, запрос deny |
| Location | Geolocation | недоступна | текущий запрос / сброс через новую view на том же URL | requesting origin, документ, 60 с |
| Notifications | Notifications | недоступны | текущий запрос / сброс через новую view на том же URL | desktop presentation не интегрирована |
| Clipboard | ClipboardReadWrite | недоступен | текущий запрос / сброс через новую view на том же URL | не все clipboard операции наблюдаемы |
| JavaScript | settings.javascriptEnabled | включён | toggle + reload banner | tab UID + origin, память; нет наблюдения DOM |
| Popups | newWindowRequested | только user gesture | временное allow/deny | вкладка; нет подсчёта подавленных Chromium попыток |
| Cookies | cookieStore filter | third-party deny | профильные сведения/контроль | явно глобальная настройка, не site privilege |
| Storage | settings.localStorageEnabled | включено | toggle + reload banner | localStorage, не IndexedDB/service workers |
| Network | URL interceptor | tracker rules + постоянный HTTPS strict | категории/причины, HTTP и tracker opt-in/revoke | bounded profile sample; исключения: отдельный профиль, один документ, 60 с; tracker только HTTPS |

Reload/Undo действуют только при совпадении исходной вкладки, полного URL и navigation epoch; смена контекста закрывает баннер. Request IDs, независимые PermissionLease и проверки SiteContext реализованы в C++. Typed C++ policy transactions остаются в roadmap.

## Следующий контракт UX и защиты (planned; не реализован)

Текущие `policy / pending / blocked / recent` не следует смешивать с фактической активностью сенсора. Grant camera/mic не гарантирует «идёт запись». По умолчанию нейтральная закрытая capability — не ошибка или произошедшая атака. Hover должен оставаться полноценной **интерактивной стеклянной панелью**, не краткой подсказкой.

| Контроль | Следующее проверяемое поведение | Что запрещено утверждать |
| --- | --- | --- |
| Camera/Mic/Location | Показать срок действующего grant, предупреждать о последствиях истечения; улучшение без внезапного обрыва лишь после real-media/revoke API proof | Что grant означает активную запись или что можно безопасно отозвать работающий stream без проверки |
| Requests из iframe | Запрет top-level origin распространяется на nested requester; показываются оба origin; конкретный request ID обязательный | Что посещение сайта или разрешение для родителя автоматически разрешает iframe |
| Network (Ads/Trackers) | Две *backend-подтверждённые* категории и независимые временные действия **в существующей Network hover-карточке** | Что старый единый tracker suffix list уже даёт полноценный adblock или отдельные per-tab счётчики |
| Журнал | Ограниченные по времени confirmed events с явным scope profile/tab/unknown | Что `firstParty` одного сетевого запроса точно определяет tab |
| Навигация | Локальные проверки IDN/URL/redirect/private-network, при реальном риске restricted mode и JS-off before-load | Что неизвестный сайт опасен или что off-the-record profile анонимен |

**Реализованный узкий промежуточный шаг:** `TrackerExceptionLease` разрешает известные tracker-list запросы одной HTTPS-страницы на срок до 60 с только в отдельном off-the-record профиле, с countdown и revoke в Network glass-панели. Старые cookies/cache не наследуются; reload/expiry уничтожают контекст. Нельзя объявлять это независимыми Ads и Tracker категориями. **Следующий этап** расширяет уже реализованное поведение, а не проектирует исключения с нуля.

**Существующие ограничения сохраняются:** Screen chooser не интегрирован; Notifications presentation не готова; cookies глобальны по нескольким профилям; Storage контролирует localStorage, не полный инвентарь IndexedDB/Service Workers; Network наблюдает только доступные ему HTTP(S)/ws события. Новые таблицы — acceptance criteria, а не shipped API. Подробнее: [SECURITY_UX_EVOLUTION](SECURITY_UX_EVOLUTION.md).
