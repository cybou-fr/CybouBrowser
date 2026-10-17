> **Versioning notice (2026-10-17):** CYBOU currently declares **0.3.0-alpha.1** (engineering alpha, not release-qualified). This ADR records its scoped product decision; any historical test-status claim is not current-candidate security evidence. [Version policy](../VERSIONING.md) · [OWASP verification](../SECURITY_VERIFICATION_FRAMEWORK.md).

# ADR 0004 — tracker-only exception через отдельный профиль

**Статус:** узкий механизм реализован; независимый ads/tracker rule engine запланирован.

Запросы перехватываются на уровне `QWebEngineProfile`; без достоверной tab attribution нельзя отключить filtering на общем профиле ради одной вкладки.

По явному согласию выбранная HTTPS-страница открывается заново в отдельном off-the-record профиле с внутренним `AskEveryTime`. Обычные cookies/cache не переносятся. `TrackerExceptionLease` разрешает одно конкретное открытие документа, а затем применяет исключение к matched tracker-list requests при соответствующем `firstParty`. HTTPS strict, URL credentials deny и остальные ограничения не отменяются. Этот режим не совмещается с временным HTTP-исключением.

Отзыв, новая навигация, reload и завершение временного контекста удаляют исключение и могут разрушать документ. Hover-панель Network показывает состояние, подтверждённые network decisions и возможность revoke. Никакого бессрочного или межвкладочного Allow.

События ограничены sanitized origin/firstParty, resource type, action и category. «Фильтр пропустил» не доказывает, что ресурс действительно загрузился. Это ещё не полноценная блокировка рекламы и не изоляция процесса, DNS или маршрута. См. [ROADMAP](../ROADMAP.md), [SECURITY_MODEL](../SECURITY_MODEL.md), [SECURITY_UX_EVOLUTION](../SECURITY_UX_EVOLUTION.md).
