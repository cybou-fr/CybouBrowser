> Historical report. Current normative contracts: [CAPABILITY_CONTRACT](CAPABILITY_CONTRACT.md), [ROADMAP](ROADMAP.md). Implementation claims below describe the previous version.

# CYBOU 0.7.1 — Adaptive Omnibox & Control Bar integration

> [!NOTE]
> CYBOU 0.7.1 supersedes the earlier 0.6 floating acrylic rail overlay. The 11 site capabilities, permission models, origin policies and network telemetry are now directly integrated into the single navigation row between the address display and the Downloads button.

## 0.7.1 Delivered architecture

### Navigation row integration
The browser maintains two 36 logical px high chrome rows: the draggable tab/title bar and the unified navigation bar. In 0.7.1, there is no separate floating panel overlaying the page.

1. **BROWSE mode (`!address.activeFocus`)**:
   - The address display shows a compact, verified hostname (`displayHost`, formatted via `src/ui/AddressDisplay.h`).
   - The remaining width budget (`widthBudget`) between the address and the action buttons is allocated to `ControlBarInline.qml`.
   - Available capability icons are displayed directly inline up to the budget. If all 11 fit, no overflow is shown.
   - If not all icons fit, an overflow button `···` appears containing **only** the hidden capabilities in their fixed deterministic order.
   - Clicking `···` opens `ControlBarOverflow.qml` without stealing or corrupting navigation focus.

2. **EDIT mode (`address.activeFocus`)**:
   - Triggered by clicking the address area or pressing `Ctrl+L`.
   - All inline capability icons and the `···` overflow button are completely hidden (`effectiveWidth = 0`).
   - The address `TextField` expands to the maximum available width, displaying the complete, editable raw URL with selection.
   - Address suggestions (bookmarks and history) open beneath the expanded field.
   - Pressing `Escape` restores the canonical URL without navigating and returns focus.

3. **Shared detail card & consent host (`CapabilityPopover.qml`)**:
   - Hosted under `Overlay.overlay`, positioned deterministically below the navigation bar (`y >= 74`).
   - Activated via hover (~170ms open delay, ~220ms exit delay with pointer corridor into card) or click (pins card).
   - Selecting any item in `ControlBarOverflow` opens this same shared detail card anchored to the overflow button.
   - Never triggers automatic permission grants upon hover. Native permission confirmation dialogs remain the secure modal authority.

4. **Address display security (`src/ui/AddressDisplay.h`)**:
   - Exact ACE punycode conversion for IDN domains to mitigate Unicode confusable attacks.
   - Bracket notation for IPv6 (`[::1]:port`).
   - Explicit port display for non-default HTTP/HTTPS ports.
   - Neutral handling for `about:blank` (empty display host, no fake warnings or question marks) and `file:` schemes.
   - The raw URL is never modified by cosmetic hostname formatting.

## Changed files in 0.7.1

- `CMakeLists.txt`: updated version to 0.7.1; added `src/ui/AddressDisplay.h`, `qml/AdaptiveAddress.qml`, `qml/ControlBarInline.qml`, `qml/ControlBarOverflow.qml`; removed superseded `qml/ControlBar.qml` and `qml/ControlBarPositionPicker.qml`.
- `src/ui/AddressDisplay.h`: stateless address display formatter.
- `src/browserstore.h`: exposes `displayHost()` to QML.
- `src/security/SecurityTests.h`: unit tests for `AddressDisplay::host` (ACE/IDN, IPv6, port, blank, local files).
- `qml/AdaptiveAddress.qml`: split BROWSE label and EDIT `TextField`.
- `qml/ControlBarInline.qml`: budget-based inline capability icon row.
- `qml/ControlBarOverflow.qml`: dropdown for hidden capabilities.
- `qml/ControlBarIcon.qml`: updated contrast, accessible roving focus and state styling.
- `qml/CapabilityPopover.qml`: shared detail popover anchored to browser popup host.
- `qml/Main.qml`: embedded `ControlBarInline` into `navRow`; removed floating rail.
- `qml/SettingsPanel.qml`: updated explanatory text, removed obsolete position picker references.
- `qml/ControlBarHarness.qml`: automated omnibox test suite (A01–A24 coverage).
- `src/main.cpp`: updated test hooks and event dispatching.
- `translations/cybou_ru.ts`: updated Russian translation entries.

## Verified test suites

All three test suites run fully automated using the built-in loopback test server:

1. `./build/Release/cybou.exe --security-test`
   - Strict HTTPS-only interceptor checks.
   - Download policy dangerous file-extension blocking.
   - `AddressDisplay` host verification: ACE/IDN, IPv6, port, blank, data and local file.
   - **Result: PASS (exit code 0)**

2. `./build/Release/cybou.exe --controlbar-test`
   - BROWSE mode at 1360px (all 11 icons inline, no overflow).
   - EDIT mode at 1360px (Ctrl+L expands address, hides all 11 icons, selects full raw URL).
   - BROWSE mode at 820px (compact host, partial inline icons + `···` overflow menu).
   - Overflow dropdown: contains only hidden capabilities in stable order, keyboard navigation.
   - Shared `CapabilityPopover` opening from both inline icons and overflow.
   - Pointer corridor hover/mouse-out, click-to-pin, Escape-to-dismiss.
   - Zero-budget fallback (all 11 icons in overflow, minimum address width preserved).
   - Neutral `about:blank` and `file:` displays.
   - Russian localization switch and live label updates.
   - Webpage click pass-through outside popovers.
   - **Result: PASS (exit code 0)**

3. `./build/Release/cybou.exe --smoke-test`
   - Tab dragging and reordering via mouse events.
   - Address suggestions keyboard navigation.
   - Local bookmarks and history.
   - Independent private windows and in-memory profile destruction.
   - PDF export and 4 MiB resumable download.
   - **Result: PASS (exit code 0)**

### Generated snapshots (`build/`)
- `omnibox-browse-1360.png` — wide BROWSE mode (all 11 icons inline).
- `omnibox-edit-1360.png` — wide EDIT mode (expanded raw URL, controls hidden).
- `omnibox-browse-820.png` — narrow BROWSE mode with `···` overflow.
- `omnibox-edit-820.png` — narrow EDIT mode with expanded address.
- `omnibox-overflow.png` — overflow dropdown menu open.
- `omnibox-details.png` — shared detail card open below navigation bar.
- `omnibox-blank.png` — clean `about:blank` start state.
- `omnibox-file.png` — local file display.
- `omnibox-ru.png` — Russian localization of capability detail.

---

## Historical reference: CYBOU 0.6 (superseded)

In 0.6, the Control Bar was delivered as a 36px translucent acrylic capsule overlaying the webpage viewport with four selectable placements (top/right/bottom/left). That placement model was superseded in 0.7.1 by the Adaptive Omnibox architecture documented above.
