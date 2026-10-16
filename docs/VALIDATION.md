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
