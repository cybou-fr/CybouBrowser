# CYBOU — Control Bar UX

**Текущий контракт 0.4.0-alpha.1:** все 11 отдельных иконок постоянно доступны в **одной строке навигации** с URL. Не группировать, не прятать, не переносить в новый toolbar.

## Геометрия и клавиатура

Слот 42 px, кнопка 30 px; URL занимает оставшуюся ширину, цель ≥120 px при ширине окна 820 px. Справа разделитель, Downloads и Menu в той же строке. Ctrl+L показывает полный URL, Escape возвращает просмотр. Не менять число строк chrome.

## Интерактивные карточки

Hover с задержкой около 170 мс открывает **стеклянный интерактивный popover**, закрытие около 300 мс. Курсор должен переходить внутрь, соседняя кнопка сменяет карточку без stale state. Карточка показывает effective policy, scope, observed event, источник/ограничения, настоящие действия. Обычный GlassToolTip используется для nav controls, **не вместо capability popover**.

Клик по иконке действует непосредственно, если backend поддерживает действие; Screen без chooser и Cookies profile-wide не изображать как site-scoped toggle. Request, Grant, Block и аппаратное использование различаются цветом и понятной подписью.

## Граница с облачной панелью

«Почта» и девять точек живут **только внутри NewTabPage справа сверху**. Click → отдельная Cloud Glass 2.0 панель выбранных web apps. Она не меняет Control Bar, omnibox и permission controls. [ADR 0007](ADR/0007-selectable-cloud-services.md).

## Приёмка

820×560, 1024×768, 1360×900, 1920×1080; DPI 100/150/200%; FR/RU, hover corridor, keyboard/Tab/Enter/Space/Escape, reduced motion. Это **проверки**, не автоматически PASS. [TEST_PLAN](TEST_PLAN.md).
