# Local validation — 16 October 2026

Environment: Windows, MSVC 2022, Qt 6.12.0; Qt WebEngine 6.140.0 / Chromium 140.0.7339.225 (runtime-reported security patch 152.0.7977.83). These are runtime version strings, not an assertion of vulnerability status.

## Product clarification
User explicitly overrode the proposal: no separate capability row. Final layout keeps all eleven controls on the right of the same navigation row; address fills all remaining width. Ctrl+L and mouse editing preserve controls. No fixed narrow address or intervening spacer.

## Results
- Release build: PASS (`cmake --build build --config Release --parallel 4`).
- Security test: PASS; HTTP/URL-credentials/subresource rejection, tracker rules, sanitized display, download policy and a recorded blocked network event with reason and profile scope.
- Smoke suite inside controlbar test: download review/type guard, private profile cleanup, tab drag and address suggestions have passed.
- Control Bar layout: all 11 controls visible at 820×560, 1024×768, 1360×900 and 1920×1080; full URL selected by Ctrl+L and mouse; no extra row; hover does not open shared details.
- Full grant deadline / timeout regression: PASS. Final Control Bar test completed in 82,208 ms, exit 0. A live notification was granted by clicking its icon; no native permission was stored. After 60 seconds the document unloaded; reopening required new consent; unanswered consent was denied after 15 seconds. Full smoke suite and PDF export also PASS.

Logs: `build/observable-security.stderr.log`, `build/observable-lifecycle.stderr.log`. Acceptance screenshots: `docs/design/observable-browsing/` (browse/edit at four sizes, DPR 1).

## Remaining release gates
Real microphone/camera/combined iframe capture, OS sleep/resume deadlines, process-restart migration with a pre-existing granted profile, FR/RU completeness and DPI 150/200% are unverified. Full rulesets, request-scoped HTTP exceptions, risk engine and proxy isolation are not implemented by this increment. No claims of active device use or per-tab network counts.

Early test iterations exposed delayed URL selection after resizing and retained JS policy after a lease unloaded about:blank. Fixes defer selection until layout settles and explicitly expire the tab context on unload. Earlier separate-row screenshots are superseded by the final acceptance set.

## C++ PermissionCoordinator increment

Release build PASS. Security test PASS, including weak context ownership, actual view/profile/tab/top-origin/epoch matching, preserved iframe requester and dual-clock expiry checks. Final native Control Bar + smoke + PDF run PASS, exit 0, 82,862 ms.

The native test rejects a real request after its epoch changes; a reset-icon click recreates the same URL in the same tab slot; a stale request ID cannot resolve the later request. Notification and geolocation grants coexist and the second does not extend the first deadline. At the earliest expiry, the old WebEngineView is destroyed and replaced with a blank view and new tab UID. The reopened document asks again; the unanswered request resolves `denied` after 15 seconds. Button press captures the request ID used on release. This validates permission decisions, not actual device activity.

Evidence: `docs/design/observable-browsing/coordinator-security-test.log` and `coordinator-controlbar-test.log`. Original layout screenshots remain applicable: no navigation geometry changes in this increment.

A native denial was not re-requested after a same-view reload in the test iteration; the supported sensitive reset action therefore recreates the view, rather than relying on reload to clear document-level state. Real media shutdown, OS sleep/resume, pre-existing disk profile migration, cross-origin iframe enforcement and DPI/translation gates remain unverified.

## Glass hover panels and spacing

Latest user instruction replaces the earlier tooltip-only contract. Capability buttons now have 42px center spacing and 12px clear gaps. All eleven stay inline on the right; the address fills remaining space with a 120px minimum.

Release build PASS. Updated `--controlbar-test` PASS in 84,079 ms, exit 0; smoke and PDF export PASS. Mouse hover opens the current glass panel without click; moving inside keeps it open for controls; moving to JavaScript replaces the location content; moving away closes it. All four window sizes and URL editing passed. Permission lifecycle regressions still pass. Native ordinary capability ToolTips are removed. Visual inspection of the JavaScript hover panel and 820px layout passed.

Evidence and screenshots: `docs/design/glass-hover/`. The earlier tooltip-only findings above describe historical increments and are superseded by this section.

## Temporary HTTP transport

Release build PASS. `--security-test` PASS, exit 0: exact URL/query, atomic single-use main navigation, same-origin/firstParty resource restrictions, independent leases, canonical root URLs, credentials/ws rejection and revoke. Existing permission context, tracker and network-denial checks still pass.

Native `--transport-test` PASS, exit 0, including smoke and PDF output. Actual mouse hover and button clicks open/revoke HTTP in a separate off-the-record AskEveryTime profile. A cross-origin HTTP fetch is blocked by the interceptor, same-origin resources load, another normal-profile tab remains denied, stale consent is rejected, reload requires fresh consent, and 60-second background expiry replaces the HTTP view without switching the peer tab. A native QPointer verifies final profile destruction. Both panel screenshots were visually inspected; the toolbar stays inline and address fills remaining width.

Final `--controlbar-test` regression PASS, exit 0, 83,806 ms; hover interaction, window sizes, permission request identity, independent grant deadlines, document teardown, timeout denial, smoke and PDF checks pass after transport integration.

Evidence: `docs/design/transport/`. Qt prints a deprecation guidance warning for the currently supported public QQuickWebEngineProfile constructor; this implementation uses no private Qt API. Real-device media shutdown, OS suspend/resume, cross-origin iframe behavior and independent proxy routing remain outside this validation.

## Git integration and window chrome (2026-10-17)

Merged origin/main, retaining the updated README structure and C++20 requirement while updating current HTTP/permission/hover behavior. No unmerged entries remain. C++20 Release build and security suite PASS.

The unused ListView tab-strip area now explicitly receives title mouse events. A system drag starts only after the platform movement threshold, preserving double-click handling. Native tests PASS for small pointer movement, maximizing by double-click, restoring the previous 1100×720 size, and independent tab reordering. Actual system movement between monitors and maximized-window dragging remain manual Windows checks.

The connection indicator becomes an explicit HTTP consent/revoke button for pending/temporary HTTP. Native transport suite PASS, exit 0: first consent through the glass panel, later consent and revoke through the new address-row button, isolated profile, stale consent, reload and 60-second expiry. Final Control Bar regression PASS, exit 0, 83,771 ms: separator placement and all eleven controls at four widths, address editing, glass hover, independent permission deadlines, timeout, smoke and PDF. The 820px layout and HTTP screenshot were visually inspected. Evidence: `docs/design/window-chrome/`.

## Glass tooltips and workspace drawers (2026-10-17)

Release build PASS. Native `--surface-test` PASS, exit 0: actual mouse hover shows the custom GlassToolTip; downloads, bookmarks, history and settings match the content workspace at 1360×900 and 820×560. Drawer tops are at the page boundary (74px in the fixture), heights match the remaining workspace, and full-window modality/dimming is disabled. Navigation and tab captions share the styled item tooltip; no default attached ToolTip remains in QML. Tab drag and title double-click checks also pass in this run.

Visually inspected the navigation tooltip, downloads at 1360px and settings at 820px: blue rounded hint without the native black/white-border style; drawers cover neither tabs nor address row. Evidence and all eight drawer screenshots: `docs/design/glass-surfaces/`.
