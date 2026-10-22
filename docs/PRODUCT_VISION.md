# CYBOU — продуктовый контракт

**Текущие исходники: 0.4.0-alpha.1. Публичного релиза нет.**

CYBOU — браузер C++20/Qt WebEngine/QML, построенный вокруг **Observe → Restrict → Control → Revoke**: пользователь непосредственно видит и управляет полномочиями страниц.

## Обязательные принципы

- **11 отдельных постоянно видимых иконок** в одной существующей строке навигации; URL занимает остаток ширины.
- **Интерактивные стеклянные hover-карточки**, не заменяемые tooltips. Клик выполняет поддерживаемое действие.
- Policy, request, grant, confirmed block и фактическое использование устройства — разные факты. Нет постоянных положительных разрешений.
- Защита по умолчанию, строго ограниченные исключения, честная область действия.
- Никаких сторонних браузерных расширений и привилегированных userscripts; сторонний исполняемый код минимален. [ADR 0006](ADR/0006-no-third-party-extensions.md).
- Облачный провайдер выбирается независимо от поисковика: Proton по умолчанию; Google, Microsoft и другие, в том числе отдельный провайдер каждой службы. Никаких партнёрских привилегий. [ADR 0007](ADR/0007-selectable-cloud-services.md).

## Уже есть в коде

Control Bar, PermissionCoordinator, NavigationGuard, строгий HTTPS с exact-address HTTP exceptions, категоризированный ad/tracker RuleEngine, origin-scoped tracker exceptions, bookmarks/history/downloads, Alpine Dawn, Cloud Glass 2.0 и C++/QML cloud catalog с per-role overrides.

## До 1.0.0 не доказано

Полный hardware stop/revoke, Chrome/Qt runtime sandbox, DNS/WebRTC/PNA сценарии, полная рекламная/трекерная фильтрация, совместимость всех cloud login/2FA, независимый аудит, обновление подписанной сборки и применимые OWASP-контроли.

Статус: [ROADMAP](ROADMAP.md) · Текущие функции: [CAPABILITY_CONTRACT](CAPABILITY_CONTRACT.md) · Тестирование: [TEST_PLAN](TEST_PLAN.md).
