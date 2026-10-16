# CYBOU Browser 0.8.0

CYBOU is a browser for Observable Browsing: a visible capability bar provides direct control over page permissions and behavior. Built with C++17 / Qt Quick / Qt WebEngine.

Current implementation and limitations: [product vision](docs/PRODUCT_VISION.md), [security model](docs/SECURITY_MODEL.md), [capability contracts](docs/CAPABILITY_CONTRACT.md), [roadmap](docs/ROADMAP.md). The static tracker suffix list is a limited network filter, not a complete ad-blocking system. Risk-adaptive restrictions and verified proxy isolation remain planned. Temporary HTTP access uses a separate in-memory profile and expires after 60 seconds.

## Windows build

Requires Qt 6.9+ Quick, Quick Controls, WebEngine/Core, Network and LinguistTools; MSVC 2022 x64; CMake. Tested with Qt 6.12.0 and MSVC 19.44.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/msvc2022_64
cmake --build build --config Release --parallel 4
C:/Qt/6.12.0/msvc2022_64/bin/windeployqt.exe --release --qmldir qml --no-translations build/Release/cybou.exe
./build/Release/cybou.exe
```

Linux/macOS: configure with the matching Qt kit and compiler. Source structure is portable; builds, window integration and packaging on these platforms remain unverified. macOS bundle icon packaging is outstanding.

## Implemented today

The following older feature inventory is being reconciled with the normative contracts linked above.

- **Dark Glass UI & Alpine Dawn**: In 0.8.0, all browser-owned chrome (tab strip, navigation row, omnibox, control bar icons, overflow menu, capability popover, library drawer, settings panel, search suggestions, and modal dialogs) is unified under the CYBOU Dark Glass visual language (`CybouTheme`). The `about:blank` new tab page features the high-resolution Alpine Dawn landscape (`assets/cybou-alpine-dawn-1920x1080.jpg`), original CYBOU branding, centered floating glass card, and responsive layout. Untrusted third-party web content is left unmodified.
- **Capability-first chrome**: eleven compact controls remain visible in the navigation row while the full URL is edited. Spaced buttons open glass panels on hover, containing explanations and controls; supported policies and live requests also use direct icon clicks.
- **Temporary permissions**: AskEveryTime engine policy, no arbitrary native pregrants, no modal ASK, 15-second pending timeout and independent 60-second grant deadlines; the earliest deadline destroys and replaces the WebEngineView. No claim of active-device monitoring.
- **Network observation**: bounded, sanitized profile-scoped events include blocked outcomes and reasons. A small built-in tracker suffix list blocks selected third-party requests.



- Combined draggable tab/title bar; minimize, maximize/restore, close, edge resizing and F11 fullscreen.
- Scrollable tabs, mouse drag reordering with edge autoscroll, favicons, middle-click close, duplicate and move-left/right menu, reopen closed tabs and keyboard switching. Both browser bars remain 36 logical px high, with 28 px tab/address controls.
- URL/search input, back/forward, stop/reload, home and zoom.
- Address suggestions from local bookmarks and history, deduplicated and limited to 8 results. Up/Down select, Enter opens, Escape dismisses. No search query is sent to a suggestion service; private windows suggest bookmarks only.
- Pinned tabs stay compact on the left; drag reordering stays within pinned/unpinned groups. Pin and mute controls are in the tab menu. Sound indicators appear on audible/muted tabs; Ctrl+M toggles the current tab. Pinned/muted states survive session restoration and reopening a closed tab. Existing URL-only sessions remain compatible.
- Persistent bookmarks, latest 1000 history entries, filterable library and confirmed history deletion.
- Configurable ordinary session restoration. Private windows are independent, each with an isolated in-memory profile destroyed when its window closes. Ordinary tabs stay open; private visits never enter history/session storage.
- Downloads to a configurable directory, optional save-location dialog, progress, pause/resume, cancellation, explicit file opening and folder access. Files never open automatically. Download list and resumability last for the window lifetime; interrupted-download recovery after a restart is not implemented. Closing an active transfer or PDF export asks for confirmation.
- Native local-file picker for HTML, text, PDF and images (Ctrl+O). Local page access to other local files and remote resources stays blocked.
- Save the current loaded page as a PDF via a native save dialog (Ctrl+P), with asynchronous completion/error feedback. Ctrl+P currently exports PDF rather than presenting a physical-printer dialog.
- Find in page with match counts and previous/next.
- Settings: DuckDuckGo/Google/Bing/Brave, startup restoration, third-party cookie blocking, per-profile cleanup and permission reset.
- Icon in Windows executable, application window, tab fallback and start page.

Shortcuts: Ctrl+L address, Ctrl+T new tab, Ctrl+W close, Ctrl+Shift+T reopen, Ctrl+Tab / Ctrl+Shift+Tab switch, Ctrl+D bookmark, Ctrl+H history, Ctrl+J downloads, Ctrl+F find, Ctrl+Shift+N private window, Ctrl+, settings, Ctrl+O local file, Ctrl+P PDF export, Ctrl+M mute tab, Ctrl+R reload, Alt+Left/Right navigation, F11 fullscreen. Start directly in private mode with --private.

## Privacy and development limits

Certificate errors are rejected; insecure mixed content and local file access from web content are disabled. Only user-initiated HTTP(S) new-window requests become tabs. Chromium sandboxing remains enabled.

Foreground supported requests appear on their capability icon. One click grants the current request; unattended requests are denied after 15 seconds. Both profiles use AskEveryTime. C++ PermissionCoordinator checks the exact profile/tab/origin/navigation epoch and maintains independent 60-second deadlines. The earliest deadline destroys and replaces the WebEngineView. Actual camera/microphone activity is not observable. Native notification presentation remains outstanding.

Control Bar never equates a permission grant with active device use. Permission events and the profile-wide HTTP(S) metadata sample are memory-only and bounded. No bodies, headers, query strings or URL paths are retained by this observer. Cookie filtering remains a global preference affecting all open profiles, rather than a fictional site-only control. New panel strings can be switched FR/RU in Settings; unrelated legacy UI remains French.

Each private window owns a separate in-memory profile, destroyed on close. Private browsing does not anonymize traffic. Explicit bookmarks, preferences and downloaded files remain on disk. History and session URLs are stored locally in QSettings without encryption.

Cleanup supports cookies, HTTP cache, history, permission decisions and saved tabs. Cookie/saved-tab cleanup closes tabs in the current window. Cookie deletion is asynchronous; cache completion is tracked. LocalStorage, IndexedDB and service workers are not removed by this command. Third-party cookie filtering applies to open profiles; it is not a full tracker blocker.

Development build, not security audited. Passwords, extensions UI, physical-printer UI, automatic updates, installer, crash recovery, platform polish and comprehensive security/accessibility testing remain outstanding. Keep Qt/Chromium patched.

### HTTPS-First and transport security

HTTPS strict is always enabled in production; old global opt-outs are removed. HTTPS-First attempts TLS for public HTTP addresses. After a blocked HTTP attempt or a failed known upgrade, the inline Network glass panel offers explicit access to that exact address.

Consent creates a separate off-the-record profile without normal cookies/cache, valid for at most 60 seconds. One exact main-frame URL and subsequent same-origin HTTP resources with matching firstParty are allowed. Other HTTP origins and unencrypted WebSockets stay blocked. Reload or later navigation destroys the context and requires fresh HTTP consent. Revoke and expiry close the document in its existing tab slot. Page-controlled navigation is never automatically replayed with normal-profile cookies. This separates profile data; it does not provide process or proxy isolation.

HTTP(S) URLs containing credentials are always rejected by the interceptor, independently of strict mode. Page-triggered external protocol launches are disabled through Qt's unknown-scheme policy. WebRTC is restricted to public interfaces; this is not an anonymity guarantee. TLS errors and mixed active content remain rejected. Control Bar stays inline with navigation and never moves into a separate row.

### Architecture and modularity

Run `./build/Release/cybou.exe --transport-test` to exercise HTTP panel consent/revoke, profile separation, exact navigation, reload, stale consent and 60-second expiry against a loopback fixture. See [test plan](docs/TEST_PLAN.md) and [validation](docs/VALIDATION.md).

The UI layer is modularized into discrete, reusable QML components:
- `AdaptiveAddress.qml`: Context-aware omnibox supporting BROWSE and EDIT modes.
- `ControlBarInline.qml` & `ControlBarOverflow.qml`: Responsive permission and privacy indicators.
- `CapabilityPopover.qml`: capability details; global tracker disabling is unavailable from site details.
- `PolicyBanner.qml`: Slide-down notification banner for 1-click policy application with instant undo and reload actions.
- `FindBar.qml`: Dedicated in-page search bar with match count and cycling.
- `GlassButton.qml` & `GlassDialogBackground.qml`: Shared frosted glass design tokens and interactive components.

Run `./build/Release/cybou.exe --security-test` for URL-policy cases and actual WebEngine network checks: strict HTTP denial, successful load after a test-only opt-out, denial of a new same-origin fetch from an already loaded HTTP page after enabling strict mode, credential-URL denial, exact host formatting, IPv6, ACE/IDN punycode and safe display tests. The fixture listens only on loopback and no security preferences are changed by this test.

### Reviewing downloaded files

The download library's Open action now shows the filename and download origin before handing a file to an external application. The default focused action is Cancel; no trust decision is remembered. External opening is restricted to an explicit list of document, image and media extensions. Programs, scripts, shortcuts, archives, HTML/SVG, macro document formats, extensionless and unknown files require examining the download folder. Saving these files is still supported. Missing files and symbolic links are rejected; the native launch function rechecks eligibility immediately before opening. The extension list does not verify file contents, scan for malware or guarantee the safety of an external application. Local HTML/SVG can still be opened explicitly through the browser's existing local-file picker with local content access restrictions.

`--security-test` covers executable double extensions, scripts, shortcuts, alternate-stream syntax, ambiguous trailing characters, document eligibility, and the native launch guard using temporary files. The integrated smoke suite checks the review dialog against real completed `.txt` and `.bin` downloads, origin display, cancellation and cleanup. It captures `build/download-review.png` and `build/download-blocked.png` without launching an external application.

## Integrated smoke test

```powershell
$env:QT_FORCE_STDERR_LOGGING = '1'
./build/Release/cybou.exe --smoke-test
```

Uses a separate QSettings organization and WebEngine storage name. Checks tab dragging with mouse events, close/reopen, bookmarks, history, preference updates, independent private windows and destruction of their profiles. A loopback-only HTTP fixture checks page loading, find matches, a real permission request and grant, cookie removal, permission reset, cache completion, PDF output signature, local-file loading, and a 4 MiB download with pause/resume and cancelled window closure. Artifacts go to build/test-downloads. Produces build/smoke-preview.png; exits 0 on success and 2 on test failure. No external sites required. Native OS file/folder dialog interaction is not automated by this test.

Also checks address suggestions via keyboard events, deduplication/private history exclusion, pinned group boundaries, mute binding, tab-state restoration and compatibility with URL-only sessions. The final preview shows pinned/muted tabs and suggestions using temporary fixture data, removed after capture.

Run `--controlbar-test` to execute the full Adaptive Omnibox suite: BROWSE vs EDIT mode transitions, budget-based responsive icon sizing, overflow dropdown behavior, hover and pinned details card, keyboard roving navigation, Russian/French catalog, about:blank / local file display, network telemetry stability, and snapshot generation (`build/omnibox-*.png`). Actual test runs and verification results are documented in the integration report.
