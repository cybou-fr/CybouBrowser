# CYBOU Browser — roadmap и проверяемый статус

**Срез кода:** `main`, изменения `51559ca` (Control Bar и surfaces) и `9f7b698` (tracker exception), 17.10.2026. Позднейшие изменения документации не равны изменениям кода.

**Статусы:** **implemented** — существует код; **tested (recorded)** — ранее сохранён результат локального теста; **unverified** — не подтверждено на реальных сценариях; **planned** — ещё нет реализации. Детали и журналы — в [VALIDATION](VALIDATION.md).

## Реализовано в текущем коде

| Возможность | Статус и границы |
| --- | --- |
| **Control Bar** | **implemented, tested (recorded):** 11 иконок в одной навигационной строке, адрес занимает остаток ширины, hover открывает интерактивные стеклянные карточки; test harness на четырёх размерах при DPR 1 |
| **Browser surfaces** | **implemented, tested (recorded):** отдельные GlassToolTip для навигации, drawers под шапкой, управление вкладками и окном |
| **Чувствительные permissions** | **implemented, tested (recorded):** C++ `PermissionCoordinator`, request ID, `SiteContext`, fail-closed ожидание, независимые leases, `AskEveryTime`; real mic/camera и cross-origin iframe **unverified** |
| **Текущий отзыв grant** | **implemented, UX gap:** отзыв реализован через завершение WebEngineView, что может оборвать пользовательскую работу; новый UX отзыва **planned** |
| **Transport** | **implemented, tested (recorded):** production HTTPS strict; exact-address HTTP consent в отдельном off-the-record профиле, последующее завершение/отзыв без наследования обычных cookies |
| **Tracker-only exception** | **implemented, narrow:** отдельный временный профиль для выбранной HTTPS-страницы, пропуск matching tracker-list traffic, ручной revoke и завершение по закрытию/перезагрузке/смене документа; `--tracker-test` harness присутствует; не полный adblock |
| **Network events** | **implemented:** ограниченная очередь, sanitized origin, blocked reason и type, только scope профиля; нет проверенного per-tab attribution |
| **Windows** | **tested (recorded):** C++20 Release, Qt 6.12/MSVC, security/controlbar/surface/transport suites; Linux/macOS и release qualification остаются unverified |

## Далее — приоритетный порядок

| ID | Приоритет | Задача (**planned**, если не указано иное) | Критерий приёмки |
| --- | --- | --- | --- |
| S0.1 | P0 | Одинаковое применение explicit deny к top origin и вложенным requester iframe | Двухoriginные fixtures, stale ID/tab/profile/epoch тесты |
| S0.2 | P0 | Достоверный и пригодный для звонков отзыв media без внезапного разрушения документа; перепроектировать жизненный цикл доступа по требованиям владельца продукта и проверить гарантии отзыва | Реальные mic/camera/combined tests, долгий звонок, revoke, sleep/resume |
| S0.3 | P0 | Поддерживаемый патченный Qt/Chromium, sandbox/release flags и безопасная доставка обновлений | Runtime-проверка, регрессия и проверка обновлений |
| S1.1 | P1 | Версионированный ad/tracker RuleEngine с проверкой целостности и false positives | Реальные категории, производительность, rollback, сайты login/video/payment |
| S1.2 | P1 / частично | Развить реализованное tracker-only исключение до независимых временных Ads/Trackers действий **в той же Network hover-карточке** | Две вкладки, independent categories, Undo, HTTPS unchanged |
| S1.3 | P1 | Семантика policy/event и `ActivityAggregator` с правдивым scope | Default != blocked event; grant != recording; profile != tab |
| S2.1 | P2 | Локальный Navigation Guard: IDN/redirect/private-network | Positive/negative fixtures, отсутствие необоснованных предупреждений |
| S2.2 | P2 | Restricted Browsing: JS-off до документа и проверяемые ограничения | Before-load tests, корректный отказ и выход |
| S3 | research | Process/proxy isolation | DNS/WebRTC/route leak tests; никакого рекламного обещания заранее |

## Правила внесения изменений

Все изменения прямо в `main`, **без веток**, после локальных тестов. GitHub Actions пока не является обязательным условием. Никакого редизайна Control Bar, Always Allow, глобального opt-out из карточки сайта или фальшивых событий. Старые интеграционные планы — исторический материал и не переопределяют [PRODUCT_VISION](PRODUCT_VISION.md) и [CONTROL_BAR_UX](CONTROL_BAR_UX.md).

**Детальный план реализации:** [SECURITY_UX_EVOLUTION](SECURITY_UX_EVOLUTION.md). **Проверки:** [TEST_PLAN](TEST_PLAN.md).
