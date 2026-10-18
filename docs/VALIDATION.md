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

## PNA redirects and WebSocket — 2026-10-18

The `pna` suite (engine decision only, hardened flags) gained redirect and WebSocket cases. Results on Qt 6.12 / Chromium 140.0.7339.225:

| Case | Result |
| --- | --- |
| public -> public target that 302-redirects to local (literal and hostname alias) | blocked, zero hits on the local target |
| local -> public target that 302-redirects to local | allowed (the client address space is local) |
| public page opens `ws://` to local literal / hostname alias | **reaches the target (1 handshake request each)** |
| local page opens `ws://` to local | reaches the target (expected control) |

**Known gap, not a PASS:** Chromium's private-network protections do not cover WebSocket connections, so a public page can open a WebSocket to a local service through the engine alone. The suite logs `PNA_COMPAT_KNOWN_GAP` for the two public-source cases and still exits 0 so the baseline keeps running; the final line reports the gap count. Whether the application's URL interceptor stops the literal `ws://127.0.0.1` form is not covered by a test yet, and the hostname alias cannot be stopped by URL classification at all. A fix needs an enforceable connected-address check for WebSockets, or an explicit policy that blocks public-page WebSockets to resolved private addresses. CYB-PNA-001/002 and CYB-WS-001 remain open. Not covered: WebRTC, workers, real public IPs, DNS/CNAME changes.

**Flaky result recorded for this change set:** the first full gate run after adding the redirect/WebSocket cases reported BLOCKED because the `transport` suite failed at `TRANSPORT_PHASE 1` (`CYBOU_SMOKE_TEST FAIL`, exit 2; the other six suites including `pna` passed). Three isolated `--transport-test` reruns passed (exit 0), and a second full gate run passed 7/7 with 0 issues (`build/pna-ws2`). The change set only touched the `pna` suite, so a causal link is not evident, but the root cause of the one UI-smoke failure is **not diagnosed**; treat the transport phase-1 smoke as intermittently unstable until reproduced and explained.

### Transport phase-1 investigation — 2026-10-18

The failure was a timeout, not an assertion: no `TRANSPORT_TEST FAIL` line, polling exhausted (700 x 300 ms) at `TRANSPORT_PHASE 1`. Phase 1 sends a single synthetic mouse move onto the `network` control and waits for its hover card (opened by a 170 ms timer on a hover change); unlike the control-bar harness it had no wait-and-retry, so one lost or ineffective move hangs the run. This mechanism is a **hypothesis**: the failure did not reproduce in 10 sequential isolated runs (18-20 s each), 4 runs under saturated CPU, or 5 further runs after the change, and no log of the failing run was retained beyond its tail. Mitigation (hardening, not a proven fix): `TransportHarness.qml` now re-enters the icon (alternating a move away and back) once two seconds pass without the card on phases 1 and 6, and logs `TRANSPORT_HOVER_RETRY`. If that marker ever appears in a log the hypothesis is confirmed; if the phase-1 timeout recurs without it, the cause is elsewhere. Full gate after the change: PASS, 0 issues (`build/gate-after-harness-1`, local).

## WebSocket public->local: application layer — 2026-10-18

With `RequestObserver` installed (the product interceptor) on a public-source page, `ws://127.0.0.1` was already blocked before reaching the target by URL classification; a hostname resolving to loopback was **not** (target reached). The interceptor now resolves WebSocket target hostnames for public first parties (`RequestObserver::webSocketResolvesPrivate`, default `QHostInfo`, injectable via `setHostResolver`) and blocks with reason `private-network-access` when any answer is a private/loopback/link-local address. The `pna` suite covers literal blocked, alias blocked (stub resolver returning loopback, because the engine's test `--host-resolver-rules` mapping is invisible to the OS resolver) and a public-resolving name allowed.

Limits, stated plainly: the lookup is not bound to the connection Chromium later makes, so a changing DNS answer (rebinding) or a resolver that differs from Chromium's (DoH, hosts, proxy) can bypass it; a failed lookup fails open to the engine; the lookup is blocking on the interception thread and only runs for WebSockets from public pages. The two engine-level cases (`public ws->local` literal and alias with no app interceptor) still log `PNA_COMPAT_KNOWN_GAP`. This is a mitigation, not enforcement at the connected address; CYB-WS-001, CYB-PNA-001/002 stay open. No real-DNS test exists yet.

**Gate not green for this change set (environmental):** the full gate for the WebSocket resolver change was BLOCKED (`build/gate-ws-resolver`): security, surface and pna PASS; controlbar, transport, tracker and smoke FAILED at the shared UI smoke with `Unable to move the cache: Acces refuse` in every log. A leftover `cybou_tests.exe --controlbar-test` (PID 12876, started when an earlier gate run was aborted by an external timeout) could not be terminated and holds the default WebEngine cache directory. This is a test-environment fault, not evidence about the change, but those four suites have **not** been rerun green on it. Re-run the full gate after the stale process is gone (reboot or end it in Task Manager). Never wrap the gate in a short `timeout`: aborting it orphans UI test processes. The earlier unexplained transport phase-1 flake is not attributed to this cause.

## Dedicated worker coverage — 2026-10-18

Three `pna` cases run `fetch()` inside a dedicated Worker created from a blob URL: a public-space page's worker to a local literal and to a hostname alias is blocked with zero target hits (engine, hardened flags); a local page's worker to a local target works. Dedicated workers therefore inherit the creator's address space in Chromium 140. Not covered: shared/service workers, WebSocket inside a worker, WebRTC, HTTPS workers, real DNS. Only `--pna-test` was run for this addition (PASS, exit 0); the full gate is still blocked by the stale-process cache lock recorded above and was not rerun.

## WebRTC ICE probe — 2026-10-18

New opt-in diagnostic `cybou_tests.exe --webrtc-probe` (`WebRtcProbe.h`; raw engine, off-the-record profile, no app interceptor, no STUN/TURN servers; Qt 6.12 / Chromium 140.0.7339.225). It runs with `QWebEngineSettings::WebRTCPublicInterfacesOnly` off, then on.

| Observation | setting off | setting on |
| --- | --- | --- |
| Local ICE candidates visible to the page | two `*.local` mDNS host candidates, no raw LAN or loopback IP | none |
| STUN connectivity checks received on a UDP socket at `127.0.0.1` after the page injected a remote loopback candidate | **87** | **91** |

Findings: (1) the page does not learn private IPs without a media permission (mDNS masking); with `PublicInterfacesOnly` it gets no candidates at all. Behaviour with a granted camera/microphone permission is untested and is when Chromium exposes real addresses. (2) **Open gap:** a page can make the browser send UDP STUN binding requests to an arbitrary loopback address by injecting a remote ICE candidate, and `WebRTCPublicInterfacesOnly` does not stop it - that setting limits exposed local candidates, not connection targets. The traffic is UDP, so neither Chromium's private-network protections nor the URL interceptor see it. Impact is limited to STUN-format datagrams (a fixed small request, rate of roughly 17 per second here), a local-network probing and reflection primitive rather than arbitrary UDP; reading responses is limited to ICE state. Not tested: private-range LAN targets, TCP candidates, TURN-relayed paths, an HTTPS public page, any Chromium flag or policy that filters remote candidates. The probe exits 0 whenever the experiment ran; it is not a PASS. CYB-PNA-001/002 and ASVS 17.1.1 stay open.

### WebRTC mitigation search - 2026-10-18

Candidates tried with `--webrtc-probe` (injected remote loopback candidate; STUN checks counted at a test-owned UDP socket, setting `WebRTCPublicInterfacesOnly` off/on):

| Chromium switch | STUN checks at loopback (off / on) | Two-peer data channel without TURN (off / on) |
| --- | --- | --- |
| none | 77-85 / 85-87 | opens / **does not open** |
| `--force-webrtc-ip-handling-policy=default_public_interface_only` | 85 / 81 | not measured |
| `--enable-features=LocalNetworkAccessChecks,LocalNetworkAccessChecksWebRTC` | 84 / 82 | not measured |
| `LocalNetworkAccessChecks` + the two PNA preflight features | 85 / 86 | not measured |
| `--force-webrtc-ip-handling-policy=disable_non_proxied_udp` | **0 / 0** | does not open / does not open |

Findings: only `disable_non_proxied_udp` stops the probing, by forbidding direct UDP altogether. A WebRTC-specific Local Network Access check exists upstream only from Chrome 142 (public Chrome documentation and a WebRTC commit "LNA: Request LNA permission for local/loopback ICE candidates"; tracking crbug 421223919). The embedded engine is Chromium 140.0.7339.225 (security patch level 152), where the guessed feature name has no effect, so the real fix is a Qt WebEngine with that check plus an application handler for the permission. Side observation: with the application's current `WebRTCPublicInterfacesOnly` setting two peers on the same machine cannot connect without STUN/TURN, so serverless local P2P is already unavailable. The real cost of `disable_non_proxied_udp` for calls through public relays is **not measured**: it needs a TURN/STUN-capable endpoint or real provider accounts; calls would have to fall back to TCP/TLS relays. Therefore the flag is NOT enabled in production; the choice (default on, opt-in setting, or wait for an engine with LNA for WebRTC) is a product decision. Gap remains open: CYB-PNA-001/002, ASVS 17.1.1.

## Opt-in strict WebRTC - 2026-10-18

Settings > Security now has **WebRTC strict - no direct UDP** (`BrowserStore::strictWebRtc`, saved as `preferences/strictWebRtc`, **default off**). When enabled, the next start passes `--force-webrtc-ip-handling-policy=disable_non_proxied_udp` to the engine; the switch is process-wide and read before the engine starts, so the panel tells the user to restart while the saved preference differs from `strictWebRtcActive`. An explicitly supplied `--force-webrtc-ip-handling-policy` in `QTWEBENGINE_CHROMIUM_FLAGS` always wins over the preference.

New gate suite `webrtc` (`cybou_tests.exe --webrtc-test`, `StrictWebRtcTests.h`): (1) pure flag-composition checks: off by default, exactly one switch when enabled, idempotent, preserves and merges existing `--enable-features`, respects an explicit policy; (2) two child processes of the same binary, strict off (control) and on, each injecting a loopback ICE candidate and counting STUN checks on a test-owned UDP socket. Result on Qt 6.12 / Chromium 140.0.7339.225: control **91** checks, strict **0** (suite PASS, exit 0). The parent requires the control to reach loopback (>= 10) so a silent probe cannot pass vacuously. Also rerun and passing: `--security-test`, `--surface-test` (renders the settings panel at 1360 and 820), `--pna-test`; gate self-tests 12/12 with the suite list now eight.

Not proven and not claimed: usability of real calls with the option on (no TURN/STUN endpoint or provider accounts available; calls need TCP/TLS relays and may degrade or fail), the French/Russian wording review, restart-flow behaviour in a packaged build, behaviour with a user-configured proxy, and the full eight-suite gate (the four UI suites are still blocked by the stale-process cache lock recorded earlier). With the option off, the exposure documented in "WebRTC ICE probe" remains. CYB-PNA-001/002 and ASVS 17.1.1 stay open.

## Untrusted text in native UI - 2026-10-18

**Defect found and fixed.** The browser's QML UI shows attacker-controlled strings (page titles in the tab strip, history titles and URLs, download file names, permission origins). `Label`/`Text` default to `Text.AutoText`, which renders HTML-looking strings as rich text, and rich text loads `<img src>` through the application's own network stack - outside the browser profile, the request interceptor, strict HTTPS, private-network protection, tracker blocking and the private-profile boundary. `--text-injection-probe` (`TextInjectionProbe.h`) measured it on Qt 6.12: a title containing `<img src="http://127.0.0.1:PORT/...">` caused **1** request with `AutoText` and with `StyledText`, **0** with `PlainText`. A hostile page could therefore make the UI contact an arbitrary host (including loopback and private addresses) and reveal the user's real IP, timing and tab titles' presence.

**Fix.** `textFormat: Text.PlainText` on all 48 dynamic `Label`/`Text` items (no rich text was intentionally used anywhere). **Guard:** `tools/security/qml_text_lint.py` (standard library; run by `python -m unittest tools/security/test_qml_text_lint.py`, 5 tests) fails when any non-literal text binding lacks the opt-out; it passes on the current tree. After the change `--surface-test` and `--security-test` still pass; gate self-tests 12/12 plus 5 lint tests. New case `CYB-UI-004` (in progress) and applicable control ASVS 3.2.2.

**Not proven:** a live end-to-end run with a hostile page title in the tab strip and library (UI suites are blocked by the stale-process cache lock), text rendered outside QML (C++ dialogs, system notifications, accessibility names), other Qt versions' defaults, whether the fetch was exploitable in the shipped packaged build. No exposure period or exploitation evidence is claimed.

## HSTS enforcement - 2026-10-18

New gate suite `hsts` (`cybou_tests.exe --hsts-test`, `HstsTests.h`; Qt 6.12 / Chromium 140.0.7339.225, engine decision only, hardened flags). A TLS-only loopback fixture (profile-local test CA, leaf for `cybou-source.test`) is reached through test-process resolver rules. Results, exit 0:

| Scenario | Result |
| --- | --- |
| HTTPS visit with `Strict-Transport-Security: max-age=3600`, then `http://` to the same host and port in the same profile | upgraded to `https://`, request reached the TLS server once |
| Control: fresh off-the-record profile, plain `http://` to the TLS-only port | load failed, zero server requests |
| Control: HTTPS visit without the header, then `http://` | not upgraded, zero server requests |

So the upgrade comes from the header and is scoped to the profile. Not covered: HSTS persistence across restarts for persistent profiles, `includeSubDomains`, the preload list, interaction with the application's single-navigation HTTP exception (the engine would upgrade before the application sees an http:// request for an HSTS host), cleared-site-data behaviour, and ports other than the fixture's. ASVS 3.4.1 stays `not_verified`; this is one engine-level fixture. Gate self-tests 12/12 with nine suites listed; only `--hsts-test`, `--pna-test` were run for this addition, the full gate is still blocked by the stale-process cache lock.

## Download name handling and Mark-of-the-Web - 2026-10-18

New gate suite `download` (`cybou_tests.exe --download-test`; `--download-probe` prints observations only; `DownloadFileTests.h`). A loopback HTTP fixture answers eight hostile `Content-Disposition` headers; the engine downloads into a per-case directory with no application handler. Qt 6.12 / Chromium 140.0.7339.225, Windows 11.

| Case | Observed |
| --- | --- |
| plain, `..\..\x`, `../../x`, RFC 6266 `filename*=..%2F..%2Fx`, absolute `C:\...` path | saved inside the directory under the basename; Mark-of-the-Web (`Zone.Identifier`) present |
| `CON.txt` | saved as `CON.txt` with Mark-of-the-Web |
| `stream.txt:hidden` | download interrupted, no file written |
| `trailing. ` (dot and space) | **anomaly:** reported name `trailing. ` while the directory holds two files (`trailing`, `trailing. `); Mark-of-the-Web absent on the first |

**Defect found and fixed.** `DownloadPolicy::allowsExternalOpen` allowed `CON.txt` because `txt` is allowlisted, although Windows reserves device names with any extension. It now rejects CON, PRN, AUX, NUL, CONIN$, CONOUT$ and COM/LPT 1-9 (including superscript digits) with any extension; `--security-test` covers 11 denied names and 6 near-miss names that must still open (`console.txt`, `com10.txt`, `lpt0.txt`, ...). The engine itself still saves such names. Suite result: PASS, exit 0 (security suite also PASS); the trailing-dot case is logged as `DOWNLOAD_KNOWN_ANOMALY`, not asserted. Gate self-tests 12/12 with ten suites listed.

Not covered: the application's own accept/rename path (QML handler), name collisions, very long and Unicode/bidi names, `Zone.Identifier` content, PDF export, non-NTFS targets, the full gate (UI suites still blocked by the stale-process cache lock). Wallpaper intake checks the extension only; its size and pixel limits are untested.

## Data inventory draft - 2026-10-18

`docs/DATA_CLASSIFICATION.md` (draft, unapproved) inventories what CYBOU stores, where, and with which proposed protection level. Source- and filesystem-based, no new automated test. Findings that need owner decisions: history, session and bookmarks keep **complete URLs including query and fragment in plaintext** in the user registry (history capped at 1000, absent in private mode); the engine profile holds its own `History` and `Favicons` databases whose clearing is untested; the registry key `HKCU\Software\CYBOU\CYBOU` also held values that do not belong to the browser (origin not verified, possibly another application with the same organization and application name); no at-rest protection and no retention schedule beyond the history cap. None of this is verified or fixed; ASVS 14.1.1/14.1.2 remain `not_verified` pending reviewer approval and a real-session check.

## Secrets in stored URLs - 2026-10-18

Owner decision (recorded): clean history and bookmarks, leave the open-tab session untouched. `src/security/StoredUrl.h` now strips the fragment and embedded credentials and replaces the values of secret-bearing query parameters with `redacted` before `BrowserStore::recordVisit` and `toggleBookmark` persist anything; titles that equal the URL are cleaned the same way; bookmark lookup compares cleaned URLs; a one-time migration (`privacy/storedUrlsSanitized1`) cleans entries written by earlier versions. URLs without anything to remove are stored exactly as before. Parameter detection is by name (exact words such as `code`, `key`, `sig`, `sid`, `jwt`; substrings `token`, `password`, `secret`, `signature`, `credential`; ignoring case, `-` and `_`) - a heuristic, not a guarantee.

Evidence: `--security-test` PASS (exit 0) including `STORED_URL_TEST`: eight URL cases with exact expected output (fragment, credentials, `token`, `Access-Token`, `code`, `X-Amz-*`, `api_key`, and look-alike names `zipcode`/`encoding`/`monkey` left intact), a `BrowserStore` history round-trip with a title equal to the URL, bookmark add/lookup/remove by the raw URL, and migration of a legacy history and bookmark list. `--surface-test` PASS afterwards.

Not covered: the session store still keeps full URLs in plaintext (by decision, needed to restore tabs); the engine's own `History` database in the profile is unchanged; page titles and tokens with unrecognised parameter names; bookmarks edited elsewhere; behaviour of history suggestions for redacted URLs in a live UI (a redacted link does not reopen the original page - accepted trade-off). ASVS 14.2.1 and 14.3.3 stay `not_verified`: a secret-canary test across session, engine profile and diagnostics is still missing.

## Engine profile and visited URLs - 2026-10-18 (correction)

New diagnostic `cybou_tests.exe --engine-history-probe` (`EngineHistoryProbe.h`; Qt 6.12 / Chromium 140.0.7339.225): three visits to marker URLs in a persistent temporary profile, a 15 s wait for database commits, profile destroyed, then every file in the profile scanned for the marker bytes. Result: the visited URL appeared **only in the HTTP cache** (`Cache/Cache_Data/data_1`), in none of the other files - not in `Cookies`, not in `Favicons`, and no `History` file listing contained it. `QWebEngineProfile::clearAllVisitedLinks()` did not change that; `clearHttpCache()` removed it (0 files contain the marker).

**Correction to the data-inventory draft:** the claim that the engine's `History`/`Favicons` files are a second copy of browsing history was an inference from file names and is not supported by this measurement; it is replaced by "the HTTP cache keeps visited URLs". Consequences: (1) clearing history alone leaves visited URLs recoverable from the cache files; (2) the clear-data dialog already ticks both "history" and "page cache" by default, so the normal path is covered, but unticking the cache leaves them; (3) the Qt Quick profile used by the application has no `clearAllVisitedLinks()` (only `QWebEngineProfile` has it), so the engine's visited-link state cannot be cleared through the supported Quick API. The application still calls only `BrowserStore::clearHistory()` for the history option. Not tested: a real `History` database in the production profile after long use (the probe profile is fresh), the application's own clear flow end to end, favicon database content with a real icon.

## External protocol launches - 2026-10-18

New gate suite `protocol` (`cybou_tests.exe --protocol-test`, `ExternalProtocolTests.h`; Qt 6.12 / Chromium 140.0.7339.225). Two fabricated schemes (`cybou-ext-a`, `cybou-ext-b`) have `QDesktopServices` handlers that only count calls, so no real application can start even if a policy fails. A loopback page fires seven script triggers; the suite counts handler calls per trigger under two page policies.

| Policy | Triggers that reached the external handler |
| --- | --- |
| `AllowAllUnknownUrlSchemes` (control) | `location.href`, `location.assign`, `iframe.src`, `anchor.click`, `form.submit`, `meta-refresh` (`window.open` produced none, likely the popup blocker) |
| `DisallowUnknownUrlSchemes` (the product's setting in `BrowserTab.qml`) | none |

The control proves a launch attempt is observable, so the zero is meaningful. `tools/security/test_qml_policies.py` (3 tests) additionally checks that every QML file declaring a `WebEngineView` sets `DisallowUnknownUrlSchemes`, that nothing loosens it, and that every `QDesktopServices::openUrl` call site in application code passes a local file. Both pass; gate self-tests 12/12 with eleven suites listed.

Consequence stated plainly: with the strict policy a legitimate `mailto:` or `tel:` link does nothing and the user gets no explanation or choice - there is no confirmation dialog. Not covered: user-gesture and address-bar navigations, real registered OS schemes (deliberately avoided), drag and drop, `file:`/`view-source:`/internal `chrome:` pages, schemes already known to Chromium. ASVS 3.7.3 is `applicable`, `not_verified`.

## Idle outbound connections - 2026-10-18

`tools/security/idle_connections.ps1` launches `cybou.exe --private` (off-the-record profile, no session restore, so the user's persistent profile and open tabs are not involved), samples every TCP connection and UDP endpoint owned by the process tree, and stops the tree afterwards. Windows 11, development build, no capture driver.

| Run | Result |
| --- | --- |
| Positive control: same launch opening `https://example.org/`, 25 s | one established TCP connection to a Cloudflare IPv6 address on port 443, owned by `cybou` (the network service runs in the main process), seen in 9 samples - the sampler detects traffic |
| Idle, new-tab page only, 120 s, 122 samples | **no TCP connection of any kind** from `cybou` or `QtWebEngineProcess`; no UDP endpoints; no new DNS-cache entries; no surviving processes |

Source reading agrees: address-bar suggestions come from local history and bookmarks; search providers (DuckDuckGo, Brave, Bing, Google) and cloud launch targets are contacted only when the user navigates there.

Limits: private window and new-tab page only (a persistent-profile start with session restore, cloud tiles, an open settings panel and the engine's behaviour over longer periods were not sampled); about one sample per second, so a connection shorter than that can be missed; UDP shows local endpoints only (and none appeared even in the control); the DNS-cache signal did not change in the control either, so it is not a reliable indicator. This is evidence for ASVS 16.2.3 / 13.1.1 / 13.2.4 at idle, not a proof of no telemetry; those controls stay `not_verified`.

## URL parser differential and script construction - 2026-10-18

New gate suite `url-parser` (`cybou_tests.exe --url-parser-test`; `--url-parser-probe` prints the table; `UrlParserDifferentialTests.h`). For 17 spellings of the loopback address on a loopback fixture it records whether the engine actually connects and whether `NavigationGuard::isPrivateNetwork` (built on `QUrl`) classifies the same URL private: canonical, `127.1`, hex and octal octets, decimal/octal/hex integers, zero-padded, IPv4-mapped IPv6, `localhost` and `localhost.`, ideographic dots, fullwidth digits, `0.0.0.0`, `0`, `127.0.0.1.` and `127.0.0.1..`. Qt 6.12 / Chromium 140.0.7339.225.

**Defect found and fixed:** `http://127.0.0.1.:PORT/` (IP literal with a trailing dot) reached loopback in the engine while `NavigationGuard` treated it as a public host, so the application-level private-network and HTTP-exception checks could be bypassed with it (the engine-level PNA flags still applied to subresources). `NavigationGuard::normalizedHost` now ignores trailing dots; unit cases cover eight trailing-dot spellings and confirm `example.com.` and `8.8.8.8.` stay public. After the fix every spelling that reaches loopback is classified private (suite PASS, exit 0); `0.0.0.0`/`0` are not reachable in this engine, and `127.0.0.1..` has an empty `QUrl` host and is not reached. `--security-test` PASS including the new cases.

**Hardening:** `stopMediaTracks` now embeds its argument with `JSON.stringify`; `tools/security/test_qml_policies.py` (now 4 tests) fails on any QML line that concatenates a value into a quoted JS argument. Not covered: IPv6 zone ids, userinfo and backslash tricks, percent-encoded hosts, redirect-time re-parsing, and non-loopback private ranges in the differential (only loopback is reachable from the fixture). Gate self-tests 12/12 with twelve suites listed; the full gate is still blocked by the stale-process cache lock.

## Deterministic URL fuzzing - 2026-10-18

New gate suite `fuzz` (`cybou_tests.exe --fuzz-test`, `FuzzTests.h`, fixed seed). Part A feeds 20 000 generated URLs (schemes, hosts incl. mixed-radix IPv4 and bracketed IPv6, userinfo, ports, paths, queries with canary secrets, fragments, unicode and control characters) and 8 pathological inputs (50 000 dots, 200 000 digits, 200 000 `%`, 100 000 `../`, ...) through `StoredUrl::sanitize`, `NavigationGuard` (private-network, local host, IDN), `NetworkPolicy` and `DownloadPolicy`, asserting: no crash, each call under 100 ms (pathological under 2 s), sanitising idempotent, no canary survives, classification unchanged by a trailing dot or host case. Part B asks the engine to open 250 random spellings of the loopback address (1-4 parts, hex, octal, decimal, upper case, trailing dot) and requires that every spelling the engine reaches is classified private by `NavigationGuard`. Result: PASS, exit 0; slowest generated call 1 ms; 250 of 250 spellings reached the fixture and none was classified public.

**Defects found by the first runs and fixed in `StoredUrl::sanitize`:** (1) a URL that `QUrl` cannot parse was stored unchanged, keeping its query and fragment - it is now cut at the first `?` or `#`; (2) `;` was not treated as a parameter separator; (3) a secret nested in a parameter (`continue=/cb?token=X`, also percent-encoded) survived because only top-level names were checked - decoded parameters are now scanned piece by piece. `--security-test` gained four cases for these. The first run's other failures were defects of the test itself (appending a dot to an IPv6 host) and were corrected rather than ignored.

This is a floor: not coverage-guided, not run under AddressSanitizer or UBSan, only URL-shaped inputs, loopback-only differential. ASVS 1.4.1/1.4.2/1.4.3 stay `not_verified`; the sanitizer/fuzzer build ASVS expects is still to do. Gate self-tests 12/12 with thirteen suites listed; the full gate remains blocked by the stale-process cache lock.
