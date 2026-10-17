# CYBOU Security Verification Framework (normative)

**Scope:** Windows desktop C++20 / Qt Quick / Qt WebEngine browser, including native C++ controls, Chromium subprocesses, OS integration, QML, profiles, permissions, media, downloads, network interception, cloud launcher and distribution.

**Current state:** `0.3.0-alpha.1`. The repository includes an **unqualified Python standard-library baseline runner**, a machine-readable scenario inventory and a strict **release coverage gate**, but **does not yet supply full OWASP data, mappings, evidence or passing release qualification**. A written plan is not a passed test.

## Reference baselines, pinned per release

| Reference | Stable version | Use | Scope note |
| --- | --- | --- | --- |
| [OWASP ASVS](https://github.com/OWASP/ASVS) | **5.0.0** | Full machine-readable requirement inventory, applicability, security claims and test/evidence mapping | Requirements written for web apps/services must be triaged honestly for a desktop client |
| [OWASP WSTG](https://wstg.owasp.org/v4.2/) | **4.2** | Full testing-scenario inventory; apply only to CYBOU-owned code/local test fixtures, not live third-party services | Every scenario must be mapped or have reviewer-approved N/A rationale |
| [OWASP Top 10](https://top10.owasp.org/2025/) | **2025** | Threat taxonomy (A01 through A10), NOT a substitute for test cases | Includes misconfiguration, supply-chain, cryptography and exceptional conditions |
| [OWASP SCVS](https://scvs.owasp.org/) | **1.0** | Component and supply-chain assurance, SCA, SBOM, build provenance | Manual + automated checks, explicitly tracked |
| [OWASP SAMM](https://owasp.org/www-project-samm/) | **2** | Secure SDLC governance and maturity review | Process assessment; not a browser exploit test |

Avoid the misleading term **"tests all OWASP"**: OWASP publishes many projects for unrelated platforms. The correct promise is **complete disposition of every requirement/test in the pinned applicable baselines, plus browser-specific controls and threat review**, with no unjustified N/A. Do not copy external standards into privileged executable modules.

## Sources of truth in this repository

- `tests/security/standards.lock.json`: official version/source, pinned SHA-256 for ASVS CSV and WSTG scenario-ID inventory. SHA fields start **null** and therefore release certification is blocked until independently pinned.
- `tests/security/cases.json`: **64 initial browser/OWASP-review case IDs**, each with a risk domain and intended method; `planned` does **not** imply implemented or passing.
- `tests/security/coverage.json`: mapping of **every** official imported requirement/scenario ID to disposition `applicable` or `not_applicable`; mappings start empty. For applicable items: `owner` and `caseIds`; for N/A: explicit `rationale` and `reviewedBy`.
- `tests/security/evidence.json` (created by QA when genuinely tested): exact case, result, HEAD, binary SHA-256, log path and its SHA-256. `evidence.example.json` is an example only; no synthetic PASS.
- `tools/security/owasp_gate.py`: offline gate/runner, JSON report, nonzero exit on failures.
- [THREAT_MODEL](THREAT_MODEL.md): assets, trust boundaries, attack surfaces, failure modes.
- [RELEASE_CRITERIA](RELEASE_CRITERIA.md): security gate for RC/1.0.0.

## How to run locally (no GitHub Actions required)

Prerequisites: Python 3.10+, Windows MSVC/Qt Release build, isolated test data. From repository root:

```powershell
cmake --build build --config Release --parallel 4
python tools/security/owasp_gate.py --mode baseline --exe build/Release/cybou_tests.exe --out build/security-baseline.json
```

First run the runner self-tests: `python tools/security/test_owasp_gate.py`. This **executes** six existing `cybou_tests` suites (security, controlbar, transport, tracker, surface, smoke), writes separate logs in `build/security-logs` and emits JSON including SHA-256. It does **not** prove full ASVS coverage.

Full release gate (currently intentionally blocked):

```powershell
python tools/security/owasp_gate.py --mode release --exe build/Release/cybou_tests.exe --asvs-csv path/to/official-ASVS-5.0.0.csv --wstg-ids path/to/verified-WSTG-v42-IDs.txt --out build/security-release.json
```

**Input preparation:** obtain the official ASVS 5.0.0 English CSV from its release; obtain and independently validate **the complete** WSTG v4.2 test-ID list from its versioned index (one `WSTG-v42-CAT-01` per line). Review source/license, pin each file's SHA-256 in `standards.lock.json` and store immutable copies securely for repeatability. A too-small import or a digest mismatch **blocks** the gate. Keep test inputs locally; do not fetch remote dependencies automatically.

Audit phase requires **all** IDs classified: mapped `applicable` -> test cases -> fresh PASS logs on exact code + binary, or reviewed `not_applicable` with rationale. `SKIP`, "not run", missing artifact, unknown requirement or out-of-date evidence are not PASS. Current empty coverage file means an **intentional BLOCKED** result. External manual pentest and production signed package review are additional gates not faked by the script.

## Test portfolio (minimum; expand for every discovered threat)

1. **Native deterministic:** origin canonicalization, permission identity and lifecycle, stale request/tab/epoch, iframe top/requester policy, cross-origin/profile isolation; HTTP consent, request blocker categories and exception scope.
2. **Integration in local controlled sites:** iframe, CSP/sandbox headers, cross-origin redirects, mixed content, credentialed URLs, cookies/session, subresource loading, WebSockets, private-network redirects and DNS, browser-owned JS injection and fail-closed behavior.
3. **Fuzz/property and resource budgets:** URL parsers, IDN, rule snapshot imports, settings JSON, download paths, malformed responses; fail closed on crash/timeout; ASan/UBSan supported platform configurations (not guaranteed on every Windows toolchain).
4. **Real hardware and OS:** camera/microphone/screen recording, clipboard, media revoke during long calls, unplug, suspend/resume, multiple monitors, hardware permission denied, crash recovery, certificates and safe downloaded-file opening.
5. **Supply chain and release:** reproducible source/build inputs, engine CVE patch triage, dependency licensing and SBOM, production sandbox/process flags, no extension manager/privileged plugins, safe signing/installation/update/rollback, secret/credential leakage in logs.
6. **UX/privacy/accessibility:** exactly 11 permanent Control Bar icons, hover, keyboard and screen reader, 820/1024/1360/1920 widths, DPR 1/1.5/2, FR/RU, reduce motion, cloud launcher no prefetch, actual provider per role and no elevation for partners.
7. **Adversarial review:** threat-driven manual penetration testing on test-owned services, abuse cases, negative controls and remediation verification. Do not attack real third-party cloud sites without authorization.

## Reporting

Each `PASS` must state case ID(s), linked official requirement IDs, variant/mode/profile, expected/observed behavior, full source commit, binary hash, Qt/Chromium runtime & security patch, OS/CPU, test timestamp and logfile/screenshot. Report separate **coverage** (mapped/total), **verification** (PASS of applicable), **N/A** (approved only), **blocked**, **flaky**, timeouts, and unfixed findings. A summary percentage with missing controls is insufficient.

Daily development may run narrow suites, but before milestone promotion run full baseline, imported OWASP inventory gate, fuzz/hardware/runtime/manual campaigns and release packaging gates appropriate to the milestone. No hidden network telemetry or leaking test credentials.

## Gates

- **0.4:** official inventories pinned and complete triage; framework schema tests and initial automatable mapping.
- **0.5:** priority negative cases implemented, fuzz harness and safe fixture runner, no unreviewed P0 threats.
- **0.6:** planned 1.0 functionality frozen, browser and desktop integration suites reliable; coverage and failures independently reviewed.
- **RC:** every applicable requirement has fresh evidence, review/N/A signed off, no outstanding critical/high, actual executable sandbox, updates and packaging validated, external audit complete or explicit release BLOCKED.
- **1.0:** same candidate passes all checks and support process. No blanket certification language.

This tool **does not itself generate all OWASP tests**. Maintaining the official inventory, mapping and real fixtures is a substantial engineering obligation. **Unknown remains BLOCKED, never automatically PASS.**
