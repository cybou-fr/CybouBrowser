# CYBOU Browser 0.7.1

Native C++17 / Qt Quick browser using Qt WebEngine. Navy, white and red identity follows the supplied CYBOU sheet. The compass/shield icon was reconstructed from that reference with the built-in imagegen tool. assets/cybou.png is embedded in the app; assets/cybou.ico contains 16–256 px Windows icon sizes embedded through assets/cybou.rc.

## Windows build

Requires Qt 6.9+ Quick, Quick Controls, WebEngine/Core, Network and LinguistTools; MSVC 2022 x64; CMake. Tested with Qt 6.12.0 and MSVC 19.44.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/msvc2022_64
cmake --build build --config Release --parallel 4
C:/Qt/6.12.0/msvc2022_64/bin/windeployqt.exe --release --qmldir qml --no-translations build/Release/cybou.exe
./build/Release/cybou.exe
```

Linux/macOS: configure with the matching Qt kit and compiler. Source structure is portable; builds, window integration and packaging on these platforms remain unverified. macOS bundle icon packaging is outstanding.

## Features

- Adaptive Omnibox & Control Bar: in 0.7.1, site capability controls and permission indicators are integrated directly into the browser's navigation row between the address field and Downloads, replacing the previous floating page overlay rail. In BROWSE mode, the address displays a compact, verified hostname while available capability icons are shown inline (with hidden controls accessible via an overflow menu ···). In EDIT mode (focus or Ctrl+L), all capability controls hide to give the full URL maximum width. Hover/pinned detail cards, FR/RU strings, keyboard navigation and separate policy/event marks are preserved. See [integration report](docs/CONTROL_BAR_INTEGRATION.md), [omnibox integration](docs/OMNIBOX_071_INTEGRATION.md) and [capability matrix](docs/API_CAPABILITY_MATRIX.md).

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
- Settings: DuckDuckGo/Google/Bing/Brave, startup restoration, third-party cookie blocking, per-profile cleanup and saved site permissions.
- Icon in Windows executable, application window, tab fallback and start page.

Shortcuts: Ctrl+L address, Ctrl+T new tab, Ctrl+W close, Ctrl+Shift+T reopen, Ctrl+Tab / Ctrl+Shift+Tab switch, Ctrl+D bookmark, Ctrl+H history, Ctrl+J downloads, Ctrl+F find, Ctrl+Shift+N private window, Ctrl+, settings, Ctrl+O local file, Ctrl+P PDF export, Ctrl+M mute tab, Ctrl+R reload, Alt+Left/Right navigation, F11 fullscreen. Start directly in private mode with --private.

## Privacy and development limits

Certificate errors are rejected; insecure mixed content and local file access from web content are disabled. Only user-initiated HTTP(S) new-window requests become tabs. Chromium sandboxing remains enabled.

Foreground sites can request supported permissions through an origin-labelled allow/deny dialog. Background requests and unimplemented capture types are denied. Persistent ordinary-profile decisions can be reset in Settings; private decisions stay within their window. Revoking a decision does not terminate an existing capture: close the tab to stop it. Native notification presentation remains outstanding.

Control Bar never equates a permission grant with active device use. Permission events and the profile-wide HTTP(S) metadata sample are memory-only and bounded. No bodies, headers, query strings or URL paths are retained by this observer. Cookie filtering remains a global preference affecting all open profiles, rather than a fictional site-only control. New panel strings can be switched FR/RU in Settings; unrelated legacy UI remains French.

Each private window owns a separate in-memory profile, destroyed on close. Private browsing does not anonymize traffic. Explicit bookmarks, preferences and downloaded files remain on disk. History and session URLs are stored locally in QSettings without encryption.

Cleanup supports cookies, HTTP cache, history, permission decisions and saved tabs. Cookie/saved-tab cleanup closes tabs in the current window. Cookie deletion is asynchronous; cache completion is tracked. LocalStorage, IndexedDB and service workers are not removed by this command. Third-party cookie filtering applies to open profiles; it is not a full tracker blocker.

Development build, not security audited. Passwords, extensions UI, physical-printer UI, automatic updates, installer, crash recovery, platform polish and comprehensive security/accessibility testing remain outstanding. Keep Qt/Chromium patched.

### Strict transport mode

Settings → Sécurité offers an opt-in HTTPS-only mode, persisted globally and applied immediately to the interceptors of all open ordinary/private profiles and to future profiles. It blocks HTTP and unencrypted WebSocket (`ws:`) requests, including subresources, local hosts and LAN addresses. There are no automatic upgrades, fallback or hidden host exceptions. Existing connections and already rendered/cached content are not terminated or erased; reload open tabs after enabling. Sites requiring HTTP will stop working while this mode is enabled. A blocked foreground navigation shows an explanation; subresource failures are handled by the page.

HTTP(S) URLs containing credentials are always rejected by the interceptor, independently of strict mode. Page-triggered external protocol launches are disabled through Qt's unknown-scheme policy. WebRTC is restricted to public interfaces; this is not an anonymity guarantee. TLS errors and mixed active content remain rejected. In 0.7.1, Control Bar controls are integrated directly into the navigation bar (Adaptive Omnibox).

Run `./build/Release/cybou.exe --security-test` for URL-policy cases and actual WebEngine network checks: strict HTTP denial, successful load after explicit opt-out, denial of a new same-origin fetch from an already loaded HTTP page after enabling strict mode, credential-URL denial, exact host formatting, IPv6, ACE/IDN punycode and safe display tests. The fixture listens only on loopback and no security preferences are changed by this test.

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
