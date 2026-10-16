# ADR 0003 — strict HTTPS и временный HTTP-контекст

**Статус:** реализованная модель исключения для конкретной навигации.

Production HTTPS strict без глобального opt-out. По явному запросу пользователя Network-карточка может открыть **один конкретный HTTP URL** в отдельном off-the-record профиле; допуск same-origin HTTP resources ограничен origin и firstParty контекстом. Следующий переход, reload, revoke или закрытие контекста не превращает исключение в разрешение для других URL или обычного профиля. Запрещены embedded credentials и незащищённые дополнительные origins. Page-controlled navigation не переигрывается с нормальными cookies.

Отдельный профиль означает разделение cookies/cache, **не process isolation, не proxy и не анонимность**. Тестовый режим HTTP fixture не должен быть доступен обычному запуску.

Техническое устройство: [ARCHITECTURE](../ARCHITECTURE.md). Текущий статус и проверки: [ROADMAP](../ROADMAP.md), [VALIDATION](../VALIDATION.md).
