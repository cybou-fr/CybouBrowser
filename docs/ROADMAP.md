# CYBOU Browser — roadmap to 1.0.0 (gated)

**Declared current source:** `0.3.0-alpha.1` · **stage: engineering alpha / architecture and product contour**. It is **not** 0.8/0.9 and not a security-qualified beta. The old "0.8" was an informal UI-design milestone; it did not pass a full browser-security release gate. See [VERSIONING](VERSIONING.md) and accepted [ADR 0008](ADR/0008-evidence-driven-versioning.md).

**Rule:** version is a *gate outcome*, not feature-completion percentage. `implemented` = code exists; `historically tested` = previous local log; `candidate verified` = evidence on exact SHA and binary; `blocked` = proof missing/failed. There is no current-candidate OWASP full-coverage PASS. Historical results remain in [VALIDATION](VALIDATION.md).

## Current engineering baseline (not production assurance)

| Area | Code exists | Not yet demonstrated at release level |
| --- | --- | --- |
| Browser UX | Alpine Dawn, tabs, history/bookmarks/downloads, QML glass, permanent 11-icon Control Bar | Accessibility and DPI complete matrix, crash/recovery, clean packaging |
| Permissions | Native PermissionCoordinator, queue and origin/frame policies, revoke code and historical fixtures | Full OWASP mapping, real camera/mic/screen hardware and adversarial frame chains |
| Network / transport | HTTPS policy, temporary HTTP context, request interception, ad/tracker rules and exceptions, NavigationGuard | Comprehensive redirect/DNS/WebRTC leaks, race/fuzz tests, per-platform sandbox proof |
| Cloud | Local C++ catalog / selectable provider and role, FrostedGlassSurface and launcher, Proton default | Real login/2FA/media/SSO compatibility, cloud URL reviews, security regression and no-prefetch proof |
| Threat-driven security | Some native and smoke tests have recorded PASS | Entire pinned ASVS 5.0.0 / WSTG 4.2 inventory, SCVS/SAMM review, fuzz corpus, external pentest |
| Distribution | Windows MSVC local build recorded historically | Signed installer/update/rollback, SBOM, reproducibility, verified Chromium security-patch cadence |

No third-party extension runtime or packaged unsolicited external executables (ADR 0006). Provider branding never grants privileges (ADR 0007).

## Milestones and exit gates

| Target | Required deliverables | Mandatory evidence before promotion |
| --- | --- | --- |
| **0.3.0-alpha.1 — CURRENT** | Product/UI architecture and built-in controls; basic tests, unqualified alpha | Version labels aligned; disclose gaps; do not claim secure |
| **0.4.0-alpha.N — Security Engineering Foundation** | OWASP 5.0.0 ASVS official CSV and WSTG 4.2 IDs pinned, **every item triaged**; STRIDE-like browser threat model; repeatable offline gate; per-case ownership; fixture architecture | 100% applicability accounting, approved N/A, raw baseline logs on exact SHA, failing gates for missing evidence |
| **0.5.0-alpha.N — Security & Function Hardening** | Fix unsafe/unsupported logic: permissions/frame navigation, real revoke, HTTPS/cert errors, DNS rebinding/PNA, network RuleEngine, origin-scoped exceptions, profile & sandbox; fuzzers; safe cloud routes | Real negative + metamorphic + fuzz tests, hardware and OS integration, no open unreviewed critical/high |
| **0.6.0-beta.N — Product Qualification** | Freeze Windows 1.0 features, complete UX/cloud path, polish accessibility and localization, restore/history/downloads, resource usage, upgrade/downgrade and crashes | Stable multi-resolution and hardware tests; extended soak; documented cross-app compatibility; real package smoke |
| **1.0.0-rc.N — Candidate / audit** | Security fixes only; signed Windows artifact, SBOM/provenance, updater/rollback, operational response; independent penetration test | Exact SHA/hash, **every applicable pinned ASVS/WSTG mapped to current PASS**, OWASP Top 10 / SCVS / SAMM reviews, no critical/high, go/no-go signed |
| **1.0.0 — first supported release** | Production Windows x64 version, documented guarantees and limits, update channel | Candidate artifacts unchanged or fully revalidated; all [RELEASE_CRITERIA](RELEASE_CRITERIA.md) GO |

No dates invented. There is **no need for `0.8` or `0.9` on this path**. There may be multiple alpha/beta/rc patches; milestone changes are earned by gates, not by merging a feature.

## Workstreams (priority order)

1. **W0 / P0 — Test framework and reproducible baseline:** run `tools/security/owasp_gate.py` offline; pin ASVS/WSTG official inventories; build complete applicability map. Make status BLOCKED until test IDs, owners, logs and exact candidate hashes are present. [SECURITY_VERIFICATION_FRAMEWORK](SECURITY_VERIFICATION_FRAMEWORK.md).
2. **W1 / P0 — Renderer/browser threat boundaries:** top/requester origin, stale and concurrent permission IDs, revoke actually stops capture; frame sandbox and process isolation, Chromium/Qt release flags, WebRTC/DNS/IPv6/PNA negative testing, TLS errors, downloads.
3. **W2 / P0 — Supply chain:** inventory Qt/Chromium DLLs, patch policy, SBOM, component signatures, no extension loaders, safe installer/update, rollback and CVE response.
4. **W3 / P1 — Functional completeness:** real cloud provider compatibility without privilege, camera/mic/Meet support, accurate ad/tracker rules, sessions, file handling, many-tabs stability and historical data migrations.
5. **W4 / P1 — Consistent UX:** 11 controls always visible, Dark Glass cloud launcher, FR/RU, keyboard a11y, no misleading policy/event labels, all prescribed viewport/DPR checks.
6. **W5 / P0 — Release audit:** external adversarial review, remediation retest and exact-candidate evidence, strict GO/NO-GO.

## Process rules

Everything is committed to **`main` only**, no branches (owner decision). Before editing verify current HEAD; preserve parallel agents' work. Work locally without relying on GitHub Actions until available. Never promote based solely on code review or an old local test claim. Run linked suites after any relevant change, full runner and formal OWASP coverage/release gate for stage promotions. No automatic exceptions for Proton, Google or Microsoft and no imported third-party extension code.

**Authoritative links:** [DOCUMENTATION_INDEX](DOCUMENTATION_INDEX.md), [TEST_PLAN](TEST_PLAN.md), [THREAT_MODEL](THREAT_MODEL.md), [RELEASE_CRITERIA](RELEASE_CRITERIA.md), [VALIDATION](VALIDATION.md). Prior S0/S1/S2 labels denote feature tickets and historical attempts, **not maturity percentages or proof of release safety**.
