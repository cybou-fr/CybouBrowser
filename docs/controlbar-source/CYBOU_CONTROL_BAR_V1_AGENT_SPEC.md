# CYBOU Browser — Control Bar v1.0

**Implementation specification for coding agent**  
**Status:** approved UX direction; implementation is not claimed to exist  
**Date:** 2026-10-16  
**Target:** Windows 11 first; modern C++20, Qt 6 / Qt Quick / QML / Qt WebEngine, CMake  
**Product priority:** everyday browser + one distinctive feature, CYBOU Control Bar  
**UI languages:** French primary, Russian translation; localizable from day one

> **Product promise:** See, understand, and control what a website can request, use, contact, and retain — without opening DevTools or digging through browser settings.
>
> **Engineering invariant:** Never display a security claim, site-specific count, recording state, or successful policy change that has not been verified by a corresponding supported signal or operation.

---

## 0. Non-negotiable design decisions

1. The Control Bar is **one continuous, slim, nearly transparent, frosted-glass strip of icons**. It is not a full dashboard, multi-row toolbar, left navigation menu, or permanent wide sidebar.
2. The strip **floats above the webpage inside the content viewport** and must not consume layout space, push/reflow site contents, or overlap native browser tabs, address bar, browser menu, or page scrollbar track. It is drawn in trusted browser chrome as an overlay, not injected into the webpage DOM.
3. The same component supports **four placements**: `top`, `right`, `bottom`, `left`. This is a user preference, switchable immediately without a page reload, preserved on restart.
4. The bar's **last icon is a placement control** (directional/arrows/dock symbol), **not `...`**. Activating it opens a small four-position picker. Never hide this picker inside the page or browser's general settings.
5. **No CYBOU logo or CYBOU wordmark inside the bar**. Only icons, subtle dividers, state marks, and detail overlays. Product branding may exist elsewhere in the browser, not here.
6. Icon colors and subtle animation show **permission policy versus detected activity as distinct layers**. Never equate "requested" with "granted", "granted" with "currently in use", or "observed network request" with "personal data definitely transmitted".
7. Hover over an icon reveals a **compact glass popover** with actionable details. When the pointer leaves both icon and popover, close after a small grace interval. Don't close as the pointer crosses the gap or while keyboard focus remains inside. Clicking can pin the popover for keyboard/touch operation; Escape dismisses it.
8. No destructive or high-risk state change on hover. Requests to use sensitive hardware must not silently gain a permanent grant from a casual click. Every granted permission must correspond to an explicit decision.
9. Default behavior: avoid page breakage. Normal browsing must work on typical banking, messaging, shopping, video and government sites. Disabling JS/storage/etc. is deliberate and reversible.
10. A proof-of-behavior test harness is mandatory: fake event glows in a mockup are not a shippable implementation.

**Revision versus old planning:** older documents centered the launch on a "Page Passport" and general tracker reports. Those may later consume the same telemetry layer, but **the public v1.0 flagship is Control Bar**. Do not build an elaborate Passport screen as a prerequisite.

---

## 1. Why this exists / user journeys

**U1 — passive awareness:** Open a website; the strip immediately shows which permissions are currently `ask`, `allow`, or `deny`, plus verified recent events. No tutorial required.

**U2 — permission request:** An embedded map asks for location. Location icon briefly glows amber; its popover names the **requesting origin**, what is being requested, and offers "Allow once", "Always allow" where persistence is supported and meaningful, and "Block". If origin belongs to an iframe, clearly distinguish it from the top-level website.

**U3 — revoke:** User sees "microphone blocked" or "camera allowed". They can change policy and see a confirmed state (or a pending/reload-required state). Browser must honestly state when ongoing capture cannot be forcibly ended without reload/tearing down the page or stream.

**U4 — script control:** User disables JavaScript for a site. UI explains the required reload, then applies policy to that tab/site on subsequent visits without accidentally disabling JS in unrelated tabs.

**U5 — site data:** User sees whether first/third-party cookies and local storage are permitted and whether relevant cookies were observed. Actions include review and targeted removal **only to the extent the engine APIs actually support**. Avoid a deceptive "wipe everything from this site" button.

**U6 — network visibility:** A network icon shows observed outgoing request/domain activity. The user can inspect a small bounded list with timestamps and resource type. It is not a packet sniffer and cannot reliably identify all personal information or all browser traffic.

**U7 — reposition:** The user clicks the last icon, chooses top/right/bottom/left, and the very same strip changes orientation at once. The choice persists after changing tabs/windows and restarting (defined scope below).

**U8 — quiet browsing:** With no new events, the strip is nearly invisible, dimmed but readable. It must not blink forever, steal keyboard focus, or dominate the webpage.

---

## 2. Positioning / geometry / visual tokens

### 2.1 Common appearance

- Continuous rounded glass capsule, no detached square button backgrounds at rest.
- Primary material: navy tinted transparent acrylic, e.g. `rgba(13,37,71,0.32–0.50)` with background blur where supported; hairline border `rgba(255,255,255,0.14–0.25)`; shadow low and diffused.
- Fallback without blur or on low-performance systems: slightly more opaque acrylic; maintain contrast.
- Palette: sovereign navy `#0D2547`, white `#FFFFFF`, French red `#E11D2E`; semantic hues are allowed (green, amber, cyan, violet) to indicate behavior and status, not to repaint brand identity.
- Suggested bar thickness **32–36 logical px**; glyphs 17–19 px; minimum interactive hit target **32×32** logical px with optional invisible larger touch target where spacing allows; outer edge radius 15–18 px.
- Icon pitch 32–36 px; 1px translucent vertical/horizontal dividers only between conceptual groups. Never individual button cards except hover/active focus.
- A subtle glow/outline may appear for a requested or active capability; do not leave a permanent full-saturation neon glow everywhere.
- Responsive orientation, scaling at 100%, 125%, 150%, 200% DPI. Respect Qt device pixel ratio.
- Do not enlarge the entire panel simply to fit labels. **No always-visible text labels**; tooltips and expanded popovers carry text.

### 2.2 Placements (one component, one data model)

**TOP:** floating **within web viewport**, approximately 8 px below the viewport's top (i.e. below browser chrome); horizontally centered or aligned slightly to the right depending on width and content, but not overlapping tabs/omnibox. Auto shorten or overflow if viewport narrows.

**RIGHT:** floating **inside viewport**, approximately **6–10 px inward from the usable content right edge**, to the **left of the native page scrollbar/scrollbar interaction track**. This is almost flush with the window, but visually above the page, not attached to a separate browser sidebar. Vertical capsule aligned near viewport top or centered, clamped to visible area.

**BOTTOM:** floating **inside viewport**, about 8 px above usable content bottom; horizontal capsule.

**LEFT:** floating **inside viewport**, about 6–10 px inside usable content left edge; vertical capsule.

**Never** move control bar to the title bar, tab strip, omnibox, browser's native side panel or page DOM. The overlay is positioned relative to the actual web-content viewport. Maintain independent clipping/z-order and do not let fullscreen video or permission dialogs randomly reorder controls; define fullscreen policy separately.

When vertical height is insufficient, use a scrollable icon section **inside the bar** with unobtrusive fade edges. The placement button remains fixed and always visible. When horizontal width is insufficient, likewise scroll/overflow the icon portion while keeping the placement button. Do not forward wheel scrolling over the control strip to the webpage unless pointer is outside the strip.

### 2.3 Position control

The **final icon** is a neutral dock/reposition symbol: e.g. a square frame with a four-way arrow or an orientation glyph, NOT three dots.

- Hover: tooltip `Position du panneau`.
- Click: four-option radial/compact popover, labeled only there: `Haut`, `Droite`, `Bas`, `Gauche`; each has a miniature placement diagram.
- Selecting: change orientation immediately using a short optional transition of ~120–180 ms; don't animate a large overlay flying across the webpage.
- Persist as window-independent global preference (`ui.controlBar.position`) unless product explicitly introduces per-window overrides. New windows inherit global preference.
- Placement must not alter site permissions, active tab, or scroll position.

---

## 3. State and animation model

### 3.1 Separate policy from observations

**Policy** (the requested behavior when capability is attempted):
- `ASK` — no persistent explicit decision / requires user decision; neutral gray/white glyph.
- `ALLOW` — allowed under effective scoped policy; green visual marker.
- `DENY` — denied under effective scoped policy; red visual marker, strike/slash optional.
- `UNAVAILABLE` — capability unsupported or cannot truthfully be changed in current context; muted glyph + `?`, disabled action.
- `APPLYING` / `RELOAD_REQUIRED` / `ERROR` — transitional state with explicit text in popup; don't show success optimistically.

**Observation** (orthogonal; may coexist with any policy):
- `REQUESTED` — a verified permission request arrived; short amber pulse (maximum two pulses ~0.6–0.9 s) and dot until handled or marked recent.
- `BLOCKED_ATTEMPT` — a verified request was denied by control layer; short red edge flash, timestamped event. Do not infer attempts from a mere denied policy if no request arrived.
- `IN_USE_CONFIRMED` — reliable source confirms ongoing active use; small cyan/green live dot only while true; otherwise no "recording" claim.
- `RECENT_ACTIVITY` — bounded event within recent TTL; static small amber/cyan dot after animation.
- `OBSERVED_COUNT` — verified scoped events for e.g. network, cookies, storage; compact badge or number in popover, not fabricated exact value.
- `UNKNOWN` — observation unavailable or attribution ambiguous; state explicitly and do not generate a fake event.

**User settings** `reduceMotion`, `disableGlow` and high-contrast mode must not destroy meaning: keep badge/text/icon stroke differences. For users with reduced motion, replace pulse with a static amber outline.

### 3.2 Persistence rules

- `ASK/ALLOW/DENY` for persistent permissions are scoped to **scheme + hostname + port (`origin`)**, and to the correct WebEngine profile (normal/private). Not merely to displayed base domain or full URL path.
- An iframe's `requestingOrigin` is not assumed equal to the top-level page origin.
- Temporary permissions are not represented as permanent persistent permissions; retain separate CYBOU user preference `ask/deny` if required, and handle at actual request time.
- `lastObserved` events belong to the proper scope only when reliably attributed. Clear/re-label them on navigation/tab change. They do **not** become permissions.
- Panel placement global; per-origin policies persistent where explicitly supported, with normal vs private storage isolation.
- Profile-global controls (e.g. cookie filtering) **must not be mislabeled per-tab/per-origin** unless a proven origin-aware policy engine enforces that granularity.

### 3.3 Status tooltips

Every tooltip must contain (a) title, (b) policy or "monitor only", (c) a clear event label when available, (d) potential effect of clicking, (e) scope (`ce site` / `cet onglet` / `profil`). For example:

> Caméra · Bloquée · Demande récente de `https://video.example.fr` · Cliquer pour gérer l'accès.

Never show `Caméra active` unless actual usage is reliably observed. Don't use amber for a merely enabled capability.

---

## 4. Interaction specification / finite state machine

### 4.1 Hover-to-popover mechanics

- `POPOVER_CLOSED` → `OPEN_PENDING` when pointer enters icon; after approximately **140–200 ms** open popover and cancel conflicting tooltip.
- `OPEN` while pointer within icon **or** panel popover (combined hover region). A small bridge/hover corridor must avoid a false mouse-out gap between icon and popup.
- On leaving both: `CLOSE_PENDING`, delayed **160–240 ms**; return to icon/popup cancels close. Then close softly in 90–130 ms.
- Moving directly from one icon to next opens/switches content, preserving a single popover instance instead of stacking.
- Click or keyboard activation may `PIN_OPEN`; pinned panel closes on Escape, clicking outside, focus leaving popup (if not processing a dialog), selecting another icon or explicit close. On touch, tap opens / second tap closes.
- Do not close when the user moves from card title to toggle/menu via mouse; do not close while context-menu/picker is open or while an interactive subdialog has focus.
- When a request is pending and popup hides, **the request remains pending**, with a visible activity dot and a way to return. Pointer leaving must **never imply allow/deny**.
- If the request needs immediate resolution or would otherwise remain open indefinitely, choose an explicit timeout policy (safe default deny/cancel only if required by engine) and communicate it; no silent grant. Verify actual engine behavior.
- On tab switch/navigation, close or rebind the card to the correct scope; do not let a stale popup grant a permission to a different origin.

### 4.2 Click semantics

The prototype's clickable colored icon is not permission to surprise-grant the camera. Define consistent semantics:

- **Hover:** immediate lightweight details and actions, never changes policy.
- **Click on sensitive device/access icon:** opens and pins the popover (so users can select `once`, `ask`, `block`, plus `always` when supported). No immediate grant on icon click.
- **Click on reversible low-risk toggle icon (e.g. JS):** opens the popup by default; an explicit switch in popup changes policy. Avoid one-click breakage of login/payment flows. Consider an optional "quick-toggle" setting only after usability testing.
- **Click on network/storage monitor icon:** opens information card; doesn't claim to block if only observing.
- **Click on placement icon:** opens four-direction selector.

### 4.3 Keyboard / accessibility

- Tab enters bar without stealing focus on every page load; left/right arrow or up/down for roving icon focus; Enter/Space opens or activates; Escape closes and restores focus to icon or webpage.
- Screen reader labels: capability + effective state + current request/status + action; announce critical requests via accessible live region, not entire network event stream.
- Do not rely solely on color. Shapes: allowed check, denied slash, ask question mark, request dot/outline, live concentric dot. High-contrast ring.
- Touch / mouse / stylus supported. Tooltips replaced by tap popover where hover does not exist.
- Respect system reduce-motion and keyboard focus visibility.

---

## 5. v1 feature inventory / honest capability contracts

**Core rail order** (subject to UX tests; use this as initial implementation order):

`camera · microphone · screen · location · notifications · clipboard | JavaScript · popups | cookies · storage · network | position`

The vertical rail displays the same sequence top-to-bottom. Dividers rotate with the rail. The display does not contain text labels until a tooltip/popover is shown. Trackers/fingerprinting/extensions are **not** silently added to v1 unless the corresponding code and QA exist.

### F01 — Camera (`camera`)
- Policy: `ASK`, `DENY`, and scoped one-time allow on an actual `QWebEnginePermission` request. Persistent allow if/when supported and explicitly appropriate. Show requesting origin and scope.
- Detect: actual permission request event. Do **not** claim to detect camera recording duration from the permission event alone.
- Actions: allow current request once, deny current request, set site policy (ask/deny), review available effective grants; advise reload when existing media streams cannot be guaranteed stopped.
- Failure UI: `État d'utilisation non observable` if no reliable active stream source.

### F02 — Microphone (`mic`)
- Same policy model as camera. Recording/"in use" requires verified usage signal, not assumed.
- Separate permissions for camera and microphone; combined requests may require coherent dialog with both scopes.

### F03 — Screen capture (`monitor-up`)
- Typically ephemeral and user-initiated with chooser. Do not silently pre-authorize arbitrary desktop capture or bypass OS/browser picker.
- Show request and current decision. Show `in use` only with a reliable source. Respect `screenCaptureEnabled` and Qt permission behavior per pinned version.

### F04 — Location (`map-pin`)
- Permission may be persistent per origin; offer ask, deny, allow as supported.
- Pending request is amber; allowed permission is not proof GPS/location lookup was completed.
- On unsupported backend explain `Géolocalisation indisponible` rather than green success.

### F05 — Notifications (`bell`)
- Per-origin permission persistent if supported, enable/deny/reset via engine; may display observed permission requests.
- Notifications sent later may be handled by profile's presentation callback; do not count as "active request" unless instrumented.

### F06 — Clipboard (`clipboard`)
- Distinguish clipboard read vs write and legacy setting; Qt uses permission types (e.g. ClipboardReadWrite) plus `javascriptCanAccessClipboard` and `javascriptCanPaste`, depending on path and version.
- Avoid blanket claim "clipboard is blocked" unless all relevant supported paths are accounted for. Consider combined visual with detailed subrows `Lecture` / `Écriture`.
- Guard unsanitized access; no silent broad paste grant.

### F07 — JavaScript (`code-2`)
- Operational policy `ENABLED`/`DISABLED`, with optional site override; default enabled.
- Qt WebEngine `WebEngineSettings.javascriptEnabled` is associated with a view/settings rather than a native persistent per-site switch. Implement a CYBOU policy resolver keyed by origin, applying to appropriate view **before navigation when possible**. Do not change a shared profile-level value that impacts unrelated tabs.
- On switch, mark `RELOAD_REQUIRED`; explicitly request/perform reload according to confirmed action. Previously executed scripts/effects may remain until reload; never promise instant revocation.
- Warn that turning off JS can break login, checkout, apps.
- Do not claim count of inline scripts from network interceptor: external script requests are only a subset.

### F08 — Popups (`panels-top-left`)
- Policy for unsolicited JavaScript windows; default block or ask after testing compatibility.
- Use settings such as `javascriptCanOpenWindows` and `newWindowRequested` where appropriate for opening windows/tabs; distinguish user-initiated click from unwanted popup.
- An event glow requires a reliably intercepted window request; blanket blocked-policy alone is not a recorded attempt.

### F09 — Cookies (`cookie`)
- UI distinguishes: (1) `autoriser / bloquer` first-party, (2) third-party policy, (3) observed cookie entries for current site **when attribution is reliable**.
- The profile cookie store is shared by all pages using that profile. `cookieAdded/cookieRemoved` are profile-wide signals, and `setCookieFilter` operates in Chromium's IO path. Implement origin-aware read-only policy snapshot; do not do blocking lookups, disk I/O or UI callbacks in the cookie filter.
- Do not call `deleteAllCookies()` for a per-site delete action. Enumerate/filter the right cookies with attention to cookie Domain/Path/host-only attributes, including subdomain effects; wait for async evidence before showing removal success. If the required precise operation cannot be validated, label it "Supprimer les cookies correspondants" and disclose scope, or defer.
- Do not use cookie count as direct proof user data was exfiltrated.

### F10 — Site storage (`database`)
- Show whether DOM Local Storage is enabled for the view/site, using `localStorageEnabled` and supported policy layers. When disabling storage affects site behavior, request reload and document limitations.
- **Never equate `localStorageEnabled=false` with blocking/deleting IndexedDB, Cache Storage, service workers, file system access, caches or all stored data.** The cookie filter interacts with some such APIs globally; behavior must be verified.
- v1 popover can say `Stockage local : autorisé`, `Bloqué`, `État détaillé indisponible` as applicable. The exact bytes/items and complete per-site deletion are **out of scope** until validated.

### F11 — Network (`network`)
- Passive monitor and optional verified rule-based block only if implemented. Show "domains observed / requests intercepted" rather than "all connections" or "data transmitted".
- Use `QWebEngineUrlRequestInterceptor` with `firstPartyUrl`, `initiator`, `requestUrl`, `resourceType`, `requestMethod`; note interceptor is installed per profile and may **not reliably attribute every event to an individual tab**. Some requests may not appear, and it does not give complete request/response bodies or final egress.
- When attribution is ambiguous, label `Activité du profil` / `Attribution incertaine` and do NOT show a per-tab exact count.
- Security: do not retain or show raw POST data, tokens, cookies, auth headers, full query strings by default. Aggregate by origin/domain and type. Bound memory and sampling; do not stall Chromium's network thread.
- Rules engine and tracker list are **optional next phase**, not required to make a green "protection active" claim. If tracker blocking not tested, no tracker shield icon.

### F12 — Placement (`panel-top` / `layout-panel-left` etc.)
- Not a security capability; neutral icon, pinned at end. Four positions; preference persisted per app, not per site.
- No `...` icon anywhere in the rail. Other secondary capabilities can live in separate settings, not this icon.

### Deferred / optional beyond v1

- `fingerprint`: active anti-fingerprinting requires comprehensive behavior and compatibility testing; a simple symbol is not enough.
- `tracker shield`: verified list + deterministic blocker + update strategy + false-positive escape hatch required.
- `downloads`: use contextual temporary indicator on actual download events, without permanently growing rail.
- `WebRTC IP`, automatic playback, MIDI/USB/Bluetooth/file access, canvas read, WebGL: candidate follow-on icons only after Qt API audit and specific user value.

---

## 6. Popup information architecture and FR-first copy

Each icon opens **one anchored contextual popover** with the following consistent hierarchy:

1. **Title and glyph** (`Caméra`, `Microphone`, `JavaScript`, `Cookies`, etc.), with compact effective state.
2. **Origin** that is subject to policy; if embedded iframe, identify both requesting origin and top-level page.
3. **Observed event line** (only if verified): `Ce site demande votre position`, or `Demande bloquée il y a 12 s`. If no event: `Aucune demande observée dans cet onglet` rather than `N'a jamais demandé`.
4. **Primary actions**, semantics vary by capability: `Autoriser une fois`, `Toujours demander`, `Bloquer`; non-sensitive switches `Activé/Désactivé` with scope and reload notice.
5. **Consequences** in plain FR (`Ce site pourrait ne plus fonctionner correctement` / `Un rechargement est nécessaire`).
6. **Expandable details** for evidence and site/profile limitations (`Voir les détails`), optionally exact origin and last 5 events.

Normal size: 280–340 logical px wide, content-driven height, maximum usable viewport minus padding; internal scrolling for more information. Opposite-side anchoring:

- Hover TOP rail → popup opens **below** rail;
- RIGHT rail → popup opens **to the left**;
- BOTTOM rail → popup opens **above**;
- LEFT rail → popup opens **to the right**.

Clamp to screen/viewport bounds and never cover native browser chrome. Mouse corridor from anchor to card is part of hover interaction. Cards are overlays **above the webpage**, never embedded in page DOM. Only one card at a time. Protect against malicious page-generated lookalike overlays by consistent trusted browser chrome styling and z-order.

**FR examples** (translations via `qsTrId` / locale catalog):

| Key | French | Russian |
|---|---|---|
| `permission.camera` | Caméra | Камера |
| `permission.mic` | Microphone | Микрофон |
| `permission.screen` | Partage d'écran | Демонстрация экрана |
| `permission.location` | Géolocalisation | Геолокация |
| `permission.ask` | Toujours demander | Всегда спрашивать |
| `permission.allow_once` | Autoriser une fois | Разрешить один раз |
| `permission.allow` | Autoriser | Разрешить |
| `permission.block` | Bloquer | Запретить |
| `permission.unsupported` | Fonction indisponible | Функция недоступна |
| `permission.requested` | Demande d'accès reçue | Получен запрос доступа |
| `permission.use_unknown` | Utilisation non vérifiable | Использование нельзя достоверно определить |
| `controlbar.position` | Position du panneau | Положение панели |
| `controlbar.top` | Haut | Сверху |
| `controlbar.right` | Droite | Справа |
| `controlbar.bottom` | Bas | Снизу |
| `controlbar.left` | Gauche | Слева |
| `controlbar.reload` | Recharger pour appliquer | Перезагрузить для применения |
| `network.profile` | Activité observée dans ce profil | Наблюдаемая активность профиля |

---

## 7. Event schema / policies / attribution

Use **strongly typed C++ enums** with QML-facing typed/enum or stable named roles; no arbitrary status strings as business logic.

### 7.1 Suggested types (conceptual; adapt to actual project conventions)

```cpp
enum class Capability {
  Camera, Microphone, ScreenCapture, Geolocation, Notifications,
  Clipboard, JavaScript, Popups, Cookies, LocalStorage, Network,
};

enum class PolicyState {
  Ask, Allow, Deny, Unavailable, Applying, ReloadRequired, Error
};

enum class ObservationState {
  None, Requested, BlockedAttempt, ActiveVerified,
  RecentVerified, Unknown
};

enum class EvidenceKind {
  PermissionRequest, PermissionDecision, NetworkRequestIntercepted,
  CookieObserved, PopupRequested, DownloadRequested,
  ActualUsageVerified, PolicyApplied
};

struct SecurityOrigin {
  QString scheme;
  QString host;
  quint16 port; // canonical effective port / explicit consistent normalization
};

struct CapabilityState {
  Capability capability;
  PolicyState policy;
  ObservationState observation;
  SecurityOrigin subjectOrigin;
  SecurityOrigin requestingOrigin;
  QString profileId;
  QString tabId;
  bool evidenceScopedToTab = false;
  bool canChangePolicy = false;
  bool requiresReload = false;
  qint64 lastObservedMs = 0;
  QString explanatoryReason; // localizable id, NOT arbitrary website text
};
```

Potential store shape: `QAbstractListModel` with roles: `capabilityId`, `iconId`, `policyState`, `observationState`, `tooltipId`, `hasRecentRequest`, `supported`, `canAct`, `subjectOrigin`, `pendingCount`, `sourceConfidence`, `reloadRequired`.

**Never** assign the permission requested by `iframe.foo.example` to the top-level domain merely because it appears visually inside its page. Origin is scheme + hostname + port; private browsing has separate profile state.

### 7.2 Evidence classification

- `ObservedDirect`: a Qt signal/API provides this exact action for this origin/tab.
- `ObservedProfile`: exact action is known but only profile scope can be guaranteed.
- `Inferred`: derived from a heuristic or domain rule, **not** shown as precise fact.
- `Unknown`: no supported observability.

Badge thresholds must be based on direct/profile evidence, with explicit scope. No fake count of blocked third parties, no assumption that an allowed permission produced a device capture, no claim that localStorage entries are visible absent API.

### 7.3 Observation ring buffer

- Ring buffer capped per profile (e.g. <= 1,000 entries total, ≤ 20 per capability/origin visible) with TTL, bounded URLs, no sensitive payloads.
- UI event updates coalesced at e.g. 100–250 ms, preserving critical permission prompts immediately. Normal network events must not trigger animation floods.
- Keep recent-events memory only by default; persist policies and placement, not detailed browsing telemetry. Opt-in diagnostics separate.
- Clear current-tab transient badges upon cross-origin navigation (or move events to profile-aggregate view when attribution confidence falls).

---

## 8. Architecture: C++ authority, QML presentation

**Suggested modules (illustrative, adapt to existing repo rather than creating duplicate architecture):**

```text
src/
  browser/
    BrowserTabController.*       # tab identity, active view, navigation events
    ProfileManager.*             # normal/private profile lifecycle
    OriginKey.*                  # origin canonicalization / security boundaries
  controlbar/
    CapabilityTypes.h           # enums, state structs, evidence types
    CapabilityModel.*           # QAbstractListModel for QML
    PermissionController.*      # QWebEnginePermission orchestration
    SitePolicyStore.*           # scoped user preferences; private in-memory
    SitePolicyResolver.*        # effective per-origin/view settings
    RequestActivityCollector.*  # minimal bounded request interception
    CookieActivityCollector.*   # profile cookie observations and safe filter
    CapabilityEventStore.*      # bounded in-memory event ring
    ControlBarCoordinator.*     # correct tab/profile/origin to QML state
ui/qml/controlbar/
    ControlBar.qml               # one reusable overlay, 4 orientations
    ControlIcon.qml              # icon, semantic policy + observation layers
    CapabilityPopover.qml       # single instance, reanchored
    PositionPicker.qml          # dock selector (final icon)
    PermissionActions.qml       # explicit request choices
    ControlBarTokens.qml        # glass/palette/metrics/motion
    ControlBarPreview.qml       # dev-only fixture/demo harness
resources/icons/controlbar/     # consistent SVG icon set, no font distribution
spec/tests/controlbar/         # fixtures, snapshots, integration scenarios
```

**Authority boundary:** WebEngine and C++ controllers own security decisions and observations. QML reflects `CapabilityModel` and invokes explicit controller commands, never invents security events. QML-generated demo events must be compiled out/behind dev-only flag in release.

**Event flow:**

```text
Qt WebEngine permission / setting / request event
                 ↓
  C++ PermissionController / collectors
                 ↓
      Origin + profile + tab attribution
                 ↓
    SitePolicyResolver / CapabilityEventStore
                 ↓
     CapabilityModel (read-only to QML)
                 ↓
        ControlBar + popover visuals
                 ↑
       explicit UI action command
                 ↓
 controller validates origin/request lifetime and applies policy
                 ↓
   engine verifies effective result → then model updates
```

**Qt integration facts / version gates:**

- QML `WebEngineView.permissionRequested` since Qt WebEngine 6.8; permission object grant/deny/reset. Reuse existing browser view/page event plumbing, avoid duplicate request handlers.
- `QWebEngineProfile` stores persistent permissions and shares a cookie store with all pages in profile. `queryPermission` availability/behavior varies by QML vs C++ and permission persistence; test pinned version explicitly.
- `QWebEngineUrlRequestInterceptor::interceptRequest()` stalls Chromium request pipeline; must be O(1) or bounded, no blocking cross-thread UI calls / database access / JSON construction. Keep lightweight snapshot and enqueue copied *sanitized* event to dedicated consumer.
- `QWebEngineCookieStore::setCookieFilter()` likewise runs on IO path, with global profile scope and affects more than HTTP cookies; do not blindly use for a fake site-only toggle.
- `WebEngineSettings.javascriptEnabled`, `javascriptCanOpenWindows`, `localStorageEnabled`, clipboard settings vary in effect; maintain site override policy without breaking other tabs.
- Use Qt-provided callback/signal/permission lifecycles, not promises that each API call is individually observable.
- Audit **exact pinned Qt 6 version and Chromium build** before implementing any permission type, enum, or persistent permission behavior. Do not copy sample code without compiling against repository dependencies.

---

## 9. Security and privacy rules

- Browser shell/QML bar lives outside untrusted WebEngine content. Website JavaScript cannot read, toggle, imitate or remotely control UI state or privileged C++ objects.
- Never expose unrestricted local JSON-RPC/HTTP bridge or C++ Qt objects to arbitrary pages to implement this feature.
- No disabling Chromium sandbox, TLS checks, origin isolation, secure context requirements, permission prompts, automatic security updates, or Safe Browsing/phishing strategy to make this panel work.
- No silent auto-grant for camera, mic, screen, geolocation, notification, or clipboard access. `allow once` is tied to one validated live engine request if supported.
- Do not treat two origins as identical based on similar-looking hostname, public suffix, normalized path, or raw domain alone.
- Confirm whether permission changes can invalidate an already active media stream; if not guaranteed, show `Bloqué pour les prochaines demandes` and/or require reload, and do not falsely report stopped usage.
- Async cookie removal and profile-cache operations only show success after known confirmation; avoid "delete everything" when engine cannot isolate site data.
- Avoid storing request bodies, page form inputs, query secrets, auth headers, clipboard contents or location coordinates as debug telemetry. Sanitized domain and type only; no analytics without user opt-in.
- Private mode must use off-the-record profiles and zero durable permission/event storage from private pages.
- No network calls or cloud accounts required for Control Bar baseline.
- If data source is not trustworthy/available, choose `Unknown/Unsupported` instead of success-colored fiction.

---

## 10. Layout, compositing and performance constraints

- Must remain responsive during busy page loading; no UI jank from thousands of requests.
- Avoid heavyweight full-window shader blur applied every frame. Limit blur to bar/card rectangle; cache icon assets; fade glow only when state changes. Provide low-power fallback.
- For page scrolling: rail floats over content and does not resize Chromium viewport. Scrollbar must remain reachable. Pointer events are accepted only inside actual bar/popover hit regions; do not use an invisible viewport-wide MouseArea that blocks site clicks.
- On native window resize/DPI change: recompute anchor and clamp; bar must not fall outside viewport or cover essential browser chrome.
- On page context menus/native file pickers/permission dialogs: define stacking to prevent collision, and freeze/close popover safely without losing pending permission state.
- On fullscreen video or immersive web fullscreen: default hide or reduce UI according to explicit browser fullscreen UX; never overlay video controls unexpectedly. Permission events still need accessible safe prompt.
- For narrow windows: do not overflow outside browser; scroll icons within continuous capsule, keep position icon always accessible, preserve minimum popup reachability.
- Desktop first: mouse, keyboard, wheel. Touch secondary but functional.

---

## 11. Delivery plan for coding agent (incremental and test-driven)

### Phase A — repository audit / engine spike (must happen first)

1. Inspect repository structure and Qt / Chromium exact versions and API availability; do not assume empty project.
2. List current tab, WebEngineView, profile, signal plumbing, CMake and QML styles. Determine whether there is already a PermissionController or CookieStore helper.
3. Build a small executable spike proving actual Qt permission request handling, policy query/reset and event-to-tab scope for this version. Document unsupported capabilities and conditions under which permissionRequested does NOT fire (cached decisions).
4. Build a second spike proving overlay works on top of QML WebEngineView in 4 positions without covering browser chrome/scrollbar, at common DPI factors. Output a short `API_CAPABILITY_MATRIX.md` with `verified / partial / unavailable` per icon.

**Gate A:** no release copy promises a type of control that has not been verified on the pinned build.

### Phase B — real Control Bar shell (QML)

5. Implement one `ControlBar.qml` with 4 placements, glass tokens, dense icon groups, stable last-icon position picker, overflow scrolling and responsiveness.
6. Implement `ControlIcon.qml` with orthogonal policy/observation representations, inactivity style, reduced-motion and colorblind accessibility.
7. Implement single shared `CapabilityPopover.qml`, pointer corridor, delayed show/hide, focus and Escape/keyboard semantics.
8. Integrate into browser **trusted QML content overlay** without changing page geometry or interfering with web content input/scroll; no logos or text on rail.
9. Use development-only static fixture data to review screenshots; remove fixture path from shipping code.

**Gate B:** visual matches approved concepts: slim, continuous, nearly transparent, nearly right-edge overlay, 4 selectable orientations, hover panel hides on mouse-out without flicker.

### Phase C — permission core

10. Implement origin-aware `PermissionController` and `SitePolicyStore` tied to profile/tab and requesting origin; handle pending lifetimes, request cancel on navigation and explicit user selection.
11. Wire camera, mic, screen, location, notifications, clipboard through actual supported signals; no fabricated "in use" detection.
12. Implement persistent / temporary grant behaviors where tested; no high-risk one-click grant; background/iframe request correctness.
13. Build tab-specific `CapabilityModel`, observe tab switch, page navigation, profile changes and browser restart.

**Gate C:** actual engine permission changes are effective, isolated and faithfully displayed.

### Phase D — browser-behavior icons

14. JavaScript per-site/per-view policy resolver with safe reload UX, regression testing across tabs sharing profile.
15. Popups support with verified event source and action semantics.
16. Cookies: confirmed profile-wide observation and effective policy, scoped deletion only if precise filtering is verified. Build a correct async success/error flow.
17. Storage: honest `localStorageEnabled` indicator/setting with explicitly stated limits. Avoid full site data enumeration in v1.
18. Network: bounded sanitizer and scope-classified interceptor; no false tab attribution, no network stalls.

**Gate D:** no icon claims unsupported site-scoped behavior, no runaway performance/memory.

### Phase E — stabilization / test and documentation

19. Accessibility, FR/RU translation and tooltips, visual QA across four sides, normal/private browsing, cross-origin iframe behavior.
20. Write user-facing "What indicators mean" one-pager, full API matrix, evidence limitations and security decision log.
21. Run regression, performance and security tests below. If blocker exists, keep internal preview; do not label as production-safe.

**Workflow:** small coherent commits into `main` (project preference), feature flags for incomplete items, local tests before every commit, no mandatory long-lived branches. Do not touch unrelated product features or reinvent UI shell without the repo audit.

---

## 12. Test matrix / definition of done

### UI / positions

- [ ] `top`, `right`, `bottom`, `left` all render the exact same icons/states and place popover on the inward side.
- [ ] Right rail is almost touching right content edge but **is not attached to outside browser window** or covering scrollbar; top/bottom are above webpage, not in tab/URL rows.
- [ ] Last icon is four-position control; no ellipsis in rail; new position persists after restart.
- [ ] Active page can scroll and receive clicks outside rail/popover; rail intercepts events only inside itself.
- [ ] Popup opens from hover, survives mouse crossing from icon into card, closes on mouse-out grace, and does not disappear mid-click.
- [ ] Single popup only; stale popup cannot affect another tab/origin.
- [ ] Windows at 100/125/150/200% DPI, ultra-wide and narrow widths, system dark/light, high contrast, reduce motion.
- [ ] Tab switch, browser window resize/move, fullscreen, drag/download/context menus, mouse+keyboard+touch.

### Security semantics

- [ ] On a first genuine geolocation request: amber icon + correct requesting origin + explicit action; no auto-grant.
- [ ] On deny and reattempt: real deny + red policy + verified blocked request event when signal exists; no fabricated request if cached deny suppresses callback.
- [ ] On allow: engine confirms permission, correct scope; other origin remains unchanged. Private profile separated.
- [ ] Camera/mic/screen **not marked currently active** merely because permission was granted.
- [ ] Iframe request from origin B while top-level A is correctly attributed; no grant to A by accident.
- [ ] Change JavaScript off on A → A reload required/handled; unrelated open tab B stays functional; changing back restores behavior.
- [ ] Cookie pane does not delete unrelated profile cookies when "current site" removal is selected; completion is async confirmed, not optimistic.
- [ ] `network` shows exact origin attribution only when supported; otherwise profile-level scope; no raw payloads or query tokens.
- [ ] No untrusted website can invoke privileged QML/C++ policy methods.

### Performance / browser integrity

- [ ] Heavy 1000+ request page does not stall network or UI; interceptor bounded and tested.
- [ ] 25 popular-site smoke matrix: browsing, logins, video, shopping/cart, downloads, history, bookmarks, restore, forms, PDF; compare baseline browser functionality.
- [ ] Browser engine sandbox remains enabled; TLS errors not bypassed; no critical known unpatched engine vulnerability accepted for public release.
- [ ] App has safe update strategy and signed packaging before public beta (Control Bar is not a substitute for browser security maintenance).
- [ ] Idle memory/CPU for rail low; reduced-motion setting respected.

**Definition of done:** a user can open five different ordinary websites, see consistent correct permission indicators, change an actual policy from popover, locate the last request source, switch strip to four positions, and continue browsing without breakage. All release icon semantics are backed by tested engine capabilities. No fake states ship.

---

## 13. Risks / explicit out-of-scope items

| Risk | Mitigation |
|---|---|
| QT permission granted and cached without new request signal | Query effective permission store where supported, and don't claim "just requested" without event |
| Tab-specific vs shared-profile network attribution | Show profile-level label or unknown instead of invented per-tab stats |
| Site-specific cookie rules impact entire shared profile | Implement efficient origin-aware filter after proof; otherwise disclose global scope/defer |
| JS turned off too late | Treat as pending + reload and verify on newly loaded page; do not report instantaneous effect |
| Revoking permission doesn't stop active stream | Explicitly state future-request effect, reload/stop where supported |
| Popup auto-closes mid interaction | Combined hit region + grace interval + pinned mode/focus handling |
| Flickering/rainbow noise | One brief event pulse, static dot/TTL, coalesced telemetry, semantic colors |
| Right rail blocks scrollbar | Position inside usable content edge with scrollbar gutter, pointer hit test |
| CPU spikes from blur or telemetry | Bounded compositor regions, low-power fallback, cheap network callback |

Out of scope for Control Bar v1: anti-fraud AI, magic page trust scores, malware scanning, complete packet inspection, automatic personal-data leak detection, 100% effective fingerprint defense, full per-origin IndexedDB/CacheStorage/ServiceWorker clearing without verified API, browser-wide site dashboard replacing the strip, mandatory CYBOU network/cloud/token.

---

## 14. Deliverables required from coding agent

**Before code:**
- `API_CAPABILITY_MATRIX.md` (exact pinned Qt build, events, scopes, supported controls, hard limitations).
- Architecture map of existing repo integration points; list of file changes and phased implementation plan.

**During implementation:**
- Functional C++ model/controllers with tests; QML glass strip and popover; all four positions; persisted preference; real WebEngine integrations; FR/RU texts; accessibility.
- Reproducible local fixtures for permission, iframe, cookie, JS, popup and network scenarios.
- Consistent icons/assets; developer-only mock preview cannot appear in production build.

**After code:**
- Brief honest feature/known limitations release notes.
- Screenshot set showing top/right/bottom/left at identical site/scope and at least one request event.
- Manual test protocol and local test results. Critical unsolved API gaps must be recorded, not disguised.

Do not respond to this task with only a prettier static mockup. The deliverable is **real browser functionality** with observable facts, safe controls and a user interface that matches the approved form.

---

## 15. Canonical Qt references (verify against pinned version)

- [Qt WebEngine `WebEngineView` (QML) — permissionRequested](https://doc.qt.io/qt-6/qml-qtwebengine-webengineview.html)
- [Qt `QWebEnginePermission` — origins, persistence, grant/deny/reset](https://doc.qt.io/qt-6/qwebenginepermission.html)
- [Qt `QWebEngineProfile` — permissions and cookies](https://doc.qt.io/qt-6/qwebengineprofile.html)
- [Qt `QQuickWebEngineProfile`](https://doc.qt.io/qt-6/qquickwebengineprofile.html)
- [Qt `QWebEngineUrlRequestInterceptor`](https://doc.qt.io/qt-6/qwebengineurlrequestinterceptor.html)
- [Qt `QWebEngineUrlRequestInfo` — firstPartyUrl/initiator/resourceType](https://doc.qt.io/qt-6/qwebengineurlrequestinfo.html)
- [Qt `QWebEngineCookieStore` — async operations and IO-thread filter](https://doc.qt.io/qt-6/qwebenginecookiestore.html)
- [Qt `WebEngineSettings` (QML) — JS, storage, popups, clipboard](https://doc.qt.io/qt-6/qml-qtwebengine-webenginesettings.html)

**Existing CYBOU planning inputs:** `CYBOU_Browser_Overview.md` and `CYBOU_Browser_Roadmap.md` (2026-10-16). This document supersedes their former launch-killer-feature choice but retains everyday browser completeness, local-first privacy, accurate claims, normal/private profile handling, release security gates, and Qt/QML/C++ stack.
