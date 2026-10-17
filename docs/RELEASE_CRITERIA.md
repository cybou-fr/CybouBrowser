# CYBOU — 1.0.0 release criteria / go-no-go

**Current status: NO-GO.** CYBOU source currently declares `0.3.0-alpha.1`. This checklist is the mandatory **future gate**; current source/binary evidence must be supplied for each condition.

## Scope

First supported production release: Windows x64 with specifically documented, tested features. macOS/Linux can be separate supported platforms **only after their own completed evidence**. The product remains first-party C++/QML/Qt WebEngine, no third-party extension store/runtime (ADR 0006). Proton is a default shortcut suite, not a privileged browser trust anchor.

## Required GO decisions (every item)

| Gate | Required evidence | Current |
| --- | --- | --- |
| Scope & threat model | Frozen Windows 1.0 behavior; reviewed trust boundaries and exploit scenarios | **BLOCKED** |
| OWASP inventory | Pinned full ASVS 5.0.0 and WSTG 4.2 datasets, **100% disposition** with peer-reviewed N/A | **BLOCKED** |
| Applicable OWASP verification | Every applicable requirement maps to executed case and recent PASS evidence; no stale/skip/fail | **BLOCKED** |
| Additional risk review | OWASP Top 10:2025 all ten, SCVS, SAMM, desktop-browser threats documented | **BLOCKED** |
| C++/QML security & functional | All six existing suites plus added fuzz, negative, system/hardware, 11-button UX and cloud checks on candidate | **BLOCKED** |
| Engine/sandbox | Supported patched Qt/Chromium runtime, verified renderer sandbox/site boundaries and safe production flags | **BLOCKED** |
| Real devices & services | Capture/revoke/iframe/OS, sessions and HTTPS cloud compatibility tests in isolated accounts | **BLOCKED** |
| Supply chain & distribution | SBOM/licenses, signed Windows installer, verification, secure updater/rollback, patch and incident-response process | **BLOCKED** |
| Findings | **0 open Critical/High**; Medium requires recorded owner/risk/expiry and public-impact review; no accepted unknown | **BLOCKED** |
| Independent assurance | Security peer review and external pentest or written no-go; red-team retest for fixed findings | **BLOCKED** |
| Candidate reproducibility | Clean commit, binary SHA-256, signed artifact, all evidence attached to matching candidate and dates | **BLOCKED** |
| UX, a11y, documentation | End-to-end FR/RU, multiple DPI/screens, no misleading safety claims, docs align to candidate | **BLOCKED** |

## Release sequence

1. Implement code/test improvements directly on `main`; no branches by owner instruction. Before touching code verify HEAD and avoid overwriting concurrent agent commits.
2. After feature freeze, make `1.0.0-rc.1` candidate identifier; build artifact and fingerprint. A commit with changed source requires a **new candidate** and new proof.
3. Run [TEST_PLAN](TEST_PLAN.md) and `tools/security/owasp_gate.py --mode release` with official pinned datasets; preserve logs outside user profile. Independent reviewer examines manual results, N/A justification and source diff.
4. If any gate is missing, severity too high, patch level stale or coverage incomplete: **NO-GO**, continue on `main` without creating a stable tag.
5. If everything passes, produce signed release with exact binary hash, SBOM, release notes, fixed vulnerabilities, rollback/install instructions and response contact. Tag exactly matching source and publish `1.0.0`.

**Prohibited release claims:** "OWASP certified", "fully secure", "Chromium isolated" or "no tracking" without bounded, documented independently auditable evidence. Compliance with OWASP guidance is not a guarantee that an attack is impossible.
