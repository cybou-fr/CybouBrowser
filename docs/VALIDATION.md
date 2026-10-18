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

## TLS / WSS engine boundary — 2026-10-18

Release `cybou_tests` rebuilt and `--security-test` exited 0. `TLS_BOUNDARY_TEST PASS` confirms self-signed HTTPS/WSS rejection; a real WSS message in an exact-pinned isolated test-only positive profile; no allowance leaking to another profile; and server-observed active WSS disconnect after destroying the owning QWebEnginePage. The existing CORS/ws and native security assertions also passed in that run.

Artifacts: `build/tls-boundaries/report.json`, `security.log`, tested source/PEM snapshots and tracked patch. Expected Chromium certificate-authority handshake errors in the log are negative-test observations. No system trust store or production certificate policy changed. Cases remain in_progress and controls remain not_verified: the full TLS matrix, production temporary-profile/UI revoke and public/private DNS boundaries are still open. Only the affected security suite was rerun.

## Precise TLS certificate matrix — 2026-10-18

The final Release security run exited 0. All four cases passed: valid private-CA HTTPS/WSS; expired leaf (-201); hostname mismatch (-200); and no test-CA trust in a fresh profile (-202). Invalid WSS cases reached no HTTP upgrade handler. The positive uses native certificate validation with profile-local CA trust and no certificate-error acceptance callback. Existing CORS/ws, untrusted TLS/WSS, live page teardown and security regressions also passed.

The first matrix attempt exposed a test-server teardown crash after the positive case. Socket callbacks are now disconnected before server member destruction, and profiles are off-the-record. The failing log is retained as `build/tls-certificate-matrix/security-before-cleanup-fix.log`; final evidence is `report.json`, `security.log`, source/fixture snapshots and patch in that directory. The report fingerprints current source files, binary, log and fixture bytes. No production policy changed, and cases/controls remain in_progress/not_verified for the broader open requirements.

This matrix needs Qt 6.10+ profile-local trust; the source retains the Qt 6.9 compilation baseline but reports the unavailable verification as BLOCKED there. No OS trust store was modified.

## WSS revoke through temporary-profile UI — 2026-10-18

The Windows Release browser and test executable both rebuilt. A focused transport run passed, followed by **6/6 PASS** in a full baseline (security, controlbar, transport, tracker, surface, smoke). The transport log includes `TRANSPORT_WSS_TEST PASS`: three real WSS handshakes/messages followed by server-observed closure across reload, popover revoke and inline revoke. The same three HTTP streams close; all ws attempts remain denied. Retired profile pointers clear and the unrelated tab remains intact.

`BrowserStore` now records a native owner for temporary profiles; the default owner is the profile itself. Its optional constructor-only C++ factory lets the test use a public Qt prototype with profile-local CA trust. Production construction does not provide the factory and has no added trust input. The actual BrowserTab, lease/interceptor, tab replacement and UI revoke functions run unchanged in this test.

Artifacts: `build/transport-wss-ui/baseline.json`, `security-logs/`, focused log, `run-summary.json`, source snapshots and `tested-working-tree.patch`. The patch SHA was checked against the baseline source-diff SHA and both tested source hashes were rechecked. The supplementary summary fingerprints `cybou.exe`; the baseline fingerprints `cybou_tests.exe` and each nonempty log. This is development working-tree evidence; the following documentation/coverage edits were made after the run, without further native code changes. No broad release case/control was promoted to verified.

Remaining: hostile close peers and renderer/download stress, CRL/OCSP and protocol/cipher matrix, redirect/downgrade and public/private DNS boundaries, plus the other unchanged release gates.

## Adversarial transport — 2026-10-18

The Release test target rebuilt and the final affected suites both exited 0: security and transport. The security log records seven `HTTP_REDIRECT_CASE PASS` markers (two positive endpoints, five blocked 302 variants). The transport log records `TRANSPORT_WSS_UNCOOPERATIVE_TEST PASS`, including three observed close frames, zero server replies and all three WSS sockets disconnected before completion. Existing TLS/CORS/permission/security and UI/profile/HTTP-stream checks passed in those runs.

Artifacts: `build/adversarial-transport/report.json`, both logs, source snapshots including the new redirect header and tracked patch. The report fingerprints the one final test executable, each log and all changed test source files. Earlier redirect attempts exposed fixture setup errors (favicon counted as landing; strict mode not explicitly enabled); their logs are preserved under `build/http-redirect-boundaries/` and are superseded by the final strict-mode run. Production source behavior was unchanged in this iteration, so only the two affected suites were rerun; the preceding 6/6 baseline remains evidence for its recorded candidate, not this test binary.

Broader cases remain in_progress and controls not_verified. Remaining: suspended renderer/download stress, other redirect status/methods, HTTPS downgrade, DNS/CNAME boundaries and TLS revocation/protocol/cipher qualification.

## Redirect status/method and downgrade matrix — 2026-10-18

The Release test executable rebuilt and the affected security suite exited 0. The log contains 54 HTTP GET/POST case PASS markers across 301/302/303/307/308 (four direct positive controls and 50 denied inherited-consent redirects) and ten HTTPS-to-HTTP GET/POST case PASS markers. Native deny observations and server counters confirm no redirected HTTP landing request. The initial HTTP POST method/body and positive endpoint health are explicitly checked. Existing CORS, ws/WSS, certificate matrix and native security assertions also passed.

Artifacts: `build/redirect-method-matrix/report.json`, `security.log`, source snapshots and tested patch. Report fields identify base commit, dirty source, binary hash, log hash and changed test-file hashes. This iteration changes test infrastructure only; only the affected security suite was rerun. Cases remain in_progress and broader controls not_verified. Multi-hop/UI redirect presentation, DNS/CNAME and public/private transitions, OCSP/CRL and protocol/cipher qualification remain open.

## UI redirect chain and page-error regression — 2026-10-18

The Release browser and test executable rebuilt. The actual UI three-hop 302→307 cross-origin chain passed, including separate button consent per URL, rejected stale tokens, exact server hit counts, six destroyed temporary profiles and an intact unrelated tab. HTTP streams and hostile WSS peers still disconnect on reload and both revoke paths.

An initial full run failed the native negative redirect matrix while waiting for a renderer loadFinished signal, despite the request being denied. The negative assertion now accepts a synchronous MainFrame deny with the expected reason and short settlement, while still requiring target counters and denial metadata; direct positive controls require successful renderer completion. A focused security rerun and the subsequent full-run security suite passed all 54 HTTP and ten downgrade cases.

The next attempt exposed a QML name collision: the BrowserTab errorNotice signal shadowed the popup id. The popup now has a distinct name, and a native regression emits the actual tab signal and checks popup visibility. Expected download/cancelled navigations do not open page-failure notices. Controlbar and transport both passed on the corrected binary. The interrupted attempt's logs are preserved in build/ui-redirect-chain/final/security-logs; the earlier failed report is baseline-before-redirect-signal-fix.json.

Final candidate evidence is under build/ui-redirect-chain/final-ui-fix/: baseline.json, security-logs, tested-working-tree.patch and source snapshots/hashes. This remains development working-tree evidence; this validation entry is written after the tested patch was captured. No broad case or control is promoted to verified. Runtime DNS/CNAME enforcement remains open following the source review recorded in THREAT_MODEL.

Final full baseline: **6/6 PASS** (security, controlbar, transport, tracker, surface, smoke). The tested patch hash matches baseline sourceDiffSha256, and all five tested source-file hashes were rechecked unchanged. run-summary.json fingerprints the browser and test binaries plus the baseline report. The overall release gate remains BLOCKED by outstanding coverage requirements.

## Resolver-alias private-network probe — 2026-10-18

The opt-in `cybou_tests.exe --private-network-probe` diagnostic was added and run on the Release test binary (commit `89a3490`). Result: **exit 3 (PRIVATE_NETWORK_ALIAS_GAP)**, as designed — not a PASS.

- `PRIVATE_NETWORK_LITERAL_TEST PASS`: direct literal/alias/source navigations reached the fixture; four page `fetch()` calls (127.0.0.1, localhost, decimal and hex loopback) were denied with native `private-network-access` metadata and zero server hits.
- `PRIVATE_NETWORK_ALIAS_GAP`: a URL-public hostname (`cybou-alias.test`) resolved to loopback by test-process resolver rules reached the endpoint. The app classifies URLs only; no runtime DNS/connected-address enforcement is demonstrated.

The six-suite baseline was rerun afterwards: gate **PASS, 0 issues** (`build/probe-baseline/`, local, ignored). The probe is deliberately outside the baseline. The source connection is loopback, so this does not model Chromium's public address space, real DNS changes, CNAME chains, HTTPS or WebRTC. CYB-PNA-001/002 and the DNS-rebinding gate stay open.

## Engine-level private-network experiment — 2026-10-18

New opt-in `cybou_tests.exe --engine-pna-probe` (no app interceptor; engine decision only). Resolver rules map `cybou-source.test`/`cybou-alias.test` to loopback and `--ip-address-space-overrides=127.0.0.1:38471=public` makes the source server a public-space origin; the target (port 38472) stays local. Extra Chromium flags are supplied via `QTWEBENGINE_CHROMIUM_FLAGS`. Qt 6.12 / Chromium 140.0.7339.225, Windows, development tree.

| Chromium flags | literal 127.0.0.1 | alias hostname→loopback | target hits |
| --- | --- | --- | --- |
| none | reached | reached | 2 |
| `LocalNetworkAccessChecks` | reached | reached | 2 |
| `PrivateNetworkAccessSendPreflights,…RespectPreflightResults` (± `PermissionPrompt`) | reached | reached | 2 |
| `BlockInsecurePrivateNetworkRequests` | **blocked** | **blocked** | 0 |

Finding: the engine, not URL classification, can block a hostname that resolves to a private address when the **insecure-context** source is in public space (`BlockInsecurePrivateNetworkRequests`; zero target hits, no preflight sent). This is a connected-address boundary, closing the alias gap for HTTP sources only. Not proven: HTTPS public sources (preflight features did not block in this build), real public IPs/DNS changes/CNAME chains, WebRTC, workers. Production does **not** yet set this flag; enabling it is a compatibility decision (HTTP pages calling local services). CYB-PNA-001/002 remain open.

## Engine PNA hardening enabled — 2026-10-18

`applyEngineHardening()` (`src/security/EngineHardening.h`) now adds `--enable-features=BlockInsecurePrivateNetworkRequests` before engine initialisation in both `cybou.exe` and `cybou_tests.exe`; only the raw-engine probes opt out. New seventh gate suite `--pna-test` (`PnaCompatTests.h`) uses engine decisions only, with fixed-port `--ip-address-space-overrides`: public->local (literal and hostname alias) is blocked with zero target hits; public->public, local->local (literal and alias), local->public and direct navigation to a local address all still work. Gate: **7/7 PASS, 0 issues** (`build/pna-hardening/`, local). Scope: insecure-context sources only; HTTPS public sources, real DNS/CNAME changes, WebRTC and workers remain unproven. CYB-PNA-001/002 stay open.

## HTTPS public-source PNA and preflight enforcement — 2026-10-18

New diagnostic `cybou_tests.exe --https-pna-probe` (`HttpsPnaProbe.h`, raw engine, no app interceptor): HTTPS source `cybou-source.test:38471` (profile-local test CA, new public leaf `tests/security/fixtures/tls/pna-cert.pem` with SAN for the two test hostnames, issued by `generate_tls_matrix.py --pna-leaf-only`; existing fixtures untouched) overridden to the public address space; local HTTPS literal, HTTPS hostname alias and HTTP literal targets, each fetched in no-cors and cors mode. Qt 6.12 / Chromium 140.0.7339.225.

| Chromium features | Target grants private network | Result |
| --- | --- | --- |
| none, `BlockInsecurePrivateNetworkRequests`, or `LocalNetworkAccessChecks` | n/a | all six fetches reached the targets (no block for secure public sources) |
| `PrivateNetworkAccessSendPreflights` + `PrivateNetworkAccessRespectPreflightResults` | yes | preflight sent (also for the hostname alias), all reached |
| same | no | all six blocked; only OPTIONS reached targets, zero GET |

Finding: secure public pages were unrestricted until preflights were enabled; with them the engine enforces on the connected address, including hostname aliases. `applyEngineHardening()` now enables all three features. The `pna` suite grew to ten fetch cases plus a direct-navigation control, adding HTTPS public->local with preflight allowed (reaches target), denied (zero GET), alias denied, and HTTPS local->local (no preflight). Gate: **7/7 PASS, 0 issues** (`build/pna-https/`, local). Compatibility consequence: a public HTTPS site can use a local service only if that service answers the preflight with `Access-Control-Allow-Private-Network: true`. Not proven: real public IPs, DNS/CNAME changes, WebRTC, workers, WebSocket, redirects between address spaces. CYB-PNA-001/002 remain open.
