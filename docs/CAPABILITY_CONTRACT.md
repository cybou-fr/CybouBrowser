# Контракты возможностей

Политика и событие независимы. Allow означает временную выдачу права, не использование устройства. Pending всегда сопровождается requesting origin в tooltip. Срок ожидания 15 с, срок документа с доступом до 60 с после последней выдачи. Все события памяти ограничены 256 записями.

| Кнопка | Наблюдение/API | Default | Клик | Область / отзыв / ограничения |
|---|---|---|---|---|
| Camera | permissionRequested, MediaVideoCapture | недоступна | grant только pending; иначе сброс отказа и reload | документ вкладки; навигация/60 с; работа сенсора не наблюдается |
| Mic | MediaAudioCapture / MediaAudioVideoCapture | недоступен | как camera; комбинированный запрос выдаёт оба | объект исходного запроса; неизвестна активность |
| Screen | DesktopVideoCapture | недоступен | сведения | chooser не интегрирован, запрос deny |
| Location | Geolocation | недоступна | текущий запрос / сброс и reload | requesting origin, документ, 60 с |
| Notifications | Notifications | недоступны | текущий запрос / сброс и reload | desktop presentation не интегрирована |
| Clipboard | ClipboardReadWrite | недоступен | текущий запрос / сброс и reload | не все clipboard операции наблюдаемы |
| JavaScript | settings.javascriptEnabled | включён | toggle + reload banner | tab UID + origin, память; нет наблюдения DOM |
| Popups | newWindowRequested | только user gesture | временное allow/deny | вкладка; нет подсчёта подавленных Chromium попыток |
| Cookies | cookieStore filter | third-party deny | профильные сведения/контроль | явно глобальная настройка, не site privilege |
| Storage | settings.localStorageEnabled | включено | toggle + reload banner | localStorage, не IndexedDB/service workers |
| Network | URL interceptor | tracker rules + HTTPS default | сведения | bounded profile sample; не per-tab счётчик |

Reload/Undo действуют только при совпадении исходной вкладки, полного URL и navigation epoch; смена контекста закрывает баннер. Per-request IDs, независимые сроки grants и typed C++ transactions остаются в roadmap.
