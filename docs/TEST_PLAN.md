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
