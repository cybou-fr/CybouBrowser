# Roadmap и статус

Статус исходного кода, 17.10.2026. Не считать наличие кода подтверждением поведения на публичном сайте.

| Этап | Статус | Следующий gate |
|---|---|---|
| P0 продуктовые документы | implemented | сверка контрактов с исходниками |
| P0 запрет сохранённых положительных grants | implemented; notification test PASS, restart/media pending | restart + iframe/media на Qt 6.12 |
| P0 no modal / pending timeout / document lease | implemented; notification lifecycle test PASS | timeout, комбинированные grants, уничтожение документа |
| P0 C++ PermissionCoordinator и независимые leases | implemented; native notification + geolocation tests PASS | реальные media/iframe и sleep/resume |
| P0 Undo/Reload исходного контекста | implemented basic tab+URL+navigation epoch guard | typed C++ transaction |
| P1 постоянные 11 иконок, glass hover panels, прямой клик | implemented; layout + input tests PASS at DPR 1 | четыре размера, DPI и FR/RU |
| P1 сетевые события блокировки | implemented profile scope | реальная блокировка, bounded burst, zero secrets |
| P1 event-driven network refresh | implemented | queued coalescing; timer только expiration |
| P2 HTTPS deny default | implemented | strict без глобального opt-out; точный HTTP opt-in в отдельном профиле на 60 секунд |
| P2 временное исключение трекеров | implemented | отдельный профиль, точный HTTPS-документ, 60 секунд, countdown/revoke; native gate в VALIDATION |
| P2 версионированный ads/tracker engine | planned | полноценные списки, версии правил и false-positive fixtures |
| P3 risk engine | planned | доказанные сигналы и JS-off до документа |
| P4 proxy isolation | research / unsupported | независимый процесс, DNS/WebRTC/route leak tests |

Полный исходный план: [OBSERVABLE_BROWSING_PROPOSAL_RU](OBSERVABLE_BROWSING_PROPOSAL_RU.md). Это предложение, не отчёт о выполнении. Никаких обещаний готовой прокси-изоляции или полной ad-block системы.
