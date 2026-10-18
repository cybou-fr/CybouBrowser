# CYBOU — модель безопасности

**0.3.0-alpha.1: защитные механизмы в коде есть, релизная квалификация не выполнена.**

## Недоверенные области

Любой сайт, iframe, URL, redirect и сторонний облачный провайдер является недоверенным. Browser-owned C++/QML не должен принимать полномочия от сайта без конкретного подтверждённого запроса. Сторонние браузерные расширения запрещены ([ADR 0006](ADR/0006-no-third-party-extensions.md)).

## Permissions

C++ PermissionCoordinator связывает QWebEnginePermission с view/profile, top и requesting origin, navigation epoch. Без живого request нельзя выдать постоянный positive grant. Pending без ответа → deny. Отзыв может пересоздать WebEngineView. Grant/JavaScript stop-track не доказывают физическую остановку *всех* media tracks — это предмет аппаратных тестов.

## HTTPS / Network

Strict HTTPS и certificate-error reject. Ограниченное HTTP consent — exact-address в отдельном off-the-record профиле, **не** process/proxy isolation. NavigationGuard ограничивает известные небезопасные пути; DNS rebinding, IPv6/PNA/redirect chains требуют негативных проверок.

Категоризированный RuleEngine и tracker observer реализованы с ограниченным набором правил. Tracker exception действует по **origin в общем профиле** и принадлежит вкладке-владельцу; другие вкладки того же origin показывают реальное состояние. Блокировка конкретного запроса не означает полного устранения tracking. Сетевые события profile-scoped, не per-tab точность.

## Локальные данные и облако

History, session URLs, cookies и site storage могут сохраняться; не считать private WebEngine profile анонимизацией. Cloud catalog содержит локальные URL/настройки, но не читает почту, файлы или OAuth-токены. Proton, Google, Microsoft и остальные не получают дополнительных permissions. Nextcloud — пользовательский проверяемый HTTPS endpoint.

## Не доказано

Все применимые OWASP ASVS/WSTG требования, runtime sandbox, WebRTC/DNS leaks, hardware revoke, полный adblock, release signing/update, безопасность реальных облачных SSO. [THREAT_MODEL](THREAT_MODEL.md) · [TEST_PLAN](TEST_PLAN.md) · [RELEASE_CRITERIA](RELEASE_CRITERIA.md).

**WebRTC strict (opt-in).** По умолчанию WebRTC разрешён, локальные кандидаты скрыты через mDNS, но страница всё равно может заставить браузер слать STUN-проверки на loopback и приватные адреса. Настройка «WebRTC strict» в разделе безопасности отключает прямой UDP и закрывает этот вектор ценой звонков, которым нужен peer-to-peer UDP; применяется после перезапуска. Подробности: [VALIDATION](VALIDATION.md).
