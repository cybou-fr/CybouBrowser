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
| Network | URL interceptor | tracker rules + постоянный HTTPS strict | сведения, HTTP opt-in/revoke | bounded profile sample; HTTP: один URL, отдельный профиль, 60 секунд |

Reload/Undo действуют только при совпадении исходной вкладки, полного URL и navigation epoch; смена контекста закрывает баннер. Request IDs, независимые PermissionLease и проверки SiteContext реализованы в C++. Typed C++ policy transactions остаются в roadmap.
