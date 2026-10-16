# ADR 0002 — request-scoped permissions и отзыв контекста

**Статус:** принятое описание текущей архитектуры. Будущий UX media-сессий — отдельное продуктовое решение.

- Оба WebEngine-профиля используют внутреннее `AskEveryTime`. Permanent positive origin grants и user-facing ASK/Always Allow не применяются.
- C++ `PermissionCoordinator` обрабатывает один актуальный pending request на окно, связывает `QWebEnginePermission` с request ID, view/profile, tab UID, top/requester origin и navigation epoch; stale/invalid/background/unsupported отклоняются.
- Кнопка сохраняет request ID при нажатии, чтобы отпускание не подтвердило следующий чужой запрос.
- Для выданных permissions существуют независимые `PermissionLease`, а закрытие/смена документа и отзыв контекста удаляют право. Текущая реализация может разрушать `WebEngineView`; это **не** доказательство того, что native media stream можно достоверно остановить *внутри той же страницы*.
- Сброс чувствительной capability без ожидающего запроса пересоздаёт документ с тем же URL в том же слоте вкладки, сохраняя важное состояние вкладки, но не grants. No automatic pregrant.

**Открытая задача:** проверить top-origin deny и requester-origin для cross-origin iframe; отдельно проверить реальные устройства и улучшить UX длинных звонков в соответствии с решением владельца продукта. См. [SECURITY_MODEL](../SECURITY_MODEL.md), [SECURITY_UX_EVOLUTION](../SECURITY_UX_EVOLUTION.md).
