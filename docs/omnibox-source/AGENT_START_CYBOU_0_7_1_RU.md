# START — CYBOU Browser 0.7.1 Adaptive Omnibox

Репозиторий: https://github.com/cybou-fr/CybouBrowser, работать только маленькими проверяемыми коммитами в `main`; не создавать branches и не делать force push. Последний проверенный GitHub HEAD на момент аудита: `9a809aaacad28ed587aadcd0d8d8d3e48b6fc4a8`. **Обязательно обновить/проверить HEAD перед работой.**

Прочти `CYBOU_0_7_1_REVISED_PLAN_RU.md` целиком. Он заменяет устаревшие указания про четыре положения Control Bar и уточняет предыдущую спецификацию 0.7. Пожалуйста, не реализуй ещё один floating rail или position picker.

**UX-контракт:** В BROWSE (`TextField` адреса без фокуса) показать compact hostname и максимум доступных цветных icon buttons между адресом и Downloads, недостающие — в `···`. В EDIT (адрес в фокусе, в том числе Ctrl+L) спрятать ВСЕ icon buttons и `···`, отдать всю ширину полному редактируемому URL. Browser Downloads/Menu и nav кнопки всегда остаются. Нет отдельной панели поверх страницы.

**Начинай с baseline:** `git status`, `git log -1`, сборка, сохранение текущих `--smoke-test`, `--controlbar-test`, `--security-test`. Последний коммит добавил HTTPS-only. Не сломать его. Не переписывать ControlBarController, CapabilityModel, Qt WebEngine profiles или разрешения ради UI.

**Этапы:** P1 inline row/hostname/EDIT, P2 overflow только hidden capabilities, P3 единый popover/consent, P4 state colors/animations/accessibility, P5 новый harness и документация. Подробные gates и A01–A24 — в основном документе.

**Важно:** не записывай короткий hostname в raw address.text. Пользователь может редактировать/копировать **полный** URL; Ctrl+L selects all; query/path и origin не теряются. Использовать inputWidth budget без circular QML bindings. Убрать `ControlBarPositionPicker.qml` и старые тесты dock после замены. На `about:blank` никаких 11 знаков вопроса. HTTPS-only остается opt-in, без изменения semantics.

Выход: HEAD до/после, SHAs небольших коммитов, реальные скриншоты Browse/Edit/Overflow/Details, test matrix с PASS/FAIL/BLOCKED и командами/exit codes. Не объявлять production readiness без updater/security audit.
