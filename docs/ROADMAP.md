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
