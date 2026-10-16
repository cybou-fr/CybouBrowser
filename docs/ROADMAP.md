# CYBOU Browser — roadmap: реализация и развитие

**Актуально на 17 октября 2026.** Проверка `main` @ `51559ca33e089eeebca3edfb49dff05710d1d84c`. Это документ о состоянии **кода**, а не сертификат реальной защищённости. `implemented` — есть код; `tested (recorded)` — в репозитории сохранены логи локального прогона; `planned` — реализации пока нет.

## Реализовано в текущей кодовой базе

| Этап | Статус и подтверждение | Границы |
| --- | --- | --- |
| 11 inline capabilities + glass hover | **implemented; tested (recorded)**: 820/1024/1360/1920, Ctrl+L, pointer corridor, keyboard; `--controlbar-test` | DPI 150/200%, FR/RU и все ОС требуют проверки |
| Browser-owned glass surfaces | **implemented; tested (recorded)**: `--surface-test`, обычные GlassToolTip, drawers под 74px header | Ручные real-monitor испытания остаются |
| Без постоянного positive trust | **implemented; tested (recorded)**: `AskEveryTime`, native coordinator с request ID, SiteContext, 15 c pending и независимыми 60 c grants | Camera/mic/iframe и OS suspend/resume не проверены на устройствах |
| Безопасный отзыв документа | **implemented**: earliest grant expiry уничтожает WebEngineView, не передаёт grants другому документу | Может неожиданно прерывать звонки и работу пользователя |
| Production HTTPS strict | **implemented; tested (recorded)**: точный HTTP URL через off-the-record профиль на 60 c, same-origin assets, revoke, reload/epoch guards; `--transport-test` | Data isolation != process/proxy isolation |
| Network observation | **implemented**: bounded (256) metadata, outcome/reason, profile scope; event-driven refresh | Нет доказанной per-tab атрибуции |
| Tracker blocker | **limited implementation**: небольшой static suffix-list | Нет полного ad/tracker engine, классификации и site exceptions |
| Windows build | **implemented; tested (recorded)**: C++20, Qt 6.12, MSVC 2022 Release, security tests в `VALIDATION.md` | Linux/macOS, release pipeline и независимый аудит не проверены |

## Новые этапы — не реализованы

| ID | Приоритет | Задача | Gate завершения |
| --- | --- | --- | --- |
| S0.1 | **P0** | Устранить несовпадение explicit top-origin deny и iframe requesting-origin policy | Cross-origin A→B deny, запрет requester, stale request ID, нормальный/приватный профиль |
| S0.2 | **P0** | Реальные media-тесты; безопасный UX временного grant без тихого обрыва страницы и без постоянного Allow | Mic/camera/combined, звонок 5–30 мин, expiry/revoke и OS sleep/resume |
| S0.3 | **P0** | Hardening Qt/Chromium: sandbox, актуальность патчей, release/update-дисциплина | Runtime версии и sandbox проверены; регрессия на обновлённом WebEngine |
| S1.1 | **P1** | Версионированный, проверяемый rule engine для ads и trackers | Категории реальны, false positives/производительность, безопасное обновление правил |
| S1.2 | **P1** | Две независимые временные site policies **в той же Network hover-карточке** | Two-tabs isolation, TTL, reset, Undo, HTTPS strict не меняется |
| S1.3 | **P1** | Семантические цвета policy/events, bounded `ActivityAggregator` | Нет red-allow, no fake active device, no fictitious per-tab counts |
| S2.1 | **P2** | Локальный Navigation Guard: IDN/redirect/private-network/опасные схемы | Позитивные и негативные fixtures без навязчивых false positives |
| S2.2 | **P2** | Restricted Browsing: JS-off до документа, ограниченный профиль/разрешения | Before-load JS test, safe exit; не выдавать профиль за полноценный sandbox |
| S3 | **Research** | Separate-process/proxy isolation | DNS/WebRTC/route leak tests и проверяемая процессная граница |

**Именно этот порядок — приоритетный.** Пошаговая спецификация, сценарии и неизменяемые UX-решения: [SECURITY_UX_EVOLUTION](SECURITY_UX_EVOLUTION.md). Текущие API: [CAPABILITY_CONTRACT](CAPABILITY_CONTRACT.md); зафиксированные предыдущие локальные результаты: [VALIDATION](VALIDATION.md); новые тесты: [TEST_PLAN](TEST_PLAN.md).

**Правила:** не создавать ветки, небольшие коммиты в `main`; без новой строки панели, без группировки 11 иконок, без замены glass hover на plain tooltips, без Always Allow/модального ASK или глобальных отключений в site card. Любое `planned → implemented` сопровождается кодом, реальными проверками и изменением статуса. Историческая [OBSERVABLE_BROWSING_PROPOSAL_RU](OBSERVABLE_BROWSING_PROPOSAL_RU.md) не переопределяет эти решения.
