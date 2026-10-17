<p align="center">
  <img src="assets/cybou.png" alt="CYBOU Browser — compass/shield icon" width="112" />
</p>

<h1 align="center">CYBOU Browser</h1>

<p align="center">
  <strong>See what a page requests. See what gets blocked. Control what it can do.</strong>
</p>

<p align="center">
  <em>Observable Browsing · Observe → Restrict → Control → Revoke</em>
</p>

<p align="center">
  <a href="docs/PRODUCT_VISION.md">Product vision</a> ·
  <a href="docs/CONTROL_BAR_UX.md">Control Bar UX</a> ·
  <a href="docs/SECURITY_MODEL.md">Security model</a> ·
  <a href="docs/ROADMAP.md">Roadmap</a>
</p>

<p align="center"><sub>Engineering alpha · v0.3.0-alpha.1 · NOT RELEASE QUALIFIED</sub></p>

---

![CYBOU Browser running its Alpine Dawn default new-tab page](docs/design/cybou-08-alpine-dawn.png)

<p align="center"><sub>Historical Alpine Dawn UI capture, previously labelled 0.8 — a visual milestone, not evidence of a production-qualified v1.0 release.</sub></p>

## Current release maturity and security evidence

**Declared source: `0.3.0-alpha.1` (engineering alpha).** Earlier 0.7/0.8 labels were design/prototype milestones, not security-qualified product releases. CYBOU has implemented capabilities and historical local PASS records, but its **complete OWASP verification, browser sandbox evidence, device tests, updater/signing and independent audit are unfinished**. Do not describe the browser as externally audited or production-secure.

The first supported version will be **1.0.0 on Windows x64**, only after all evidence-driven gates pass. [Version policy](docs/VERSIONING.md) · [Gated roadmap](docs/ROADMAP.md) · [Security verification framework](docs/SECURITY_VERIFICATION_FRAMEWORK.md) · [Release criteria](docs/RELEASE_CRITERIA.md) · [Documentation index](docs/DOCUMENTATION_INDEX.md).

## A browser built around what pages **do**

Conventional browser chrome gives the address bar most of the attention, while the permissions that matter are often hidden behind menus and prompts. **CYBOU reverses that priority.**

The main interface is a **permanently visible, single-row Control Bar**. Its individual icons show which capabilities a page can use, which requests the browser actually observed, and what was blocked. Supported controls act directly on a click. Hover opens a glass panel containing contextual explanations and supported controls, without requiring a click.

The full URL is always available for inspection and editing, but **the address bar never displaces the capability controls**. Page identity remains important for origin isolation and anti-spoofing; an HTTPS address alone is not proof that a site is trustworthy.

### Non-negotiable principles

- **Visible controls first.** Keep all eleven main capability icons in the **same navigation row** as the address, which fills remaining width. Do not replace them with three group buttons or move the Control Bar to another row.
- **One-click interaction.** A supported policy changes immediately, or a click acts on an actual pending permission request. Capabilities without a safe direct action open explanatory details instead.
- **No permanent positive trust.** A previous visit does not earn future camera, microphone, geolocation, or other sensitive permissions. There is no “Always allow this site” privilege.
- **No user-facing ASK workflow.** A page request highlights the relevant control. The browser does not interrupt ordinary browsing with a routine permission dialog.
- **Conservative by default.** Sensitive capabilities are unavailable until explicitly granted; defensive network rules are enabled by default. Any relaxation must be deliberate and limited.
- **Truthful observability.** A policy, a request, a grant, a blocked attempt, and actual device activity are distinct facts. A grant is **not** evidence that a camera or microphone is recording.
- **No third-party browser extensions — ever by default or by partner exception.** No Chrome/WebExtensions, extension store, installation, side-loading, privileged userscripts or extension runtime. Review useful ideas from other browsers and implement them in CYBOU-owned C++/Qt/QML; minimize nonessential external executables. See [ADR 0006](docs/ADR/0006-no-third-party-extensions.md).
- **Cloud services are user-chosen, not privileged partners.** Proton is the default cloud suite; Google Workspace, Microsoft 365 (Office 365), Infomaniak kSuite, Zoho Workplace and self-hosted Nextcloud are equal selectable options. The cloud selection is separate from the search engine, does not require an account and cannot override site permissions. This is an accepted **future feature**, not shipped code. [ADR 0007](docs/ADR/0007-selectable-cloud-services.md).

The normative definitions are in [Product Vision](docs/PRODUCT_VISION.md), [Security Model](docs/SECURITY_MODEL.md), and [Capability Contract](docs/CAPABILITY_CONTRACT.md).


## Deliberately no extension ecosystem

CYBOU **does not support third-party browser extensions**, including signed, partner-branded or Proton extensions. This is an explicit product and security ban, not an unimplemented convenience feature. There will be no Web Store, sideloading, privileged extension runtime or extension auto-updater. Successful extension *ideas* may inform first-party features that CYBOU develops and audits itself.

Qt/Chromium and necessary OS libraries remain external dependencies and require security updates; websites still execute untrusted JavaScript within WebEngine's web-content boundary. Nonessential third-party executable helpers or companion CLIs are not bundled by default. Partnerships cannot waive this rule. [Binding architecture decision](docs/ADR/0006-no-third-party-extensions.md).

## Cloud services: current prototype, compatibility under verification

CYBOU now has the **first C++/QML implementation of a Cloud Services** selector similar to the existing search-provider selector, with **Proton selected by default** for new preferences, and **Google Workspace, Microsoft 365 / Office 365, Infomaniak kSuite, Zoho Workplace and Nextcloud Hub** available on equal terms. Apple iCloud and specialist file-storage services are candidates for a later catalogue release. The user can also disable cloud shortcuts.

**Every Alpine Dawn new tab will have a Mail text link and a nine-dot (3×3) app launcher at its top right.** Clicking the launcher opens a downward Dark Glass popup with the user's **effective mixed set of cloud apps**. The user chooses both a default cloud suite and independently the provider for Mail, Files, Calendar, Docs, Sheets, Presentations and Meetings. Overrides persist when changing the default and can be reset explicitly. The central Alpine Dawn card and the eleven-icon navigation Control Bar remain unchanged. These are **ordinary HTTPS sites**, not browser extensions or native companion executables. Changing the cloud provider never changes the search engine, browser security policy, existing tabs, cookies or session. No site obtains extra privileges from a commercial partnership. **The selector and launcher exist in code, but complete provider compatibility and release-level security validation are still pending.** See [ADR 0007](docs/ADR/0007-selectable-cloud-services.md).

## Your page, your controls

**Eleven individual capabilities are visible in the existing navigation row:**

| Capability | Current implementation |
| --- | --- |
| **Camera / Microphone** | Highlight actual WebEngine permission requests; grant a still-valid request by clicking its icon. A grant is not proof of active capture. |
| **Screen sharing** | Marked unavailable while the capture chooser is not integrated. Unsupported requests are denied. |
| **Geolocation** | Observe and handle supported permission requests; a functional physical-location backend has not been verified. |
| **Notifications** | Observe and handle permission requests; native desktop notification delivery is not integrated. |
| **Clipboard** | Handle supported WebEngine permission requests; not all clipboard activity is observable. |
| **JavaScript** | Toggle the page's behavior policy; offer Reload / Undo when needed. |
| **Pop-ups** | Control future window-opening behavior; user-initiated actions are allowed by default. |
| **Cookies** | Display the current third-party-cookie policy. Filtering is **profile-wide**, not a per-site switch. |
| **Site storage** | Control the WebEngine localStorage setting; not comprehensive IndexedDB, Cache Storage, or service-worker management. |
| **Network** | Human-readable resource types and filter decisions, HTTP/tracker/other categories, bounded profile-scoped samples. A temporary tracker exception recreates one HTTPS document in a separate off-the-record profile, with explicit revocation. |

**An icon has two independent jobs:** indicate the effective policy and signal a confirmed event. A pending request is visually distinct from an allowed capability, a blocked attempt, and recent activity. Glass hover panels identify the relevant scope and requesting origin when available.

### Permission lifecycle in the current build

1. A foreground page requests a supported sensitive capability; the corresponding icon highlights **without a routine ASK dialog**.
2. Clicking a highlighted icon may grant **the current, valid request**. Unanswered requests are denied after **15 seconds**.
3. Regular and private WebEngine profiles use **AskEveryTime** internally; they do not persist positive grants to disk.
4. Each grant belongs to the current document. Reload, navigation, tab closure or explicit reset end that access; revocation may replace the WebEngineView. A grant is not evidence of active hardware use.
5. Another visit requires a new permission decision; no permanent positive site trust is retained.

A request's top-level site and actual requesting origin may differ (for example, in an iframe). This needs further real-world verification with combined camera/microphone requests, multiple tabs, and physical devices.

## Security and network protection

| Area | Shipped in the code | Planned / unverified |
| --- | --- | --- |
| **Encrypted transport** | Production HTTPS strict; exact HTTP consent in a separate off-the-record profile. Legacy global opt-outs removed; certificate errors rejected. | Real-world transport compatibility and OS suspend/resume checks. |
| **Tracker filtering** | A native versioned RuleEngine and ad/tracker controls exist in code; past fixture tests are recorded. | Broader real-world rule coverage, regression against false positives, updated rules and verified release security. |
| **Observable traffic** | Bounded, sanitized metadata with origin, resource type, timestamp, blocked outcome, and reason. | Guaranteed per-tab attribution and visibility into all page/JavaScript activity. |
| **Permission restrictions** | Native C++ coordinator, request identity checks, fail-closed pending timeout and document-scoped grants. | Proven hardware capture-stop behavior on real devices. |
| **Risk-adaptive mode** | Conservative WebEngine content policies and the transport/request interceptor. | Evidence-based risk engine and JS-off applied before suspicious content loads. |
| **Proxy isolation** | **Not implemented.** | Dedicated, tested network/process isolation including DNS and WebRTC leak checks. |

An unfamiliar website is not automatically malicious. CYBOU must base stronger restrictions on specific evidence and must never pretend that simply using a proxy makes a malicious page safe.

**Security disclosure:** CYBOU is an experimental browser, not independently security-audited. The current suffix-list filter is **not** a complete ad blocker. Network observation cannot reveal every action executed by a page. Private mode does not anonymize the connection. Keep the underlying Qt/Chromium engine patched.


## Temporary HTTP and tracker exceptions

### HTTP: exact-address consent

Production HTTPS strict is always enabled; legacy global opt-outs and host-wide fallback are removed. After a blocked HTTP attempt or failed known HTTPS upgrade, the Network panel offers explicit access to that exact address. Consent creates a separate off-the-record profile without normal cookies/cache for this document. One exact main-frame URL and same-origin HTTP resources with matching firstParty are allowed. Other HTTP origins and ws remain blocked. Reload, later navigation, tab closure and manual revoke end the temporary view/profile; HTTP requires fresh consent. Page-controlled navigation is never automatically replayed with normal cookies. This separates profile data, not processes or proxy routes.

A typed local RuleEngine exists, but real-world coverage and fail-open/fail-closed edge cases still require comprehensive validation. Restricted Browsing exists in code; verified proxy/process isolation remains unproven. See [validation](docs/VALIDATION.md) for native HTTP, permission lifecycle and hover-panel evidence.

The connection indicator becomes an HTTP consent/revoke button when applicable. A vertical separator divides the capability controls from downloads. Drag the unused tab-strip area to move the window; double-click it to maximize or restore the previous size.

### Trackers: origin-scoped temporary exception

The Network glass panel groups HTTP, tracker and other events, using resource names and clear explanations. Passing a filter is not proof of a successful load. Counts are scoped to the current profile's recent sample.

If filtering breaks a loaded HTTPS page, explicit consent grants a temporary tracker exception for that origin in the profile without losing login state, cookies, or session storage. The exception belongs to the tab that requested it and stays active while that tab remains on the origin. Tabs showing the same origin share and truthfully reflect this active exception in the Network panel. Navigating the owner tab away, closing it, or manually revoking from the panel immediately clears the exception. This mode does not combine with HTTP exceptions. See [ADR 0005](docs/ADR/0005-origin-scoped-tracker-exceptions.md).

## Security and UX: what we are building next

The **single-row eleven-icon Control Bar and interactive glass hover panels remain unchanged**. The next priorities are iframe deny inheritance, a reliable media-permission lifecycle without silent page loss, WebEngine patch/sandbox discipline, full ad/tracker rule coverage, independent site-scoped controls, truthful activity and navigation risk checks.

The RuleEngine and independent ad/tracker categories are implemented in code, with historical fixture testing, but comprehensive filter and release validation remain pending. Permission grants do not prove active hardware use; profile network events are not per-tab statistics. See the [Security & UX plan](docs/SECURITY_UX_EVOLUTION.md) and [Roadmap](docs/ROADMAP.md).

## Everyday browser features

The current application also includes draggable/reorderable tabs; pinned and muted tabs; bookmarks and history; local address suggestions; private windows with independent off-the-record profiles; a download library with an explicit review before external opening; PDF export; find in page; local file opening; and configurable search providers.

The browser-owned UI uses the CYBOU navy/red/white brand and the Dark Glass / Alpine Dawn design language. The application icon is [assets/cybou.png](assets/cybou.png); the Windows executable icon is [assets/cybou.ico](assets/cybou.ico).

**No permanent site permission does not mean no local data is saved.** Bookmarks, history, session URLs, cookies, downloaded files, and ordinary first-party storage can persist. History and session URLs are saved in QSettings without encryption. The current cleanup command does not comprehensively erase localStorage, IndexedDB, or service workers.

## Build on Windows

**Build requirements:** a C++20-capable compiler, CMake 3.24+, and Qt 6.9+ with Quick, Quick Controls 2, WebEngine Quick/Core, Network, LinguistTools and Test. The recorded Windows validation includes a successful C++20 Release build and security suite with Qt 6.12.0 and MSVC 2022 x64 (19.44); this is not an independently audited release.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/msvc2022_64
cmake --build build --config Release --parallel 4
C:/Qt/6.12.0/msvc2022_64/bin/windeployqt.exe --release --qmldir qml --no-translations build/Release/cybou.exe
./build/Release/cybou.exe
```

Linux and macOS may use their corresponding Qt kits, but builds, packaging, and integrations are **not yet verified**. Installer, automatic security updates, crash recovery, and release-grade platform validation remain outstanding.

### Keyboard shortcuts

| Shortcut | Action |
| --- | --- |
| Ctrl+L | Edit the full URL |
| Ctrl+T / Ctrl+W | New tab / close tab |
| Ctrl+Shift+T | Reopen closed tab |
| Ctrl+Tab / Ctrl+Shift+Tab | Switch tabs |
| Ctrl+R | Reload |
| Ctrl+F / Ctrl+J | Find on page / downloads |
| Ctrl+D / Ctrl+H | Bookmark / history |
| Ctrl+Shift+N | Private window |
| Ctrl+, | Settings |
| Ctrl+O / Ctrl+P | Open local file / export PDF |
| F11 | Fullscreen |

## Tests

Use the built-in local fixtures for deterministic regression checks:

```powershell
python tools/security/test_owasp_gate.py
python tools/security/owasp_gate.py --mode baseline --exe build/Release/cybou_tests.exe --out build/security-baseline.json
```

This local runner launches six `cybou_tests.exe` suites and records per-suite evidence. The separate OWASP release gate remains intentionally blocked until official inventories, triage, and per-case evidence are provided. These suites exercise policy rules, layout and interaction, and browsing workflows. The test harness also produces local screenshots. Passing a local suite is **not** equivalent to verifying permissions on real devices or auditing web-engine security. See the [Capability Audit](docs/API_CAPABILITY_MATRIX.md) and [Roadmap](docs/ROADMAP.md) for gaps.

## Documentation

| Read | Purpose |
| --- | --- |
| [Product Vision](docs/PRODUCT_VISION.md) | CYBOU's purpose and fixed principles |
| [Security Model](docs/SECURITY_MODEL.md) | Threat boundaries and permission lifecycle |
| [Capability Contract](docs/CAPABILITY_CONTRACT.md) | Observable events and truthful controls |
| [Control Bar UX](docs/CONTROL_BAR_UX.md) | Permanent, inline **single-row** interaction rules |
| [ADR 0007 — Selectable Cloud Services](docs/ADR/0007-selectable-cloud-services.md) | Accepted Proton-default, provider-neutral cloud ecosystem catalogue; planned, not shipped |
| [ADR 0006 — No third-party extensions](docs/ADR/0006-no-third-party-extensions.md) | Mandatory ban on external browser extensions and policy for minimizing executable dependencies |
| [Architecture](docs/ARCHITECTURE.md) | WebEngine, observation, models and QML |
| [Roadmap](docs/ROADMAP.md) | Implementation state and verification gates |
| [Security & UX Evolution (RU)](docs/SECURITY_UX_EVOLUTION.md) | Current security/UX direction, planned features and test gates |
| [Engineering proposal (RU)](docs/OBSERVABLE_BROWSING_PROPOSAL_RU.md) | Detailed plan — not a report of completed features |

Older design mockups and integration reports are retained in the repository for history. The documents above describe the current product direction.

---

<p align="center">
  <strong>CYBOU — Observe. Restrict. Control. Revoke.</strong><br />
  <sub>A capability-first browser designed to make page behavior visible — not to ask for blind trust.</sub>
</p>
