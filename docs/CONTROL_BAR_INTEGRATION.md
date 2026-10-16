# CYBOU 0.6 — reconciled Control Bar integration

Input: the three Markdown files in the supplied 2026-10-16 ZIP, preserved in controlbar-source/. The provided glass/alpine image is visual guidance for the rail, cards and four placements. Its illustrated extra tracker/fingerprint icons are not added: the written contract limits v1 to supported capabilities. The browser's 36px tab and navigation rows, existing features, C++17 and independent private windows remain intact.

The package's repository audit targets 0.3 / d2f1a99. Integration used actual local 0.5 / 687c1d1, not the stale source listing.

## Commits

- 69ce05d: reconciled source documents and verified baseline/API matrix.
- 78b8f2c: working Control Bar, origin-aware controller, fixed capability model, profile request observer, vector rail and FR/RU catalog.
- The subsequent 0.6 verification/polish commit contains accessibility, DPI/interaction checks, URL synchronization fix, preference persistence checks and this report. Obtain its exact ID with `git log -1 --oneline`.

## Delivered behavior

One brand-free, 36px continuous acrylic capsule overlays the actual page/start-page viewport. The final dock icon offers four miniature placement diagrams. Placement is global, immediate, persisted, and inherited by private windows. It never adds a sidebar or changes Chromium view size. Right placement reserves 28px for scrollbar/resize access. The material uses a translucent fallback, not an unverified live webpage blur.

Hover opens one card after 170ms. Moving through the gap into the card preserves it; leaving both closes after 220ms. Click pins; Escape/outside dismisses. Hover never resolves a permission. Icons have a roving keyboard stop with arrow navigation, state shapes and accessible labels. System motion/contrast preferences are read where supported by the installed Qt; manual reduced-motion/glow preferences are available in Settings. Rail is hidden in fullscreen and behind settings/library/browser modal work dialogs. Native consent remains the safe modal fallback.

| Item | Real authority and delivered boundary |
|---|---|
| Camera/microphone | Actual Qt request events; per-request explicit consent; ASK/DENY origin policies. Combined A/V updates both icons. No permanent pregrant for ephemeral permissions, no recording claim. Cached grants and existing streams may need reload/close. |
| Screen | Unavailable, safely denied. No untested capture-source chooser. |
| Location, notifications, clipboard | Native persistent permission query/grant/deny/reset. Controls show the origin and scope. Native desktop notification presentation and actual location availability are not claimed. Clipboard coverage is limited to Qt's supported permission path. |
| JavaScript | Origin-keyed policy applied to the relevant view before navigation. Existing documents display reload-required; explicit reload applies it. Different origins stay isolated. |
| Popups | Defaults to user gestures only. Explicit allow/block policy composes Qt settings with the new-window gate. Only actual Qt events produce activity. |
| Cookies | Existing single global third-party filter is preserved. Scope is all open profiles, with no per-site cookie totals or fake full wipe. |
| Storage | localStorage setting only, with reload semantics. IndexedDB/Cache Storage/service-worker enumeration and wipe remain outside the control. |
| Network | Real HTTP(S) interceptor, profile scope. Ring of at most 256 sanitized events, 60s displayed TTL, up to 12 rows. Keeps origin/first-party origin, resource type and time; no URL paths, queries, headers, bodies, transfer size or tab-attribution claim. |

Transient observations are bounded and memory-only, matched by stable tab ID plus top/requesting origins. Private policies/events stay within their controller/profile lifetime. Persistent normal policies are origin keyed, including scheme/port. Policy state and event state are separate. Permission cleanup resets native decisions and CYBOU camera/mic ASK/DENY rules.

During visual QA an existing tab-switch race was found: the omnibox could retain the previous tab URL. The active-view synchronization now fixes it; the harness asserts that the displayed URL matches the active view.

## Changed code paths

- src/controlbar/ControlBarController.h/.cpp: typed policy states, policy storage/resolution, native permission bridge and bounded scoped observations.
- src/controlbar/CapabilityModel.h: stable QAbstractListModel; updates rows without recreating focused/hovered icons.
- src/controlbar/RequestObserver.h: bounded thread-safe metadata interceptor, no callback I/O.
- src/browserstore.h: existing profile composition point, cosmetic settings, language, accessibility hints, observer lifetime. The existing cookie filter remains singular.
- qml/ControlBar.qml, ControlBarIcon.qml, CapabilityPopover.qml, ControlBarPositionPicker.qml: trusted browser-owned overlay and interaction.
- qml/Main.qml: small integration hooks, stable tab IDs, validated consent, per-view navigation settings, URL synchronization.
- qml/SettingsPanel.qml: language/motion controls and existing cleanup integration.
- qml/ControlBarHarness.qml and src/main.cpp: explicit test-only fixture/scenarios, input events and screenshots.
- assets/controlbar/: 12 small SVGs; no brand mark in the rail.
- translations/cybou_ru.ts: 77 finished FR→RU messages for the new panel/policy workflow. Existing browser UI, including the native consent fallback, remains primarily French.
- CMakeLists.txt: source/resource registration, explicit WebEngineCore and LinguistTools, Qt translation compilation; C++17 retained.

## Tests actually executed

Baseline and post-integration Release builds on Windows/MSVC/Qt 6.12. Existing --smoke-test: PASS. Extended --controlbar-test: PASS. Logs in build/controlbar-baseline.log and build/controlbar-*.log.

The extended run includes existing tab dragging, omnibox keyboard suggestions, pinned/muted restoration, downloads/pause/resume/cancelled window closure, local files, PDF signature and profile destruction. New checks include:

- Real notifications request crosses the C++ bridge; native grant/deny/reset and UI model match.
- Switching tabs cancels pending consent and clears pending indicators.
- Private policy/event separation using the actual two private profiles.
- JS disabled after reload on origin A; origin B remains enabled. localStorage toggle is verified by the fixture.
- Unsolicited popup blocked by default, opened only after explicit policy; policy restored afterward.
- 80-request fixture burst, sanitized origins, maximum retained sample enforced.
- Four placements inside 820×560 window, 36px thickness, unchanged WebEngine viewport and valid omnibox URL.
- Real pointer events for hover→card corridor, mouse-out, pin/Escape, last-icon position picker/choice and page clicks outside overlay; roving keyboard arrows.
- Settings/library/fullscreen hide policy, Russian catalog, persisted position across process restarts.

Final DPR runs use normalized QT_SCALE_FACTOR against the machine's native 1.75 scale; actual QQuickWindow DPR is logged, rather than inferred from environment variables:

| Log | Actual DPR | Result |
|---|---:|---|
| build/controlbar-dpi100.log | 1.0 | exit 0, PASS |
| build/controlbar-dpi125.log | 1.25 | exit 0, PASS |
| build/controlbar-dpi150.log | 1.5 | exit 0, PASS |
| build/controlbar-dpi200.log | 2.0 | exit 0, PASS |

Each run saves top/right/bottom/left, detail and Russian screenshots in build/dpi-100/, dpi-125/, dpi-150/, dpi-200/. The test shows real fixture data; no test activity is generated during ordinary browsing.

Final post-polish run: build/controlbar-final.log, native DPR 1.75, exit 0. All existing PASS markers plus CONTROL_BAR_TEST PASS; includes the additional custom permission-cleanup and actual mouse-operated position-picker checks. Deployment via windeployqt also exited 0. Runtime journal contains no QML warnings in this final run.

Snapshots at DPR 1.0: [top](../build/dpi-100/controlbar-top.png), [right](../build/dpi-100/controlbar-right.png), [bottom](../build/dpi-100/controlbar-bottom.png), [left](../build/dpi-100/controlbar-left.png), [details](../build/dpi-100/controlbar-detail.png), [Russian](../build/dpi-100/controlbar-ru.png).

## Reproduce

Use the README build instructions (Qt LinguistTools is now required). Run `build/Release/cybou.exe --controlbar-test` with QT_FORCE_STDERR_LOGGING=1. It includes the existing smoke suite. `--test-position=bottom` is a smoke-only setting override used for restart persistence checks; normal launch ignores that test option.

## Explicit remaining gates

See API_CAPABILITY_MATRIX.md for observed limits. No physical capture/stream stopping, screen-source chooser, full site-data wipe, desktop notification presentation, credential management, updater, signed installer, independent security review or public release readiness is claimed. Touch/stylus and mixed-monitor migration remain untested; OS accessibility settings are wired but were not changed on the user's machine. Cross-origin iframe hardware permission flows and dedicated first-/third-party cookie fixtures need further tests. Interceptor burst testing is a small local responsiveness check, not a full performance benchmark.

The next smallest increment is a dedicated cross-origin permission/cookie fixture and consent queue/non-modal request review, while keeping the current modal fallback. Screen capture should remain unavailable until source selection and cancellation are implemented and verified.
