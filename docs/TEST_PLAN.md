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
