
> **Состояние на 17.10.2026:** текущий продукт — C++20, C++ PermissionCoordinator с независимыми deadline, 11 кнопок inline в **одной** навигационной строке, интерактивные glass hover panels, HTTPS strict и временный single-URL HTTP opt-in в отдельном профиле. Ранние audit-строки в этом документе сохранены как историческая справка (часть описывает 0.5/0.7 с другими layout/dialog сценариями) и **не являются сегодняшним UI-контрактом**. Актуальные требования: [CAPABILITY_CONTRACT](CAPABILITY_CONTRACT.md), [CONTROL_BAR_UX](CONTROL_BAR_UX.md), [SECURITY_UX_EVOLUTION](SECURITY_UX_EVOLUTION.md), [ROADMAP](ROADMAP.md).

# Observable Browsing capability matrix — current working tree

| Area | Real observation | Control | Scope | Limitation |
|---|---|---|---|---|
| camera/mic/location/notifications/clipboard | native permissionRequested, originating request object | current grant only, no queryPermission pregrant | document, requester, tab, profile; 15s pending / independent 60s grant deadlines; earliest unloads document | grant does not prove use |
| screen | unsupported native capture request | deny | request | chooser unavailable |
| JS/localStorage | view setting readback | toggle + guarded reload/undo | tab + origin, memory | no DOM/storage inventory |
| popups | newWindowRequested | gesture default; temporary allow/deny | tab + origin | Chromium-suppressed attempts unknown |
| cookies | profile cookie filter | explicit global setting | all open profiles | no complete count |
| network | sanitized interceptor events incl. blocked result/reason | strict HTTPS preference + limited built-in tracker rules | profile; firstParty metadata, never proven tab attribution | no bodies, exhaustive ad rules or proxy guarantee |

Normative details: [CAPABILITY_CONTRACT](CAPABILITY_CONTRACT.md). Build/test evidence: [VALIDATION](VALIDATION.md).

---

## Historical matrix (preserved)

> Historical report. Current normative contracts: [CAPABILITY_CONTRACT](CAPABILITY_CONTRACT.md), [ROADMAP](ROADMAP.md). Implementation claims below describe the previous version.

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

## Runtime and delivered verification

Runtime diagnostic output from the installed kit: Qt `6.12.0`, WebEngine `6.140.0`, Chromium `140.0.7339.225`, reported Chromium security patch `152.0.7977.83`. These are raw API-reported values, not a certification that the runtime is current or ready for public distribution.

Verified: QML→C++ QWebEnginePermission value transfer on a real notifications request; grant/deny/reset readback; invalidation on tab switch; per-origin JS/localStorage reload behavior across 127.0.0.1 and localhost; default/explicit popup gate; private policy/event separation; profile destruction; bounded interceptor sample during an 80-request burst; all four positions, hover corridor, mouse-out, pin/Escape, roving keyboard focus, non-interception of page clicks, unchanged page viewport, drawer/fullscreen policy, FR/RU catalog, actual DPR 1.0/1.25/1.5/2.0 at 820×560 minimum window. Position persisted across separate processes.

Not verified: physical camera/mic capture; stopping a cached grant/active stream; screen chooser; a real location provider; native notifications; cross-origin iframe hardware permission flows; touch/stylus devices; OS high-contrast/reduced-motion toggling during a run; mixed-monitor migration; third-party cookie fixtures beyond the pre-existing filter's tested preference path. Those limits remain explicit and are not replaced with invented activity indicators.
