# Implementation handoff — CYBOU Browser 0.3 → Control Bar 1.0

Repository: https://github.com/cybou-fr/CybouBrowser  
Inspected `main` HEAD: `d2f1a99c9d5ac7af94b50966ae260fe58317d7b4` (2026-10-16). Re-check head before editing.

**Read in order:**
1. `CYBOU_CONTROL_BAR_V1_AGENT_SPEC.md` — full approved UX/safety contract.
2. `CYBOU_CONTROL_BAR_REPO_ALIGNMENT_2026-10-16.md` — factual audit and prioritized steps, **takes precedence on current code, language, file paths**.

**DO NOT:** start a new browser; move control bar into tabs/address bar; create a sidebar; add CYBOU logo/text to icon rail; use `...` as last button; present fake site activity as real; migrate to C++20 solely for Control Bar; replace existing per-window private profiles; enable unsupported permission types; disable sandbox/certificate checks; introduce a mandatory cloud/AI/network; create long-lived branches or require CI.

**Current baseline:** C++17, Qt 6.9+, README tested with 6.12/MSVC; `Main.qml` ~600 lines, site permissions in `askPermission`/`resolvePermission`, `BrowserStore::configureProfile` and third-party cookie filter, normal and independent private profiles, existing `--smoke-test`.

**Work in `main`, small checked commits.**

Initial execution order:

1. Run baseline build and `--smoke-test`. Record version, exact SHA, PASS/FAIL, and what was not tested; create `docs/API_CAPABILITY_MATRIX.md` for Qt 6.12.
2. Build **single** slim `ControlBar.qml` in page viewport and its `ControlBarIcon.qml`, `CapabilityPopover.qml`, `ControlBarPositionPicker.qml` with exactly 4 placements. Last icon is reposition. UI remains transparent, narrow, seamless, and brand-free. Actual web page, its scrollbar and browser chrome remain operable.
3. Show neutral/unknown for unwired capabilities; use fake activity only in dedicated test fixture. Add persistent global positioning preference to existing QSettings facade.
4. Hook real permission events without breaking existing foreground checks/modal fallback. Replace permission type numbers with named enums. Handle ephemeral and persistent scope separately, including screen capture types after capability spike. Handle changed tab/origin and combined camera/microphone.
5. Add persistent per-site JS/popup policies and safe reload semantics with existing per-tab views; compose global/per-site cookie filter.
6. Only then add bounded network observer through profile interceptor with verified attribution, otherwise label it profile activity. No fake exfiltration claims.
7. Run regression suite / screenshot all 4 placements / keyboard/mouse-out/125–200%DPI/min window/private windows. Publish known security and release limitations.

**Required response after every increment:** code paths + commit SHA; what works in real engine; what is intentionally not supported; tests actually executed and result; next minimal increment. Preserve stable normal browsing at each step. Do not declare first public release ready just because UI is polished.
