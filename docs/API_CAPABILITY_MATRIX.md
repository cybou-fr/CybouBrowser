# Control Bar integration — installed Qt capability audit

Baseline: local main `687c1d10463128554698a31798d26a5afc3d2691`, CYBOU 0.5, C++17, Qt 6.12.0 MSVC 2022 x64. The supplied alignment audit targets older 0.3; existing 0.5 features and independent private windows take precedence for integration.

Baseline executed on 2026-10-16: existing Release `--smoke-test`, exit 0. TAB_DRAG_TEST, ADDRESS_SUGGESTIONS_TEST, PRIVATE_WINDOW_TEST, CYBOU_SMOKE_TEST and PDF_EXPORT_TEST all PASS. Log: build/controlbar-baseline.log. No CI or remote repository assumptions were used.

Installed primary sources inspected: qwebenginepermission.h/.cpp, qquickwebengineprofile.h, qwebengineurlrequestinfo.h, QtWebEngine/plugins.qmltypes. Runtime Chromium version and final behavior results are recorded by the integration harness, not inferred from Qt's release number.

| Capability | Supported contract / limits |
|---|---|
| Camera / microphone | Actual permission requests; ASK/DENY policy, explicit live-request grant. `isPersistent` is false. No recording/activity detection claim. |
| Screen | Enum exists, but desktop-source chooser is not integrated or verified. Unavailable; requests denied. |
| Geolocation | Origin/profile persistent permission API. Grant/reset can be verified; physical position backend and accuracy not verified. |
| Notifications | Persistent permission API; native desktop presentation is not integrated. |
| Clipboard | Persistent ClipboardReadWrite permission path; not exhaustive clipboard read/write monitoring. |
| JavaScript | Per-view javascriptEnabled, origin policy applied before navigation; explicit reload needed for current-document effects. |
| Popups | Qt newWindowRequested event and per-view javascriptCanOpenWindows plus foreground/user-gesture gate. Engine-internal suppressed attempts cannot be counted. |
| Cookies | Existing single profile-wide third-party filter preserved; no per-site counts or all-storage wipe. |
| localStorage | Per-view localStorageEnabled control only; IndexedDB, service workers, Cache Storage enumeration/deletion remain unavailable. |
| Network | Bounded profile-scoped HTTP(S) interceptor metadata sample. No request bodies, headers, secrets, query/path logging or tab attribution claims. |
| Placement | One QML viewport overlay; four positions; global benign preference, including private windows. |

Material is a translucent acrylic fallback; no expensive live capture/blur of the Chromium surface. Browser chrome remains 36px + 36px, page viewport geometry is unchanged. Rail is hidden during fullscreen and while settings/library/modal browser dialogs are open. Right-side placement reserves 28px for scrollbar/resize access.

## Verification boundary

Do not infer hardware use from grants. Hover never resolves a request. Actual permission origin is separate from top-level origin. Native modal consent remains the fallback. Network sample is in memory, at most 256 entries per profile, 60s visible TTL, 12 displayed rows. Permission events are in memory, bounded to 256 per window/controller, matched by stable tab identity and top/requesting origins.

Final test results and known gaps are recorded in CONTROL_BAR_INTEGRATION.md after implementation.
