# CYBOU — test plan (OWASP-indexed and browser-specific)

**Current product `0.3.0-alpha.1`: NOT RELEASE QUALIFIED.** Native smoke suites exist, but full OWASP enumeration and actual independent security verification remain unfinished. This document defines **tests to implement and run**, not global PASS. Source of truth: [SECURITY_VERIFICATION_FRAMEWORK](SECURITY_VERIFICATION_FRAMEWORK.md), [RELEASE_CRITERIA](RELEASE_CRITERIA.md), [VALIDATION](VALIDATION.md).

## Local deterministic runner

Build Windows/MSVC x64 Qt 6.12 Release, with the tests executable (`cybou_tests`), isolated profile and synthetic local fixtures. No CI dependency:

```powershell
cmake --build build --config Release --parallel 4
python tools/security/test_owasp_gate.py
python tools/security/owasp_gate.py --mode baseline --exe build/Release/cybou_tests.exe --out build/security-baseline.json
```

The runner launches and records **six real suites**: `--security-test`, `--controlbar-test`, `--transport-test`, `--tracker-test`, `--surface-test`, `--smoke-test`. Nonzero result, missing binary and timeout fail the baseline. Logs, SHA and binary hash are preserved in JSON. One suite PASS does not mean all individual requirements PASS.

**Release coverage gate**, intentionally failing now:

```powershell
python tools/security/owasp_gate.py --mode release --exe build/Release/cybou_tests.exe --asvs-csv path/to/official-ASVS-5.0.0.csv --wstg-ids path/to/complete-WSTG-v42-IDs.txt --out build/security-release.json
```

Pin SHA-256 in `tests/security/standards.lock.json`, populate `coverage.json` for every imported ASVS/WSTG ID, implement and review `cases.json` cases, attach fresh verified `evidence.json`. Unmapped, unknown, unreviewed N/A or SKIP means **release BLOCKED**, not a synthetic success. Vendor cloud sites must not be scanned without permission; test on owned fixtures plus consent-based manual interoperability.

## OWASP applicability matrix (mandatory process)

- [ASVS 5.0.0](https://github.com/OWASP/ASVS): every numbered requirement mapped to `applicable` with case IDs / evidence or reviewer-approved `not_applicable` with concrete desktop-browser rationale.
- [WSTG v4.2](https://wstg.owasp.org/v4.2/): every official scenario ID treated identically. Backend SQL/database testing may be N/A for CYBOU's native app if not present, but must not be silently omitted; any local test service remains separately scoped.
- [OWASP Top 10:2025](https://top10.owasp.org/2025/): review all A01–A10 risk families; require specific negative tests and accepted mitigations.
- [SCVS 1.0](https://scvs.owasp.org/): SBOM, source/binary provenance, Qt/Chromium known-vulnerability handling, licenses, signing and update integrity.
- [SAMM v2](https://owasp.org/www-project-samm/): secure SDLC policy, design, implementation, verification and operations; a maturity review is not an exploit test.
- Additional browser-specific checks beyond OWASP web-app scope: renderer sandbox, browser privilege boundaries, media hardware, origin/frame policy, DNS/WebRTC, privacy/isolation, GUI accessibility, updates and OS integration.

## Required test campaigns

| Area | Positive + negative examples | Type |
| --- | --- | --- |
| Permissions/Control Bar | Top A / iframe B deny propagation, independent requester deny, stale IDs, epoch mismatch, queue cancellation/revokeAll, **actual hardware capture stop**, grant != actual recording | Native + WebEngine fixtures + hardware |
| HTTPS/transport | TLS invalid/expired cert, strict HTTPS, single-use exception, malicious redirect/subresources, ws/wss, mixed content, private mode, credentials-in-URL | Native + controlled network |
| Network/RuleEngine | Ads vs Trackers separation, integrity/rollback, same origin multiple tabs, exception owner destroyed, false positives login/payment/CDN/CAPTCHA | Native + stress/integration |
| SSRF/PNA/DNS | IPv4/v6 localhost & private ranges, exotic syntax, DNS rebinding, CNAME changes, redirects, dangerous ports | Owned DNS/HTTP fixtures + fuzz |
| Browser isolation | Private session cookie/cache/storage absence, renderer sandbox flags, process tree, download URI/file path abuse and system-exec boundaries | OS instrumentation + integration |
| Input/injection | URL, IDN, homoglyphs, scheme, Unicode, QSettings corruption, cloud custom endpoints, browser-owned JS actions, MIME/download content | Fuzz + property tests |
| Cloud apps | Chosen default plus overrides, no prefetch, no privileged injection/extension, correct verified service targets, genuine login/2FA/Docs/Meet compatibility | Unit + real test accounts (manual) |
| Privacy/logging | No passwords, OAuth tokens, cookies or document bodies in logs; no secret telemetry from idle NewTab | Automated scan + manual |
| System resilience | Dozens of tabs, network loss, time jumps, sleep/resume, OOM, disk full, crash/restart, long-call revoke, mouse/keyboard/accessibility | Soak + fault-injection |
| Release and supply chain | Known CVEs, no extension loaders, real sandbox, provenance/signatures, rollback/update integrity, independent pentest | Static/runtime/manual |

**UI invariant:** 11 always-visible icon slots in the original navigation row (820/1024/1360/1920), interactive glass hover, no reflow to extra row; FR/RU and 100/150/200% DPI. Cloud Glass opens from 3×3 on Alpine Dawn and cannot occlude browser-owned controls.

## Evidence and privacy

For each test: case ID, precise ASVS/WSTG ID where applicable, preconditions/fixture, expected and observed behavior, positive and negative assertions, exact code SHA, tested binary SHA-256, platform/Qt/Chromium/security patch, logs and failure reason. Keep test-only users and directories; redact secrets and never use customers' accounts. Use PASS/FAIL/BLOCKED/NOT_RUN/NOT_APPLICABLE with reviewed rationale. The complete release report must distinguish *coverage* from actual *passing verification*.

## Current verification status

The test suites and risk inventory exist, but **the current source/binary has not passed the complete OWASP and browser-specific release gate**. Run the executable suites, implement missing harnesses, attach candidate-specific logs, and obtain independent adversarial review. Do not record PASS without accessible test results. See [VALIDATION](VALIDATION.md).

## CORS and WebSocket local fixtures

`src/security/NetworkBoundaryTests.h` runs within `--security-test`. Two loopback TCP servers on distinct ports exercise actual WebEngine requests. Server-side request counts confirm that a permitted preflight reaches PUT and a denied preflight never does; JavaScript assertions check CORS response visibility and wildcard-plus-credentials rejection. Cookie observations distinguish `omit` from `include`. WebSocket checks include a real RFC 6455 HTTP 101 handshake, strict denial before reaching the server, and denial of a new connection after strict policy is restored. Matching interceptor deny events are required.

This fixture deliberately uses test-only HTTP allowance to establish the positive control. It does not prove production temporary-profile behavior, WSS certificate validation, public/private routing, cross-site cookies or revocation of an already-open socket. The two broad release cases remain in progress.

Build/run: `cmake --build build --config Release --target cybou_tests --parallel 4`, then `build/Release/cybou_tests.exe --security-test`. The fixture has a 20-second deadline and never contacts external services. Evidence from this development iteration is under `build/network-boundaries/`, including the log, report, tested source snapshots and tracked patch.

## Temporary HTTP profile connection teardown

`--transport-test` now serves a same-origin chunked HTTP response that deliberately never finishes. Each of the three consented temporary documents retains a stream reader and must receive its first chunk before the fixture becomes ready. A second probe attempts an actual ws handshake; the fixture server supports a positive HTTP 101 response if reached, while the test requires zero ws requests and a matching `insecure-transport` interceptor event.

After reload, popover revoke and inline revoke, the server must have observed three opened and three disconnected HTTP streams, with no active stream left. A surviving connection after ten final polling checks (approximately three seconds) fails the test. Existing assertions also require temporary profile destruction and preservation of the unrelated tab.

This tests real temporary-profile HTTP connection teardown and plain-ws denial, not active WebSocket/WSS closure. Temporary HTTP consent intentionally never grants ws. WSS certificate tests and active secure socket teardown remain open.

## TLS / WSS boundary fixture

`src/security/TlsBoundaryTests.h` runs inside `--security-test` with Qt's existing TLS backend and committed public test-only PEM fixtures under `tests/security/fixtures/tls/`. No system trust changes or global certificate bypass flags are used. Ordinary isolated WebEngine profiles must reject self-signed HTTPS and WSS before a WebSocket handshake reaches the server.

A separate positive-control page accepts only the exact fixture leaf fingerprint at the server's loopback address/port, loads HTTPS and receives an actual WSS message. The negative profile must still reject WSS after that exception. The test then destroys the owning positive page without calling WebSocket.close; the server must observe the live socket disconnect. The entire fixture has a 25-second deadline and fails without a working TLS backend.

This proves the engine/page lifecycle subset. The fixture is not a trusted public-CA handshake, expired/hostname/revocation matrix, protocol/cipher qualification or a production temporary-profile UI revoke test. The positive certificate handler exists only in the test header. Run tests from the repository root; OpenSSL is not a runtime dependency.

## Precise TLS validity / hostname matrix

The security suite now additionally calls `runTlsCertificateMatrixTests()`. Each case uses a fresh off-the-record WebEngine profile. A committed public test CA is trusted only in the applicable profile via Qt 6.10+ additional certificate trust. Unlike the earlier isolated leaf positive control, this matrix never accepts certificate errors.

The valid leaf must load HTTPS without any certificate-error callback and deliver an actual WSS message. Expired and wrong-SAN leaves must fail HTTPS with CertificateDateInvalid (-201) and CertificateCommonNameInvalid (-200), and reach neither the HTTPS handler nor the WSS upgrade endpoint. A final fresh profile without the test CA must reject the otherwise valid leaf with CertificateAuthorityInvalid (-202), proving prior profile-local trust was not inherited. Each case has a 15-second deadline; validity preconditions are checked against current UTC.

The test TLS server explicitly detaches socket callbacks before derived state is destroyed. This fixes the teardown crash exposed by moving between matrix cases. Public fixture regeneration is documented under `tests/security/fixtures/tls/README.md`; no OpenSSL runtime, system CA install or global TLS bypass is used. OCSP/CRL, protocol/cipher, public-CA interoperability and production UI revoke remain separate qualification work.

## WSS revoke through actual temporary-profile UI

`--transport-test` now starts the local TLS server and supplies a C++-only temporary profile factory backed by `WebEngineProfilePrototype`. The prototype owns the profile and loads only the public test CA. The test document opens an actual WSS socket and must receive the server's `live` message before becoming ready. It does not call WebSocket.close or install a certificate-error bypass.

Three successful WSS controls exercise reload, popover revoke and inline revoke. The server must observe all three sockets disconnect, alongside the three long-lived HTTP streams; otherwise the terminal polling deadline fails. Existing off-the-record, AskEveryTime, peer-tab, stale-token, ws-deny and retired-profile assertions remain in force. A full six-suite regression checks the default factory path as well as the test prototype path. This qualifies the local UI lifecycle subset, not uncooperative peers, renderer suspension, in-flight downloads, public TLS services or the whole release control.

## Uncooperative WSS peer and real redirects

The transport TLS fixture now deliberately ignores WebSocket close frames, sends no close reply and never initiates closure of an upgraded socket. Server counters require three established/message-delivering WSS controls, zero server close replies and three disconnected sockets after reload and both UI revoke paths. The passing run observed three incoming close frames. This prevents a cooperative fixture server from hiding a client teardown problem.

`src/security/HttpRedirectTests.h`, included in the security suite, runs two direct positive controls and five actual HTTP 302 negatives: same-origin path change, changed query, loop to the exact URL, cross-origin port change and embedded credentials. Each isolated WebEngine profile uses strict HTTPS plus one exact HTTP navigation lease. Server counters require one initial redirect request and no redirected landing/repeated-start request. Native deny metadata is required and target origins must exclude credentials/query values. Favicon requests are served separately and never counted as target navigation. Every case has a ten-second timeout.

This qualifies the native interceptor/lease subset for 302 GET navigation. Other status codes/methods, HTTPS downgrade, UI redirect presentation and DNS/CNAME transitions remain open.

## GET/POST and HTTPS downgrade redirect matrix

`runHttpRedirectTests()` now covers status codes 301, 302, 303, 307 and 308 with GET and POST: 50 denied path/query/loop/cross-origin/credential cases plus four live direct endpoint controls (54 total). The HTTP fixture buffers the full request body and checks that the original method and synthetic POST body actually arrived before issuing the redirect. It requires no redirected landing/repeated-start request, regardless of redirect method conversion or preservation. This does not claim to qualify successful redirected POST body semantics; the security boundary forbids the destination request.

`runHttpsDowngradeRedirectTests()` adds ten GET/POST cases for the same codes. A fresh off-the-record profile trusts only the local private test CA; its HTTPS positive control must succeed without certificate-error acceptance. The TLS server then issues a real redirect to an independently proven live HTTP endpoint. The test requires one TLS redirect response, a native insecure-transport block and zero HTTP landing requests. Target counters exclude unrelated favicon traffic. Each navigation has a ten-second timeout.

This is the native WebEngine/interceptor boundary. Multi-hop/UI consent presentation, DNS/CNAME changes and public/private network transitions remain separate work.

## Redirect chain through actual UI

The transport harness now follows a three-address 302→307 chain between two loopback origins. It requires a separate click on the HTTP consent button at each hop; a redirect must replace the old context and offer the precise next URL. Previous tokens must be rejected. Server counters require each hop exactly once, so a hidden request followed by a consented retry fails. The terminal page remains isolated and is explicitly revoked; six temporary profile QPointers (three original streaming contexts plus three chain contexts) must clear while the unrelated tab is preserved. The chain has a 45-second deadline.

The shared TCP fixture now buffers request headers and handles a request only once per connection. The redirect peer endpoint is local and synthetic. Neither this UI scenario nor the native redirect matrix qualifies DNS/CNAME rebinding.

The native HTTP negative matrix waits for either renderer completion or an explicit synchronous MainFrame deny with the expected reason plus a short event-loop settlement; target server counters and deny metadata remain mandatory. A missing completion signal alone can never pass. The page is stopped during cleanup after the network verdict. Successful direct controls still require loadFinished(true). Renderer error-page completion timing is separate from the network-policy assertion.

The shared UI smoke prerequisites emit BrowserTab.errorNotice with a synthetic message and require the actual pageLoadErrorDialog popup to become visible, then close it before the other interactions. This guards the QML signal/id name collision. Download navigations and Chromium ERR_ABORTED (-3) are excluded from page-failure notices; completed/cancelled download UI regressions remain in the suite.

## Resolver-alias diagnostic probe

`build/Release/cybou_tests.exe --private-network-probe` runs a separate, intentionally non-green diagnostic. Test-process-only Chromium resolver rules map `cybou-source.test` and `cybou-alias.test` to loopback; no OS resolver, hosts file or production flags change. HTTP is explicitly enabled only on the diagnostic profile so HTTPS-only denial cannot mask the PNA decision. Direct literal and alias navigations must first reach the fixture. A page under the source hostname then issues no-cors fetches to loopback, localhost, decimal and hexadecimal loopback plus the alias. Literal cases require four native private-network-access events with the source first-party and zero server hits.

Exit 2 means broken controls/assertions. Exit 3 records that the alias request actually reached loopback despite its URL-public hostname (known implementation gap). Exit 4 records an inconclusive alias denial requiring engine-level diagnosis. Neither 3 nor 4 is release PASS. Server counts distinguish network access from CORS response readability. The actual source connection is also loopback: this does not simulate Chromium's public IP address space, true DNS answer changes, CNAME chains, HTTPS or WebRTC. It probes the app's URL-classification boundary only and is deliberately separate from the six-suite baseline.

## PNA compatibility suite

`cybou_tests.exe --pna-test` (gate suite `pna`) guards the production engine flag `BlockInsecurePrivateNetworkRequests`. Four loopback servers on fixed ports are assigned public/local address spaces via `--ip-address-space-overrides`; hostnames map to loopback through test-process resolver rules. Six fetch cases assert exact outcomes and target hit counters (only public->local blocked), plus a direct local navigation control. No app interceptor is installed, so only the engine decides. Fixed ports 38471-38474 must be free.

The `pna` suite additionally covers a secure public source (ports 38476-38479, profile-local test CA): local HTTPS target granting the PNA preflight is reachable; a denying target (literal and hostname alias) sees only OPTIONS and zero GET; local->local needs no preflight. `--https-pna-probe` is the raw-engine diagnostic (set `CYBOU_PROBE_DENY_PNA=1` for denying targets and supply features via `QTWEBENGINE_CHROMIUM_FLAGS`). Ports 38471-38479 must be free.

`pna` redirect cases serve 302 responses from the public fixture to a local target; WebSocket cases open `ws://` to the local fixture, which never completes the handshake, so they are judged by the target hit counter. A `knownGap` case that does not reach its expected counter logs `PNA_COMPAT_KNOWN_GAP` instead of failing; it must never be reported as a PASS of the boundary.

`pna` app-layer cases install `RequestObserver` with a stub host resolver and load a public-source page over HTTP: `ws://` to a loopback literal and to a loopback-resolving alias must show zero target hits; a public-resolving name must still connect. A real resolver (CNAME chain, changing answers) is not covered.

`--webrtc-probe` is a raw-engine diagnostic for ICE behaviour: local candidates with and without `WebRTCPublicInterfacesOnly`, and STUN checks that reach a test-owned UDP socket at loopback after a remote candidate is injected. It reports observations and exits 0 when the experiment ran; interpret the counts, not the exit code.

## Strict WebRTC suite

`cybou_tests.exe --webrtc-test` (gate suite `webrtc`) composes engine flags without touching the environment and then starts two child runs of itself (`--webrtc-strict-child=0|1`) because the policy is a process-wide Chromium switch. Each child loads a page, injects a loopback ICE candidate and reports `stunChecksAtLoopback`. Pass requires control >= 10 and strict == 0. The suite needs a free loopback UDP port and about a minute.
