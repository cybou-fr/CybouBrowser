> Historical report. Current normative contracts: [CAPABILITY_CONTRACT](CAPABILITY_CONTRACT.md), [ROADMAP](ROADMAP.md). Implementation claims below describe the previous version.

# CYBOU 0.7.1 adaptive omnibox integration

## Baseline (2026-10-16)

Actual local and GitHub main: `095257c3461081cebf4ad3e3ae2452fe385a0662`, ahead of the supplied audit's `9a809aa`. Download launch review/protection must be retained along with HTTPS-only. Qt 6.12.0 / MSVC 2022 x64, C++17. Release build exit 0.

Baseline logs in `build/baseline-071-*.log`: smoke exit 0; first controlbar run exit 2 at the old hover bridge assertion, retry exit 0; security exit 0. This records a pre-existing intermittent hover failure, not an unqualified all-pass baseline. The replacement must test the actual pointer corridor using window coordinates. Baseline screenshot: `build/baseline-071-preview.png`.

The two original input documents are retained in `omnibox-source/`. Their repository audit and source line numbers are historical observations. The active contract is BROWSE/EDIT in one navigation row, deterministic hidden-only overflow, one existing capability controller/popover, and preservation of the security backend and browser features. No cloud service, branch, force push or remote publication is required.

## Phase Delivery & Commit Sequence (P1–P5)

- **P1 — Foundation & BROWSE/EDIT Layout**:
  - `src/ui/AddressDisplay.h`: verified host parser (`AddressDisplay::host()`) with exact punycode (`QUrl::toAce`), IPv6 brackets, port numbers, neutral `about:blank` and `file:` handling.
  - `src/browserstore.h`: exposed `displayHost()` to QML.
  - `src/security/SecurityTests.h`: automated test cases for address formatting.
  - `qml/AdaptiveAddress.qml`: separate visual display host (`Text`) and edit field (`TextField`).
  - `qml/ControlBarInline.qml`: inline capability bar with `widthBudget` calculation and zero-width EDIT state.

- **P2 — Responsive Overflow**:
  - `qml/ControlBarOverflow.qml`: dropdown popup displaying only hidden capabilities in deterministic model order.
  - Integration in `ControlBarInline.qml` showing `···` when `visibleCount < icons.count`.

- **P3 — Shared Details Popover & Host Placement**:
  - `qml/CapabilityPopover.qml`: single details host mapped into `Overlay.overlay` below the navigation bar (`y >= 74`).
  - Coordinated pointer corridor, hover timers (170ms open, 220ms close), click-pinning and Escape handling.

- **P4 — Control Bar Icon Polish & Localization**:
  - `qml/ControlBarIcon.qml`: updated contrast, accessible roving focus stop, and visual indicators.
  - `translations/cybou_ru.ts`: updated Russian translations.

- **P5 — Omnibox Integration, Harness & Cleanup**:
  - `qml/Main.qml`: integrated `ControlBarInline` into navigation row, removed floating `ControlBar`.
  - `qml/ControlBarHarness.qml`: updated test scenarios covering A01–A24.
  - `src/main.cpp`: updated test hooks and event dispatching.
  - `qml/SettingsPanel.qml`: removed obsolete position picker references.
  - Removed obsolete files `qml/ControlBar.qml` and `qml/ControlBarPositionPicker.qml`.
  - `CMakeLists.txt`: updated target definition and version 0.7.1.
  - Documentation: updated `README.md`, `docs/CONTROL_BAR_INTEGRATION.md`, and this file.

## Verification Matrix

| Suite | Command | Exit Code | Status |
|---|---|:---:|:---:|
| Security & URL Display | `./build/Release/cybou.exe --security-test` | 0 | PASS |
| Adaptive Omnibox & Control Bar | `./build/Release/cybou.exe --controlbar-test` | 0 | PASS |
| Smoke Suite | `./build/Release/cybou.exe --smoke-test` | 0 | PASS |

Generated test snapshots:
- `build/omnibox-browse-1360.png`
- `build/omnibox-edit-1360.png`
- `build/omnibox-browse-820.png`
- `build/omnibox-edit-820.png`
- `build/omnibox-overflow.png`
- `build/omnibox-details.png`
- `build/omnibox-blank.png`
- `build/omnibox-file.png`
- `build/omnibox-ru.png`
