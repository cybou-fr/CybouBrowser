# Проверки

Build: `cmake --build build --config Release --parallel 4` (Qt 6.12, MSVC 2022).

HTTP transport: `build/Release/cybou.exe --transport-test`. A loopback fixture checks initial strict denial, real hover-panel consent, a separate off-the-record profile, same-origin assets, blocked cross-origin HTTP, no inheritance by another tab, stale-token rejection, reload requiring fresh consent, 60-second background expiry preserving the selected peer tab, real panel revoke, and destruction of the temporary native profile. `--security-test` also checks exact URL/query matching, concurrent single-use consumption, origin/firstParty restrictions, ws and credential rejection, canonical root URLs and explicit revocation. Screenshots: `build/transport-http-blocked.png`, `build/transport-http-temporary.png`. Test-only HTTP opt-out remains unavailable in normal launches.
Run: `build/Release/cybou.exe --security-test`, `--smoke-test`, `--controlbar-test`.
Test organization CYBOU-Tests and test profile separate ordinary browsing; HTTP fixture access is enabled explicitly only for test runs.

ControlBar harness must enforce all 11 controls during URL editing and at 820 px; no arbitrary notification pregrant; glass hover panels with explanations and controls; pointer corridor and current-icon content. Verify live notification request cancellation on tab switch. Existing historical overflow/ASK assertions are superseded by this contract.

Manual release gates: real mic+camera requests; cross-origin iframe requesting origin; timeout after 15s; grant deadline unload after 60s; close/tab navigation; restart without restored grants; Undo after same-origin navigation; blocked tracker network event with reason; HTTPS HTTP/ws strict; cookies persistence independent from permissions. Real media use and proxy routing cannot be certified by model color.

Record actual test outcomes and unverified cases in VALIDATION.md. Never label manual tests passed without running them.

C++ permission coverage (`--security-test`): view/profile/tab/top-origin/navigation-epoch mismatch; cross-origin requester preserved; destroyed weak context; invalid native request; monotonic ceiling despite wall-clock rollback and expiry after forward jump.

Native coordinator coverage (`--controlbar-test`): stale same-origin epoch denies the actual request; old request ID cannot resolve a later native request; a real reset-icon click replaces the denied view at the same URL without adding a tab; notification and geolocation grants have independent deadlines; later grant does not renew the first; earliest deadline destroys the WebEngineView and replaces its slot with a new UID; a new document must ask again; 15-second unanswered request is denied.
