# CYBOU Browser — Roadmap to 1.0.0

**Current source: 0.3.0-alpha.1 · no public release · security qualification incomplete.** Work takes place on `main`.

## Actual code baseline

| Area | Currently implemented | Missing proof/work for 1.0 |
| --- | --- | --- |
| UI / Alpine Dawn | C++/QML browser, tabs, compact floating new-tab content, glass search, 11 permanent navigation controls, glass popovers and Cloud Glass 2.0 | Keyboard/screen-reader, DPI/resolution, many-tab and crash/recovery matrix |
| Browser tools | History, bookmarks, downloads, PDF export, Find, search providers, private profiles | Data cleanup, reliability and OS integration verification |
| Permissions | Native PermissionCoordinator, per-document decisions and explicit controls | Real capture stop, combined mic/camera, iframe and stale-request adversarial tests |
| Network | Strict HTTPS, temporary HTTP access, NavigationGuard, categorized RuleEngine, tracker exceptions | DNS/PNA/redirect/WebRTC threat tests, list updates/false positives, engine sandbox proof |
| Cloud | Provider-neutral C++ catalog/manager, Proton default and role overrides, Nextcloud HTTPS configuration, web app launcher and local icons | Real login/2FA compatibility, redirects, no-prefetch/permission regression, clarity of product vs web-link shortcuts |
| Test support | `cybou_tests`, six native suites, `tools/security/owasp_gate.py`, machine-readable risk cases | Pinned ASVS/WSTG inventory and mappings, evidence and independent pentest |
| Distribution | CMake Windows executable | Reproducible package, SBOM, signing, secure update/rollback and response process |

No third-party extension platform or partner-granted browser privileges are permitted.

## Source audit — 2026-10-18

This review covers the local Windows implementation and test infrastructure. Implemented behavior is distinct from release qualification. Current milestone remains **0.3.0-alpha.1**; there is no stable release candidate.

| Work item | Code evidence | State / remaining work |
| --- | --- | --- |
| New-tab, tabs and cloud controls | `qml/Main.qml`, `NewTabPage.qml`, `CloudAppLauncher.qml` | Implemented, including 12 px new-tab gap and shared 36 px cloud-button sizing; complete keyboard/DPI/many-tab checks still required |
| Profiles, history, bookmarks, downloads and PDF | `src/browserstore.h`, `qml/BrowserTab.qml`, `tests/test_main.cpp` | Implemented; smoke fixtures exercise normal paths; crash recovery and data-deletion matrix still required |
| Temporary permissions and document context | `src/security/PermissionCoordinator.cpp`, `SiteContext.h`, `PermissionLease.h` | Native request queue, expiry, document/profile checks and revoke paths exist; hardware capture-stop and hostile iframe proof remain open |
| Restricted browsing | `NavigationGuard.h`, `BrowserTab.qml`, `Main.qml` | IDN/port detection, restricted settings, muted audio, permission/download deny and user override exist; IDN decoding regression addressed in this iteration; full navigation/override regression remains open |
| HTTPS and tracker exceptions | `NetworkPolicy.h`, `HttpNavigationLease.h`, `TrackerExceptionRegistry.h`, `TrackerBlocker.h` | Scoped temporary exceptions and local fixtures exist; TLS/DNS/PNA/redirect adversarial coverage remains incomplete |
| Filtering | `src/security/RuleEngine.h` | Small built-in categorized suffix list with protected endpoints; no qualified list-update pipeline or broad effectiveness claim |
| Cloud apps | `src/cloud/`, `qml/CloudSettingsSection.qml` | Local catalog, role resolver, Nextcloud configuration and launcher implemented; real vendor login/2FA and document/media workflows unverified |
| Test evidence | `tests/test_main.cpp`, `tools/security/owasp_gate.py` | Six suites exist; previous report had empty suite logs and two failures. Diagnostics capture, empty-log rejection and source-diff identification added; fresh results tracked in VALIDATION |
| Release infrastructure | `CMakeLists.txt`, `tests/security/standards.lock.json` | Windows build exists; official dataset hashes are null. No installer/signing/updater/SBOM pipeline found in reviewed tracked source |

### Immediate delivery queue

- [x] **R1 — repeatable baseline (local development, 2026-10-18):** fix IDN failure, wait for the real hover delay, capture nonempty suite logs, record current source/binary identity, obtain six green suites.
- [ ] **R2 — inventory:** vendor the pinned official ASVS/WSTG inputs, validate parsers and control counts, assign applicability/owners. No automatic N/A or fabricated PASS.
- [ ] **R3 — browser-specific negative fixtures:** restricted-mode origin/redirect/override lifecycle, cross-frame permissions, real capture revoke, downloads and local-network boundaries.
- [ ] **R4 — Windows product qualification:** cloud login/2FA, keyboard/FR/RU/DPI, many tabs, crash/session recovery and cleanup.
- [ ] **R5 — distributable candidate:** reproducible installer, licenses/SBOM, signing, safe update/rollback, independent review and matching candidate evidence.

R1 passed locally: Release built, six gate self-tests and six native suites passed with nonempty logs and a saved tested patch. R2 is next; R3–R5 remain open. This does not authorize a 1.0 claim. See [VALIDATION](VALIDATION.md) for current executed results.

## Gates

| Version | Work to complete | Exit condition |
| --- | --- | --- |
| **0.3.0-alpha.1 — CURRENT** | Keep implementation and limitations aligned with source | Not a release |
| **0.4.0-alpha.N** | Pin official ASVS 5.0.0/WSTG 4.2; inventory 100% IDs, owners, accepted applicability; automated negative fixtures, logs, coverage schema | Every standard item dispositioned, repeatable test framework, strict FAIL on missing evidence |
| **0.5.0-alpha.N** | Fix origin/iframe/permissions lifecycle, actual media revocation, TLS/PNA/DNS/redirect issues, tracking-rule limits, sandbox process constraints; add fuzz/system/hardware tests | All P0 risk cases implemented and tested; no open unreviewed critical/high |
| **0.6.0-beta.N** | Freeze Windows 1.0 scope; complete cloud compatibility, accessibility, localization, browser lifecycle and long-running stress | Full regression, repeated real-device tests, tested Windows build package |
| **1.0.0-rc.N** | Independent security review, dependency/SBOM audit, signed binary/update/rollback, candidate-specific OWASP evidence | Every applicable pinned ASVS/WSTG requirement and browser-specific test verified; all formal release gates GO |
| **1.0.0** | Public Windows x64 product and maintenance process | Exact qualified candidate signed and published |

No estimated date or percentage stands in place of actual completion.

## Current execution order

1. **Testing foundation (P0):** obtain official pinned inventories, complete control applicability, implement deterministic local fixtures and connect each applicable requirement to executed evidence.
2. **Browser security (P0):** renderer and OS boundaries, permissions, WebRTC/media revoke, local/private network, TLS/redirects, download and process abuse, fuzzing and negative tests.
3. **Engine/release supply chain (P0):** Qt/Chromium vulnerability management, actual sandbox, SBOM, signing, update/rollback.
4. **Product quality (P1):** cloud account compatibility, realistic ad/tracker filtering, UX/FR/RU accessibility, keyboard navigation, device and stress regressions.
5. **Release decision (P0):** peer review, external pentest, audit findings, signed candidate, same-SHA results and published support/incident process.

**Only actual tests on the target source/binary may be recorded as PASS.** Missing report, unknown, untested or blocked remains a release blocker.

[TEST_PLAN](TEST_PLAN.md) · [THREAT_MODEL](THREAT_MODEL.md) · [RELEASE_CRITERIA](RELEASE_CRITERIA.md) · [VERSIONING](VERSIONING.md)
