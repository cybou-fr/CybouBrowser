# CYBOU — current verification status

**Current product source: 0.3.0-alpha.1 · no public release.**

This is a status ledger, **not** a test-results archive. A test is PASS only with the executable, log, commit SHA, binary SHA-256 and environment available for review. This repository does not currently contain a complete test evidence set for the current build. Therefore **the 1.0.0 release decision is BLOCKED**.

| Verification target | Current status | Next required evidence |
| --- | --- | --- |
| Build (Windows x64) | Release build succeeded locally on 2026-10-18 (Windows x64 / Qt 6.12 / MSVC); development working tree | Successful Release build tied to a current full commit |
| Gate self-tests | 6 self-tests passed locally on 2026-10-18, including empty-log rejection and diagnostic hash preservation | Execute and save output |
| Native baseline | 6/6 suites PASS locally on 2026-10-18; nonempty per-suite logs, binary hash and tested patch preserved below | Run `owasp_gate.py --mode baseline` and preserve logs |
| OWASP ASVS/WSTG | Official inputs SHA-pinned: ASVS 345 + WSTG 97; complete 442-entry inventory imported; applicability and evidence mappings untriaged | Review each imported item, assign applicability/owner and verify applicable cases |
| Browser-specific security | Cases in `tests/security/cases.json` are plans, not proof | Deterministic fixtures, stress/fuzz, real devices and attack-driven negative testing |
| Chromium/Qt sandbox | Packaged binary runtime boundary not proved | Inspect renderer sandbox/processes and security patch level |
| Cloud interoperability | C++/QML implementation available; vendor login/2FA and media not qualified | Real supported-platform test accounts and logs |
| Supply chain | Signing/SBOM/update process not qualified | Signed package, SBOM, update/rollback tests and independent review |
| Independent security review | Not performed/evidenced for a release candidate | External pentest, risk review and retest |

Do not record a result by copying a previous note. Add verified machine-readable cases under `tests/security/` and generated logs under a dedicated build/test-artifact location. Use [TEST_PLAN](TEST_PLAN.md), [SECURITY_VERIFICATION_FRAMEWORK](SECURITY_VERIFICATION_FRAMEWORK.md), [RELEASE_CRITERIA](RELEASE_CRITERIA.md).

## Development audit — 2026-10-18

Baseline qualification is the first active item in [ROADMAP](ROADMAP.md). The source audit found a real IDN regression, an early hover assertion and missing per-suite diagnostics. Current changes decode IDNs independently of display whitelist, cover Unicode/ACE and benign IDNs, wait up to two seconds for hover opening, emit Qt diagnostics to stderr and reject empty successful suite logs. Reports also identify dirty working trees and tracked diff hashes; release mode refuses a dirty or unknown tree.

Fresh native results: **6/6 PASS** on the development working tree. The gate reported no baseline errors; all six logs are nonempty and fingerprinted. The earlier report with empty logs is superseded. These are local engineering results, not a signed or independently reviewed release result.

| Suite | Result |
| --- | --- |
| security | PASS; 2757 diagnostic bytes |
| controlbar | PASS; 3995 diagnostic bytes |
| transport | PASS; 1401 diagnostic bytes |
| tracker | PASS; 827 diagnostic bytes |
| surface | PASS; 289 diagnostic bytes |
| smoke | PASS; 435 diagnostic bytes |

Artifact directory: `build/release-audit-2026-10-18/` (ignored, local only). Preserve `baseline.json`, `security-logs/` and `tested-working-tree.patch` together.

- Source base commit: `3cfcf578c9c725cb3f5237384970624404f5d617`.
- Tested working tree: dirty; patch SHA-256 `835effc2533bc68e0a89b046eeb96d4b23017f8173c2df8cd0ed06ac128c450b`. The saved patch hash was checked against the report.
- Test executable SHA-256: `bb853260ff90a5ccce855bc88de49955d7f703f3a3cd8038885ea974e9278a43`.
- Environment: `Windows-11-10.0.26300-SP0`, Python `3.14.7`; Qt/Chromium engine versions appear in suite diagnostics.

This document and the roadmap were updated after the run; those documentation-only edits are not in the tested patch. No application/test code changed after the successful run. Artifact availability on this machine does not imply the files are published in Git. All official inventory, hardware, cloud, packaging and independent-review release gates remain open.

## OWASP inventory import — 2026-10-18

Official ASVS 5.0.0 release CSV and WSTG 4.2 Markdown checklist are stored under `tests/security/standards/`, with provenance and upstream licenses. SHA-256 pins, immutable source commits and exact counts are in `standards.lock.json`. Coverage has 442 unique identifiers: 345 ASVS and 97 WSTG. All 442 remain **untriaged**; no reviewer, N/A waiver or test result was fabricated.

The importer now reads ASVS's actual `req_id` column (`V1.1.1` form), rejects malformed/duplicate identifiers, checks exact pinned counts and compares WSTG IDs with the pinned upstream Markdown. The upstream JSON duplicate is documented in the standards README.

`--mode inventory` successfully imports all 442 entries, then intentionally reports **BLOCKED** because applicability and case evidence remain incomplete. Local diagnostic report: `build/owasp-inventory/report.json`. Native browser code has not changed in this import step, so the earlier native baseline was not rerun.

## First applicability batch — 2026-10-18

20 controls are classified applicable with code references, browser-specific rationale, planned case mappings and explicit remaining work; 422 remain untriaged. No N/A waiver or verified control was added. See [OWASP_TRIAGE](OWASP_TRIAGE.md). Two additional P0 planned cases cover actual CORS and WebSocket behavior (66 total browser-specific cases).

Twelve gate self-tests passed. Both evidence paths now reject empty/tampered logs and stale source/binary identities; duplicate or unknown case records block qualification. Local inventory diagnostic `build/owasp-triage/report.json` confirms 442 imported controls, 20 applicable, 422 untriaged, zero N/A and zero unknown coverage IDs. The overall result remains BLOCKED. Native code was unchanged and was not rebuilt for this metadata/gate iteration.

## CORS / ws engine fixtures — 2026-10-18

The Release test executable rebuilt successfully and `--security-test` exited 0 with both `CORS_BOUNDARY_TEST PASS` and `WEBSOCKET_BOUNDARY_TEST PASS`. Local artifacts: `build/network-boundaries/report.json`, `security.log`, tested source snapshots and patch. The report identifies the base commit, source-file hashes, executable hash and log hash. Diagnostics include engine versions and expected CORS rejection messages.

Verified subset: loopback distinct-port CORS reads, actual preflight allow/deny, cookie omission and wildcard credential rejection; actual ws handshake positive control and strict deny before handshake, including a new connection after strict policy restoration. Cases `CYB-ORIGIN-001` and `CYB-WS-001` remain **in_progress** with explicit remaining scenarios. No broad OWASP control or release-case PASS is asserted. No production browser code changed; only the affected security suite was rerun.

## Temporary HTTP live connections — 2026-10-18

Release `cybou_tests` rebuilt and the affected `--transport-test` exited 0. `HTTP_CONTEXT_CONNECTION_TEST PASS` confirms three long-lived same-origin HTTP streams opened and disconnected across reload and both UI revoke paths, plus zero ws handshakes reaching the fixture server. The temporary HTTP profile interceptor explicitly recorded the ws denial; profile-destruction, peer-tab and stale-token checks also passed.

Artifacts: `build/http-context-connections/report.json`, `transport.log`, source snapshots and tracked patch. The report records the tested binary/source-file/log hashes and base commit. Cases remain in_progress; no WSS, already-open WebSocket closure, public/private DNS or broad OWASP PASS is claimed. This iteration changes test fixtures only; production behavior was not changed.
