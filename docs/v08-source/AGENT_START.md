# START — CYBOU Browser visual UI 0.8

Репозиторий: https://github.com/cybou-fr/CybouBrowser (`main`). Последний проверенный SHA: `3d3922a592124feda0c5029df70fbbf377253e01`, версия 0.7.1. До правок проверь свежий HEAD.

Читайте `CYBOU_0_8_VISUAL_IMPLEMENTATION_PLAN_RU.md` полностью. Наша задача — **реализовать визуальный дизайн, а не переписывать браузер**. Adaptive Omnibox/Control Bar **уже внедрены**. Сделай целостный navy glassmorphism всего browser-owned UI с утверждёнными цветами CYBOU и новым фоном Alpine Dawn для `about:blank`. **Логотип строго `assets/cybou.png` репозитория**, без перерисовки.

Приложенные ресурсы: `cybou-alpine-dawn-1920x1080.jpg` — предпочтительный production asset (переложить в repo `assets/` и зарегистрировать в Qt resources); `.png` — lossless reference; `CYBOU_Adaptive_Control_Bar_Reference_v0_7.png` — визуальный ориентир UI. Обе картинки — дизайн-материалы, не результат выполнения программы.

Порядок: baseline + все 3 test modes → Theme singleton + glass primitives + navy tabs/omnibox → `NewTabPage.qml` с embedded wallpaper, реальным logo, search → ControlBar icon/overflow styling + layout budget QA → Settings, Drawer, dialogs → регрессия, DPI, реальные screenshots. Маленькие коммиты **в main**, без веток. Береги private profiles, hostname/IDN security, HTTPS-only, download confirmation и permissions.

Первым промежуточным результатом предъяви **два реальные скриншота с кодом**: новая вкладка на рассвете и обычный HTTPS-сайт с dark glass chrome. Никаких сгенерированных «скриншотов» под видом реального UI.
