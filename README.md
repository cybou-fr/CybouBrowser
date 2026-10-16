# CYBOU Browser 0.3

Native C++17 / Qt Quick browser using Qt WebEngine. Navy, white and red identity follows the supplied CYBOU sheet. The compass/shield icon was reconstructed from that reference with the built-in imagegen tool. assets/cybou.png is embedded in the app; assets/cybou.ico contains 16–256 px Windows icon sizes embedded through assets/cybou.rc.

## Windows build

Requires Qt 6.9+ Quick, Quick Controls, WebEngine and Network; MSVC 2022 x64; CMake. Tested with Qt 6.12.0 and MSVC 19.44.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/msvc2022_64
cmake --build build --config Release --parallel 4
C:/Qt/6.12.0/msvc2022_64/bin/windeployqt.exe --release --qmldir qml --no-translations build/Release/cybou.exe
./build/Release/cybou.exe
```

Linux/macOS: configure with the matching Qt kit and compiler. Source structure is portable; builds, window integration and packaging on these platforms remain unverified. macOS bundle icon packaging is outstanding.

## Features

- Combined draggable tab/title bar; minimize, maximize/restore, close, edge resizing and F11 fullscreen.
- Scrollable tabs, favicons, middle-click close, duplicate and move-left/right menu, reopen closed tabs and keyboard switching.
- URL/search input, back/forward, stop/reload, home and zoom.
- Persistent bookmarks, latest 1000 history entries, filterable library and confirmed history deletion.
- Configurable ordinary session restoration. Private windows are independent, each with an isolated in-memory profile destroyed when its window closes. Ordinary tabs stay open; private visits never enter history/session storage.
- Downloads to the system Downloads directory with progress, cancellation and folder access. Files never open automatically. Download list lasts for the window lifetime.
- Find in page with match counts and previous/next.
- Settings: DuckDuckGo/Google/Bing/Brave, startup restoration, third-party cookie blocking, per-profile cleanup and saved site permissions.
- Icon in Windows executable, application window, tab fallback and start page.

Shortcuts: Ctrl+L address, Ctrl+T new tab, Ctrl+W close, Ctrl+Shift+T reopen, Ctrl+Tab / Ctrl+Shift+Tab switch, Ctrl+D bookmark, Ctrl+H history, Ctrl+J downloads, Ctrl+F find, Ctrl+Shift+N private window, Ctrl+, settings, Ctrl+R reload, Alt+Left/Right navigation, F11 fullscreen. Start directly in private mode with --private.

## Privacy and development limits

Certificate errors are rejected; insecure mixed content and local file access from web content are disabled. Only user-initiated HTTP(S) new-window requests become tabs. Chromium sandboxing remains enabled.

Foreground sites can request supported permissions through an origin-labelled allow/deny dialog. Background requests and unimplemented capture types are denied. Persistent ordinary-profile decisions can be reset in Settings; private decisions stay within their window. Revoking a decision does not terminate an existing capture: close the tab to stop it. Native notification presentation remains outstanding.

Each private window owns a separate in-memory profile, destroyed on close. Private browsing does not anonymize traffic. Explicit bookmarks, preferences and downloaded files remain on disk. History and session URLs are stored locally in QSettings without encryption.

Cleanup supports cookies, HTTP cache, history, permission decisions and saved tabs. Cookie/saved-tab cleanup closes tabs in the current window. Cookie deletion is asynchronous; cache completion is tracked. LocalStorage, IndexedDB and service workers are not removed by this command. Third-party cookie filtering applies to open profiles; it is not a full tracker blocker.

Development build, not security audited. Passwords, extensions UI, print/export UI, automatic updates, installer, crash recovery, platform polish and comprehensive security/accessibility testing remain outstanding. Keep Qt/Chromium patched.

## Integrated smoke test

```powershell
$env:QT_FORCE_STDERR_LOGGING = '1'
./build/Release/cybou.exe --smoke-test
```

Uses a separate QSettings organization and WebEngine storage name. Checks tab identity through moves, close/reopen, bookmarks, history, preference updates, independent private windows and destruction of their profiles. A loopback-only HTTP fixture checks page loading, find matches, a real permission request and grant, cookie removal, permission reset, cache completion and a download into build/test-downloads. Produces build/smoke-preview.png; exits 0 on success and 2 on test failure. No external sites required.
