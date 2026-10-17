# CYBOU — развитие безопасности и UX

**Статус: инженерное предложение, не отчёт об уже выполненных функциях.** Рабочий статус конкретных возможностей — в [ROADMAP](ROADMAP.md); проверенные ранее результаты — в [VALIDATION](VALIDATION.md). Актуальные UI и API-контракты — [CONTROL_BAR_UX](CONTROL_BAR_UX.md), [CAPABILITY_CONTRACT](CAPABILITY_CONTRACT.md).

## Цель и инварианты

Создать браузер, в котором человек видит реальные попытки страницы и ограничения и управляет ими непосредственно, без постоянного доверия и навязчивых диалогов. **Сохранить существующие 11 индивидуальных контролов в одной строке навигации и интерактивные стеклянные hover-панели.** Адрес занимает оставшееся место. Не добавлять toolbar rows, не прятать кнопки в группы, не заменять карточки краткими tooltip.

**Новый жёсткий инвариант владельца проекта:** ни при каких партнёрских условиях не добавлять сторонние расширения браузера, Chrome/WebExtensions/CRX runtime, side-loading или исполняемые userscripts. Можно исследовать лучшие функции расширений и реализовывать их **своим кодом**; вся дополнительная native executable supply chain должна быть минимальной и проверяемой. См. [ADR 0006](ADR/0006-no-third-party-extensions.md).

Приоритет — доказуемая блокировка опасных действий без неожиданных потерь документа/сессии; любая «активность», категория угроз и область действия должны иметь подтверждённый backend-источник.

## P0. Permissions: iframe, media UX, WebEngine

**Iframe deny consistency.** Обнаружен разрыв: пользовательский deny сохраняется для top origin, а `ControlBarController::shouldDeny()` проверяет origin native requester. Для чужого iframe это разные ключи. Ввести единый `EffectivePermissionPolicy`: запрет верхней страницы наследуется любым вложенным requester, дополнительный requester deny тоже действует. Решение по grant требует прежние сильные проверки ID, профиля, tab UID, navigation epoch, валидности QWebEnginePermission и foreground. UI различает top/requesting origin. Gate: HTTPS top A / iframe B, запреты A и B, смена context до клика, normal/private.

**Media session lifecycle.** Сегодня отзыв может сопровождаться завершением документа, что обрывает конференции, формы и просмотр. Предлагается проверить реальные mic/camera/combined AV, возможности Qt для прекращения media stream, поведение при навигации и sleep/resume. Выбор механики отзыва и UX остаётся за владельцем продукта. Не вводить постоянные разрешения или незаметную передачу прав другой странице. Gate: длительный звонок, явный отзыв, отсутствие утечек и нежданного обрыва.

**Engine/security update discipline.** Проверять реальную версию Qt WebEngine и Chromium security patch, sandbox в runtime, release flags, небезопасные опции запуска. Добавить тесты после каждого обновления, надёжную поставку и политику обновлений. Номер версии или наличие теста сами по себе не подтверждают отсутствие уязвимостей.

## P1. Network protection, direct control, truthful events

**RuleEngine.** Разделить Ad/Tracker/Other **только по реально применённому правилу**. Предусмотреть версионированные snapshot-списки, дешёвую проверку в interceptor, проверку происхождения/целостности, атомарную активацию/rollback и тесты ложных блокировок на login/payment/video/CAPTCHA/CDN. Обычный suffix-list и текущее временное исключение не являются полноценным adblock. Не утверждать cosmetic filtering, если его нет. Не отправлять URL на внешние серверы по умолчанию.

**Site-scoped controls.** Сегодня существует узкое tracker-only исключение с пересозданием одной HTTPS-страницы в отдельном off-the-record профиле, ручным revoke и завершением временного контекста. Оно сохраняет данные обычного профиля изолированными, но не является процессной изоляцией; потеря привычной сессии и закрытие документа — UX-ограничения. *Следующая задача* — независимые временные Ads и Trackers политики с точным `tab UID + profile + top origin + navigation epoch`, безопасным Undo, правилом восстановления default на navigation/close/revoke. Добавить действия **в существующую интерактивную Network-карточку**. Отключение Ads не отключает Trackers, HTTPS strict или разрешения устройств. Не использовать глобальный toggle из карточки сайта.

**ActivityAggregator и цвета.** Разделить `EffectivePolicy` (доступен/закрыт/защищён/неизвестен) и `ObservedEvent` (актуальный request/confirmed denied/recent). Сейчас Allow камера получает красный capability brandColor, что конфликтует с red denied. Состояния: нейтральный default, янтарный pending, зелёный temporary grant, красный подтверждённый блок, бирюзовый включённый фильтр; обязательны формы/подписи для доступности. Ни один grant не объявлять активной записью. Memory-bounded events без path/query/headers/bodies; scope `tab`/ `profile`/ `unknown`. Пока только профильный network observer, per-tab counters недопустимы.

## P2. Доопасная навигация и ограниченный режим

**NavigationGuard.** Локальные обоснованные сигналы: IDN/confusables/homograph, redirects, protocol downgrade, обращения публичной страницы к IPv4/IPv6 private/loopback/LAN, потенциальный DNS rebinding, внешние schemes и рискованные загрузки. Unknown domain не автоматически malicious; ложные срабатывания измерять, не показывать диалоги без существенной причины. Не отправлять полный URL третьим лицам для оценки.

**RestrictedBrowsing.** Только если сигнал достаточен — запрет чувствительных разрешений, popups/download restrictions, JS-off **до первой загрузки документа** и отдельный ограниченный контекст с понятным возвратом назад. Профиль ≠ отдельный процесс, а proxy ≠ безопасность без реальных маршрутизирующих, DNS и WebRTC проверок.

## Порядок и критерии

1. Top/requester deny + native request identity regression.
2. Реальные media и улучшение жизненного цикла и проверка фактического прекращения доступа.
3. Qt/Chromium patch, sandbox и production update gate.
4. RuleEngine, независимые категории, корректная фильтрация и false-positive fixtures.
5. Развитие существующей Network-карточки и узкого tracker exception до временных независимых policies.
6. Semantic policy/event + scoped aggregator.
7. NavigationGuard и затем RestrictedBrowsing.
8. Исследование process/proxy isolation при подтверждённом threat-model.

Требования к приёмке подробно в [TEST_PLAN](TEST_PLAN.md). Не ставить shipped/tested без реального кода и сохранённых результатов. Никаких новых строк панели, permanent grants или притворной наблюдаемости.
