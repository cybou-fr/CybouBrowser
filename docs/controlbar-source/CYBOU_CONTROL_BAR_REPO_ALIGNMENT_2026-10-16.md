> **HISTORICAL DESIGN / AGENT SOURCE — NOT CURRENT RELEASE STATUS.** CYBOU now declares **0.3.0-alpha.1** (unqualified alpha). Any "0.7", "0.8", tested or implemented statements below concern their dated snapshot and do not override [VERSIONING](../VERSIONING.md), [ROADMAP](../ROADMAP.md) or [SECURITY_VERIFICATION_FRAMEWORK](../SECURITY_VERIFICATION_FRAMEWORK.md).

# CYBOU Control Bar v1 — аудит реального репозитория и согласованный план

**Дата:** 2026-10-16 16:25 CEST (контекст проверки)  
**Repo:** https://github.com/cybou-fr/CybouBrowser  
**Reference branch:** `main`  
**Verified HEAD:** [`d2f1a99c9d5ac7af94b50966ae260fe58317d7b4`](https://github.com/cybou-fr/CybouBrowser/commit/d2f1a99c9d5ac7af94b50966ae260fe58317d7b4) (`layout update`, 2026-10-16 14:26:05 UTC)  
**Audit basis:** GitHub connector; full README, CMakeLists.txt, src/main.cpp, src/browserstore.h, qml/Main.qml, SettingsPanel.qml, NavButton.qml; repo recursive tree and commit metadata. **No local build/runtime tests executed**.  
**Status:** review and implementation plan ONLY — no repo mutations.

## 0. Authority and differences from the earlier specification

Use `CYBOU_CONTROL_BAR_V1_AGENT_SPEC.md` as the UX/behavior contract, supplemented by THIS file for the **actual 0.3.0 codebase**. If the earlier spec says to start a new foundation, use C++20 immediately, replace the browser shell, or implement a new parallel profile system, **this alignment document takes precedence**:

- Actual code uses **C++17**, `Qt 6.9+`, `Qt Quick`, `Qt WebEngineQuick`, `Qt Network`; README states tested **Qt 6.12.0 + MSVC 2022**. Preserve C++17 for Control Bar; C++20 upgrade is an independent later ADR, not a feature dependency.
- `Main.qml` is the existing working shell with draggable frameless windows, tab strip, navigation, per-tab `WebEngineView`, site permissions, normal/private profiles. **Extend it rather than replace it**.
- The release-defining feature is one **thin, continuous transparent/glass icon rail** floating in the web-content viewport, repositionable `top/right/bottom/left` via its **last dock/position icon**. No CYBOU branding or ellipsis inside rail, no changing browser tabs/omnibox layout and no docked external sidebar.
- Hover popover must close on pointer leaving icon **and** card (with grace delay) but must not accidentally resolve a pending permission. Click pins where needed.
- Work in short increments directly on **`main`** (user preference). No required PRs, long-lived branches or CI; run local smoke tests instead.
- A previous Page Passport or general tracker dashboard must **not** block this project. Reuse telemetry only when it supports reliable rail behavior.

## 1. Verified architecture (exact entry points)

| Path | Verified implementation | Reuse and action |
| --- | --- | --- |
| [`CMakeLists.txt`](https://github.com/cybou-fr/CybouBrowser/blob/d2f1a99/CMakeLists.txt) | `C++17`, Qt `Quick QuickControls2 WebEngineQuick Network`, QML module `Cybou` with 3 QML files | Register new QML components, new C++ sources; explicit WebEngineCore for interceptor phase; optional Qt Test component later. No broad build migration. |
| [`src/main.cpp`](https://github.com/cybou-fr/CybouBrowser/blob/d2f1a99/src/main.cpp) | Qt WebEngine initialization, `BrowserStore` context property, private window factory, loopback smoke fixture, `--smoke-test` | Register `ControlBarController` only when built; expand fixtures and smoke phases without altering basic boot path. |
| [`src/browserstore.h`](https://github.com/cybou-fr/CybouBrowser/blob/d2f1a99/src/browserstore.h) | QSettings bookmarks/history/session/global preferences; profile config; `QWebEngineCookieStore::setCookieFilter` with global third-party policy; cache/cookie cleanup; origin helper and permission naming | Keep as store/prefs facade. Add panel placement preference and a composition point; delegate complex per-site policy/events to separate C++ files. Do not install two independent cookie filters. |
| [`qml/Main.qml`](https://github.com/cybou-fr/CybouBrowser/blob/d2f1a99/qml/Main.qml) | ~600 lines. `ApplicationWindow` with header tabs/navbar, Repeater of `WebEngineView`, start page overlay, `askPermission()`/`resolvePermission()`, one `pendingPermission`, settings and library Drawers, frameless edge resize handlers | Create rail overlay in `contentItem`, **after web/start-page in stacking order**, before/above web visuals; preserve header and resize hit zones. Bridge existing permission lifecycle to controller. Avoid further 500-line append. |
| [`qml/SettingsPanel.qml`](https://github.com/cybou-fr/CybouBrowser/blob/d2f1a99/qml/SettingsPanel.qml) | Stored site permission inventory via `listAllPermissions`, reset; global third-party cookie toggle and profile cleanup | Reuse existing reset/list API. Synchronize rail/model when preferences change and reset occurs. |
| [`qml/NavButton.qml`](https://github.com/cybou-fr/CybouBrowser/blob/d2f1a99/qml/NavButton.qml) | Basic standard text button + tooltip | Leave for browser chrome. Rail gets purpose-built SVG/glyph component. |
| [`assets/`](https://github.com/cybou-fr/CybouBrowser/tree/d2f1a99/assets) | app icon and Windows icon resources | Keep unchanged. Rail has no logo. |

**Existing normal profile:** `WebEngineProfilePrototype` with `storageName` and `persistentPermissionsPolicy: StoreOnDisk`. **Existing private windows:** distinct in-memory `WebEngineProfilePrototype` instances (`offTheRecord`) created per window; must remain independent. Existing smoke tests explicitly test their destruction.

**Current safeguards:** rejects TLS certificate errors, disables mixed insecure content and local-file access from web pages, only opens user-initiated HTTP(S) new-window requests, denies unsupported/background permission requests. Do not weaken them.

**Current incomplete product areas:** unsigned/unverified public release pipeline, no updater/installer, no credential UX/passkey assessment, no advanced site-data deletion, no native notification presentation, no guaranteed screen-capture handling, no independent security audit. Existing README does not claim those are complete.

## 2. Concrete gaps, rooted in code

1. `Main.qml:36-60`: one modal `pendingPermission`; no authoritative per-(profile, requesting origin, capability) event and policy model. A second request is denied. Preserve this safe-deny behavior until a queue/dispatcher is validated.
2. `Main.qml:51-54`: hardcoded `[1,2,3,6,7,8,9]` excludes screen capture (`4`, `5`); numeric permission enums fragile. Convert to named `WebEnginePermission.PermissionType.*`, implement a screen-capture-specific user-flow only after a Qt 6.12 test.
3. `Main.qml:405+` `settings.javascriptCanOpenWindows: true`, but new windows are accepted only if `request.userInitiated`; an icon saying `Pop-ups enabled` would be **misleading**. Define effective (settings + onNewWindowRequested policy) before enabling an icon.
4. `Main.qml` creates per-tab `WebEngineView` under shared profile. Site-wide persistent grants are **origin/profile-scoped**, not inherently per-tab. Rail follows active tab; underlying grant may affect any tab in that profile with the same origin.
5. JavaScript and popup settings are **per view**. Current app does not store per-origin JS or popup policy or apply it when navigating/reopening/moving tabs. Policy lookup must occur before the next page loads; changes involving already executed scripts require reload/new document.
6. `browserstore.h:applyCookieFilter()` is a **single profile-wide filter** capturing a global bool. Per-site exceptions require composing the global rule and top-level/third-party rules inside one filter, with immutable thread-safe snapshots and proper lifetime.
7. No `QWebEngineUrlRequestInterceptor`, no request attribution, no meaningful network counter, no tracker blocking. It must be a separate phase with performance and attribution gates; **not** fake graph or fake `7 requests`.
8. `clearBrowsingData()` clears cookies/cache/history/permissions/session, not localStorage, IndexedDB, service workers and all origin storage. Do not add an unconditional `Effacer toutes les données du site` claim.
9. QML already contains a `Drawer` on right for Settings/library, plus edge-resize mouse zones at z=1000. Rail must be layered above page but **not override browser resize strip nor the page scrollbar interaction track**. Clickability and hover cards require deliberate stacking/focus test.
10. Tab `index` values are mutable on move/close. Use a stable tab ID (`QUuid` or explicit monotonically unique ID) or safe `QObject*` view binding for events; don't log into the wrong tab on `tabs.move()`.

## 3. Release content: capability map and truthful statuses

**User-facing order:** `camera · microphone · screen · geolocation · notifications · clipboard | JavaScript · pop-ups | cookies · site storage · network activity | move panel`.

| Icon | Existing code | V1 implementation and actual limits |
| --- | --- | --- |
| Camera | Request event handled and direct grant/deny dialog | `REQUESTED` amber pulse; active-policy ask/block; user grants on live request only; never infer `RECORDING` merely from grant. |
| Microphone | Same | Same, combined camera/mic request must map to both linked icons and a coherent consent action. |
| Screen | Existing enum whitelist denies 4/5 | Phase 2 supported request UX after a real 6.12 capability test; keep gray/unavailable if unverified; never silently bypass capture picker. |
| Geolocation | Request + Qt persistent permission | Read effective per-origin state; allow/deny/reset with explicit user action; distinguish requesting iframe origin. |
| Notifications | Request + Qt stored permission; no native notification presentation | Permission controls work; native notification delivery to desktop is a **separate missing backend**. Explain limitation until implemented. |
| Clipboard | Request type handled | Explicit permissions; app/OS constraints may apply. Do not claim all clipboard API reads observed. |
| JavaScript | Qt WebEngine setting available but unbound | Apply per-origin policy to every relevant view; request confirmation/reload. No already executed code 'stopped' claim. |
| Pop-ups | `javascriptCanOpenWindows=true` plus userInitiated gate | Current effective behavior: user-initiated popups are tabs; unsolicited ones rejected. Add explicit per-site policy only when both gate and setting match shown status. |
| Cookies | Global third-party filter | V1 show effective first/third-party policy, optional verified observed cookie changes; per-site exceptions need combined filter. No unverified total cookie count. |
| Storage | localStorage currently allowed; clear command incomplete | Phase 1 policy control/indicator only if probed; true all-storage count/per-site wipe deferred until robust API + test; status `Limited visibility`. |
| Network | Not implemented | Phase 3 interceptor: bounded domain/request sample, `observed only`; site attribution only when reliable, otherwise label as profile activity; never claim body/content seen. |
| Position | Not implemented | Last icon, four-side menu, immediate resize/reflow and persisted preference; **no ellipsis**. |

**Critical semantics:** policy color ≠ request observation ≠ proven current device use. Persistent permissions do not always produce a new `permissionRequested` event. For ephemeral permissions (`camera`, `microphone`, `screen`), `queryPermission` cannot be used for arbitrary pregrant; implement CYBOU-level ASK/DENY policy and resolve actual request objects when they arrive. On revocation, do not promise existing streams end immediately—close/reload tab if needed.

## 4. Layout implementation based on Main.qml 0.3

- Add `ControlBar.qml` as a single component under `window.contentItem`, at visual stacking above `Repeater/WebEngineView` **and above the about:blank start page**. Do not add it into `header: ColumnLayout`; do not change `chromeRowHeight=36` or browser tab/toolbar structure to accommodate it.
- Bar = single glass capsule (translucent navy, hairline border, subtle highlight), thickness **32–36 logical pixels**, glyphs 17–19 pixels, no isolated button tiles except active hover. Prefer reliable translucent fallback over expensive/unavailable true backdrop blur (Qt Quick blur of the webpage may require costly texture capture).
- Top: x centered, y~8 px within content viewport. Bottom: x centered, bottom~8 px. Left: x~8 px. Right: x at ~12–18 px from viewport edge, leaving a **clickable** page scrollbar track; appearance nearly flush. Four placements use **same data model** and one QML component, not four duplicated bars.
- When viewport too narrow/short, icons scroll/overflow **within** capsule; final placement button remains visible and accessible. No overlay of address bar, window buttons, native resize borders.
- Details card is **another browser-owned overlay/popup** anchored to hovered icon; for right rail place to the **left**, for left rail to the **right**, for top place below, for bottom above. Clamp to viewport including small windows; one card at a time.
- Hover-to-show delay ~160ms; combined button+card hover region and ~200ms delayed close; pinned click/keyboard mode; Escape/outside closes; do not make a permission decision on mouse-out. Keyboard access/labels, 125–200% DPI, reduced motion.
- Animation: two amber pulses maximum for a verified request, static request dot afterward; blue live-use indicator **only if verified**; no infinite flashing or decorative fake event colors.
- `position` is globally persisted as benign UI preference through existing `BrowserStore` `QSettings`; never persist private-window event logs. New private windows inherit placement preference but not content/profile data.

## 5. Proposed source layout (minimize Main.qml growth)

```text
src/
  browserstore.h                # existing; QSettings, profiles, filters facade
  controlbar/
    ControlBarController.h/.cpp # active context, interface to QML, state-model adapter
    CapabilityModel.h/.cpp      # QAbstractListModel / fixed icon catalog / semantic roles
    SitePolicyStore.h/.cpp      # scoped policies, explicit persistence/normal-private behavior
    PermissionBridge.h/.cpp     # optional after spike; do not promise direct QML value marshalling
    RequestObserver.h/.cpp      # phase 3 QWebEngineUrlRequestInterceptor
qml/
  Main.qml                      # small integration hooks only
  SettingsPanel.qml             # existing; sync changes/reset
  ControlBar.qml                # 4-orientation rail and overflow
  ControlBarIcon.qml            # focus/hover/state icon with accessible name
  CapabilityPopover.qml        # one safe hover/focus detail card
  ControlBarPositionPicker.qml  # 4 placements, final button
  ControlBarTheme.qml           # tokens; use QML singleton where supported
assets/controlbar/             # vector icons, NO logos
```

**Incremental tactic:** First integrate a QML-only rail with a plainly marked **demo-only fixture** in smoke mode, reusing current QML `pendingPermission` and active profile. Only after tested UX and accessibility work, centralize logic in C++ models. Avoid premature mass rewriting of `Main.qml` and `browserstore.h`.

`CapabilityModel` suggested roles: `id, icon, titleFr, state, stateReason, observedEvent, eventTimestamp, eventOrigin, eventTabId, eventConfidence, available, pending, reloadRequired, effectiveScope, actionAvailability, helpText`. Strict invariant: each event has verified source + timestamp and scope. Do not set state=ALLOW until operation succeeds.

## 6. Detailed implementation sequence — each is a small main-branch change

### P0 — baseline + API capability matrix (no UI regression)
**Touch:** `docs/API_CAPABILITY_MATRIX.md`, optional test fixtures; existing code unchanged unless bug found.  
**Actions:** pin Qt kit and Chromium runtime version; run existing `--smoke-test` and record output/screenshots; test Qt 6.12 capture/permission enum mapping, persistent vs temporary, QQuickWebEngineProfile interceptor, hover overlay z-order; record actual observable/controllable/missing per icon.  
**Gate:** baseline smoke PASS; all unsupported items identified and feature-flagged. **Do not assume tested just because README says tested.**

### P1 — visible Control Bar shell (working UI, fake data only in test harness)
**Touch:** `qml/ControlBar*.qml`, `Main.qml` integration, CMake QML_FILES and resources, BrowserStore placement property.  
**Actions:** all 4 positions, thin single capsule, last position button, hover/popover with non-trivial mouse-out gap handling, non-modal click pin, touch/keyboard, focus, responsive overflow, QSettings persistence. Display neutral/unknown for functions not wired. No security-action simulation in normal build.  
**Gate:** screenshots each position incl. 820×560 minimum window, browser tabs/nav/scroller/resize still work; no extra page layout space; position survives restart.

### P2 — real permission policy + request events
**Touch:** new `CapabilityModel`, optional `PermissionBridge`, `Main.qml` ask/resolve hooks, SettingsPanel sync, smoke fixture.  
**Actions:** replace magic numeric values with enums; connect real `onPermissionRequested`, origin details, no background surprise-grant, explicit choices; ensure behavior for invalid request/tab-close/tab-switch/navigated origin; handle combined A/V; persistent read/allow/deny/reset supported types; camera/mic/screen ASK/DENY with on-demand grant and verified limitations; store bounded memory-only event history, never in ordinary browsing history.  
**Gate:** loopback tests: allow/deny/reset, mixed origins, tab move, private separation, screenshot/card states; no grant to changed tab or different origin. **Keep old modal fallback until no regressions.**

### P3 — script/popups/site-policy engine
**Touch:** SitePolicyStore, Main.qml WebEngineView settings bindings, popover actions, optionally SettingsPanel.  
**Actions:** normalized scheme/hostname/port key; JS policy per-origin applied on nav/load before scripts execute (spike verified), reload required on change; popup policy consistent with `onNewWindowRequested` + `javascriptCanOpenWindows` current gate; do not confuse current tab policy with origin/profile global; safe browser workflows.  
**Gate:** two tabs different sites, same site across tabs, move/close/reopen, reload persists JS choice, no policy leaks; popup test verifies blocked and allowed flows.

### P4 — cookies and site storage (honest v1 subset)
**Touch:** BrowserStore cookie facade, SitePolicyStore, QML popovers, fixture.  
**Actions:** compose global third-party rule + optional per-top-level-site exception in **one** `setCookieFilter()` per profile; never drop existing global block silently. Show cookie policy and verified observed events, not 'all cookies saved' unless enumeration works. Site storage may show a toggle/limited visibility, but do not advertise per-site full wipe while IndexedDB/Cache Storage/service worker cleanup is unproven.  
**Gate:** first-party vs third-party fixtures, unrelated tab/site isolation, private separate, clearAsync state and completion semantics; no silent cookies leak.

### P5 — network observation (attribution gate)
**Touch:** `RequestObserver` in `src/controlbar`, `BrowserStore::configureProfile` install point, QML network popover, CMake explicit Qt::WebEngineCore.  
**Actions:** profile-scoped `QWebEngineUrlRequestInterceptor` with no I/O/blocking logging inside callback; capture bounded metadata (`requestUrl`, `firstPartyUrl`, `initiator`, resource type, timestamp) without sensitive querystrings, passwords, request bodies. Aggregate via queued worker signals to UI, use bounded ring buffer and upper domain cap; show per-origin/top site only when supported. Otherwise label `Activité du profil` and no tab count claims. Avoid claiming network responses, data exfiltration, data types, or all traffic.  
**Gate:** local fixture 1st/3rd-party resources + two simultaneous tabs, no mixed attribution; benchmark many subresources, not worse browsing under moderate load.

### P6 — UX polish + regression + truthful beta boundary
**Touch:** tests, docs, localized UI strings, QML animations/accessibility, no broad rewrite.  
**Actions:** light/dark/glass fallback, FR strings + RU translation, reduced-motion, 100/125/150/200% DPI, touch and keyboard, private profile release, fullscreen video, settings/library drawers, page scrollbars, Windows frameless resizing, failure/retry; local smoke integrated. Produce release checklist separating browser functional baseline from public security readiness.  
**Gate:** no production fake icon events; no uncaught permission grants, test results documented. Public beta separately gated on patched Qt Chromium engine, signed installer and working updater + security review.

## 7. Acceptance scenarios specifically derived from existing code

- [ ] Launch ordinary window; restore tabs; rail above **web content** and also above `about:blank` start page; it does not replace logo/start page or header.
- [ ] Positions top/right/bottom/left switch without reload, scroll jump or lost tab; last control never becomes `...`.
- [ ] Right rail close to edge, but webpage scrollbar and 5px native resize hit zones remain usable. Open Settings and Library `Drawer` without overlay trapping.
- [ ] Hover geolocation; move into popover, press allow/deny/reset; no auto-close before click; mouse-out hides. Pending request stays pending unless user chooses or explicit safe timeout.
- [ ] Background tab request is not silently allowed; iframe origin shown separately; tab switch or navigation invalidates stale pending grant.
- [ ] Request screen capture (Qt enum types 4/5) gets correct supported-safe handling or truthful unsupported indicator—not false 'Allowed'.
- [ ] A camera permission granted once does **not** turn into a fictional permanently saved grant or claim camera is actively recording.
- [ ] Notifications saved grant is visible and reset via existing SettingsPanel; Control Bar updates immediately. Native notifications must not be claimed until implemented.
- [ ] Two sites, same profile: disable JS on A without changing B. Reload A and preserve policy; show 'reload required' while current document has already executed JS.
- [ ] Cookies first-party/third-party filter and existing checkbox do not conflict; tests record allowed and denied cases.
- [ ] Private window A + private window B have independent profiles and isolated events; cleanup on close; no telemetry/settings leak except explicitly global cosmetic position preference.
- [ ] Network counter limited to supported observed requests; display `Profil` instead of `Onglet` when impossible to attribute exactly. No invented transfer size/contents.
- [ ] Existing `--smoke-test` succeeds before and after each phase; record whether it was actually run. No CI obligation; use local MSVC/Qt.
- [ ] View full-screen/site modal behavior, multi-DPI, minimum window, and app keyboard shortcuts unchanged.

## 8. Risks and dependencies that may change sequencing

**Highest architectural risk:** site-level policies in a browser where Qt engine objects have mixed scopes (profile-wide storage/network vs view-wide JS vs origin-based permissions). Build `SiteContext` around `(profile identity, top-level origin, requesting origin, stable tab ID)` and never treat these as interchangeable.

**Hover UI risk:** `Popup`/native Chromium rendering/frameless window interactions. Spike and verify z/focus/mouse corridor before building dozens of buttons. Avoid expensive webpage capture for 'true' blur.

**Security risk:** inadvertently re-granting cameras or accepting screen capture on a stale request; always check pending object validity, requesting origin, active foreground view, and profile. Preserve safe deny on uncertainty.

**Performance risk:** synchronous request interceptor. Qt warns that interceptor stalls URL requests and main-thread profile method calls while running; decisions must be O(1) with immutable snapshots, bounded event queue, no synchronous QML/QSettings/file I/O from callback.

**Operational risk:** public release needs patch tracking for Chromium/Qt, signed installer, working automatic updates and threat/credential gap analysis. None are implemented per README; Control Bar success alone does not make safe public distribution.

## 9. Agent reporting contract, after EACH phase

Report (a) commit SHA(s) on `main`; (b) exact changed files; (c) capabilities backed by real Qt event versus neutral/unsupported; (d) tests actually executed with outputs; (e) screenshots of 4 placements; (f) breaks/regressions/limitations; (g) next smallest increment. Do not claim pass without running.

## 10. Repositories and official Qt technical references

**Verified repository at audit:**
- https://github.com/cybou-fr/CybouBrowser/tree/d2f1a99c9d5ac7af94b50966ae260fe58317d7b4
- https://github.com/cybou-fr/CybouBrowser/blob/d2f1a99c9d5ac7af94b50966ae260fe58317d7b4/qml/Main.qml
- https://github.com/cybou-fr/CybouBrowser/blob/d2f1a99c9d5ac7af94b50966ae260fe58317d7b4/src/browserstore.h
- https://github.com/cybou-fr/CybouBrowser/blob/d2f1a99c9d5ac7af94b50966ae260fe58317d7b4/README.md

**Qt API references, verify against installed 6.12 kit before using:**
- https://doc.qt.io/qt-6/qwebenginepermission.html
- https://doc.qt.io/qt-6/qml-qtwebengine-webengineprofile.html
- https://doc.qt.io/qt-6/qwebengineurlrequestinterceptor.html
- https://doc.qt.io/qt-6/qwebengineurlrequestinfo.html
- https://doc.qt.io/qt-6/qwebenginecookiestore-filterrequest.html
- https://doc.qt.io/qt-6/qml-qtwebengine-webenginesettings.html

**Confidence:** repository and Qt API descriptions inspected, but actual MSVC build, UI screenshots, Qt 6.12 behaviors and runtime performance **not tested in this audit**.
