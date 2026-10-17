# CYBOU — Versioning policy (normative)

**Current declared product source version: `0.3.0-alpha.1`**, as of 17 October 2026. This is an **unqualified engineering alpha**, not a tested release or a public security certification.

## Source of truth

- `CMakeLists.txt` sets product version `0.3.0` plus prerelease identifier `alpha.1`. Both `cybou` and `cybou_tests` receive the same compiled `CYBOU_PRODUCT_VERSION`; `Qt.application.version` displays it in Alpine Dawn.
- The `qt_add_qml_module(... VERSION 1.0)` value is the **QML module API version**, not the CYBOU product version. Do not change it to 0.3.
- Version in screenshots, historic design specs (`0.7.1`, `0.8`, earlier) records **past design milestones**, not present release qualifications. Never silently rewrite a historical report into a current assertion.
- `main` is the sole development branch by owner decision. A release is a **specific immutable Git commit + matching tested binary hash + signed artifacts**; a version bump in main alone is not a published release. Do not create tags or releases for unqualified alpha by assumption.
- The existing CYBOU user profile name/location remains unchanged. Downgrading the product identifier **must not wipe settings or profiles**. Migration compatibility is separately tested.

## Meaning of versions

| Version target | Development stage | Promotion condition |
| --- | --- | --- |
| **0.3.0-alpha.1 — NOW** | UI/product and main capability architecture shaped; existing tests partial | No public security guarantee; OWASP inventory is not yet mapped, broad runtime audit missing |
| **0.4.0-alpha.N** | Testing infrastructure, threat model, complete official OWASP inventory and applicability mapping | Reproducible offline gate and structured evidence; security checks are added, not guessed |
| **0.5.0-alpha.N** | Close high-impact security and functional gaps | Native, negative, boundary, fuzz and browser-device tests; all P0 blockers fixed and checked |
| **0.6.0-beta.N** | Feature freeze for Windows 1.0 scope, cross-component and regression qualification | Full test program runnable, Windows packaging/SBOM/updates defined; realistic soak |
| **1.0.0-rc.N** | Release candidate; no new features | All applicable release gates PASS on the exact candidate; independent review and signed package rehearsals |
| **1.0.0** | First public supported Windows stable release | RC evidence carries to identical source/artifacts; signed distribution and vulnerability response/update procedure verified |

These are **gated stages, not calendar promises or completion percentages**. Repeat an alpha/beta/rc identifier as often as needed. Any significant source change invalidates previously claimed current-HEAD evidence; rerun affected suites and full release gates before promotion. Unverified Linux/macOS builds do **not** inherit the Windows release claim.

## Rules

Use SemVer 2.0.0; pre-1.0 allows change without stability commitment. Bump alpha/beta ordinal for tested snapshot changes, minor for a new maturity stage only after its exit gate, patch for scoped fixes in the same stage. A new high severity finding blocks RC; do not raise the version to hide the risk. After 1.0, MAJOR/MINOR/PATCH follows compatibility and support policy.

Every promoted build records: version, full SHA, clean source tree, CMake/Qt/Chromium versions, Windows build kit, binaries SHA-256, signed package, SBOM/dependency inventory, all security test reports, threat review, vulnerabilities and fixed/accepted exceptions. See [RELEASE_CRITERIA](RELEASE_CRITERIA.md), [SECURITY_VERIFICATION_FRAMEWORK](SECURITY_VERIFICATION_FRAMEWORK.md), [VALIDATION](VALIDATION.md) and [ADR 0008](ADR/0008-evidence-driven-versioning.md).

**Public claims:** `implemented` means code exists; `tested (historical)` means an older run was recorded; `verified on candidate` needs current-SHA logs and proof; `secure` or `OWASP compliant` MUST NOT be used as a blanket claim.
