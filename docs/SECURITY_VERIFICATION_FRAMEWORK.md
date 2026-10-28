# CYBOU Security Verification Framework (normative)

**Scope:** Windows desktop C++20 / Qt Quick / Qt WebEngine browser, including native C++ controls, Chromium subprocesses, OS integration, QML, profiles, permissions, media, downloads, network interception, cloud launcher and distribution.

**Current state:** `0.4.0-alpha.1`. The repository includes an **unqualified Python standard-library baseline runner**, a machine-readable scenario inventory and a strict **release coverage gate**, but **does not yet supply full OWASP data, mappings, evidence or passing release qualification**. A written plan is not a passed test.

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
- `tests/security/cases.json`: browser risk case catalog (69 cases today), each with a domain, tier and intended method; `planned` does **not** imply implemented or passing. Tiers rank work in the catalog: 42 P0 cases are 42 priority scenarios, not a count of all risks; the catalog grows as threats are found.
- `tests/security/threats.json`: curated MITRE CWE weakness classes and CAPEC attack patterns for a desktop browser, each linked to the cases that must exercise it.
- `tests/security/vulns/`: offline known-vulnerability inputs for the risk gate: `snapshot.json` (normalized NVD/OSV records), `kev.json` (CISA KEV IDs) and `triage.json` (VEX-style decisions). `tools/security/import_vulns.py` normalizes downloaded feeds; the gate itself never goes online.
- `tests/security/coverage.json`: mapping of **every** official imported requirement/scenario ID to disposition `applicable` or `not_applicable`; mappings start empty. For applicable items: `owner`, `caseIds`, `verificationStatus: verified` and named `assertions`; for N/A: an item-specific `rationale`, `reviewedBy` listed in `tests/security/reviewers.json`, `reviewedOn` and `reviewRef`.
- `tests/security/evidence.json` (created by QA when genuinely tested): exact case, result, HEAD, binary SHA-256, log path and its SHA-256. `evidence.example.json` is an example only; no synthetic PASS.
- `tools/security/owasp_gate.py`: offline gate/runner, JSON report, nonzero exit on failures.
- [THREAT_MODEL](THREAT_MODEL.md): assets, trust boundaries, attack surfaces, failure modes.
- [RELEASE_CRITERIA](RELEASE_CRITERIA.md): security gate for RC/1.0.0.

## Four gates

The tool and the product are judged separately: a complete framework says nothing about whether the browser is secure.

| Mode | Question it answers | Needs |
| --- | --- | --- |
| `framework` | Is the testing toolkit complete and consistent? Every pinned OWASP control decided or carrying a reasoned proposal, every link to a case resolves, the threat catalog is well formed. Gaps (P0 cases without a threat, unlinked cases) are reported, not failed. | Repository only |
| `test` (was `baseline`) | Do the native suites run and pass on this build? | `cybou_tests.exe` |
| `risk` | Does a known vulnerability (NVD/OSV, CISA KEV) match a component version in the SBOM, and is each match triaged? Feeds older than `--max-snapshot-age` days (7) block. | SBOM, `tests/security/vulns/` |
| `r3` | 0.5.0 acceptance: the test suites, then for every P0 `execution` case its `evidence` markers in this run's suite logs (or a `tool:` script), no open required scenario (one prefixed `[rc]` is deferred to the release candidate), and every CISA KEV match fixed, not affected, or affected with a mitigation in place by default. | `cybou_tests.exe`, SBOM, `tests/security/vulns/` |
| `release` | All of the above, plus current-revision evidence for every case, verified applicable rows and reviewer-approved N/A rows. | Clean tree, candidate binary |

```powershell
python tools/security/owasp_gate.py --mode framework --out build/security-framework.json
python tools/security/import_vulns.py --kev known_exploited_vulnerabilities.json
python tools/security/import_vulns.py --nvd nvd-chrome.json --cpe google:chrome=chromium --sbom build/CYBOU-sbom.cdx.json
python tools/security/owasp_gate.py --mode risk --sbom build/CYBOU-sbom.cdx.json --out build/security-risk.json
```

Chrome version ranges in NVD do not account for the security fixes Qt backports into its Chromium base (140.0.7339.225 for Qt 6.12). `make_sbom.py` reads the Qt, Qt WebEngine and Chromium versions from the staged binaries (Qt6Core.dll and Qt6WebEngineCore.dll version resources, and the two Chromium version strings the engine embeds), so the SBOM describes what is shipped. Qt WebEngine 6.140.0 in Qt 6.12.0 declares a security patch level of **152.0.7977.83** (`qWebEngineChromiumSecurityPatchVersion()`, `CHROMIUM_VERSION` in the Qt sources; the sources name the base 140.0.7339.264 while the binary reports 140.0.7339.225, to be reconciled). The patch level is Qt's claim, not proof for an individual CVE. A match is therefore never dismissed by version alone: it is recorded in `triage.json` as `fixed` with the Qt release that carries the fix, `not_affected` with a justification, or `affected`, which blocks.

### First known-vulnerability run (2026-10-22)

CISA KEV catalog 2026.10.21 (1754 CVEs) and 7153 NVD `google:chrome` records: **3470 match** Chromium 140.0.7339.225 by version (393 critical, 1450 high), fixed upstream in Chrome 141-156. With `--sbom`, only these matches are versioned in `snapshot.json`; the complete feed stays in the ignored `snapshot.full.json`. The 9 CISA KEV matches are triaged in `tests/security/vulns/triage.json`:

| CVE | Decision | Basis |
| --- | --- | --- |
| CVE-2025-13223, CVE-2025-14174, CVE-2026-2441, CVE-2026-3909, CVE-2026-5281, CVE-2026-11645 | fixed | `[backport]` commit on the Qt 140-based branch before the Qt 6.12.0 build |
| CVE-2026-3910 | fixed | Part 2/2 backported; part 1/2 deliberately reverted, matching Chrome 144-LTC |
| CVE-2026-85046 | not_affected (source inspection) | The fixed Array.prototype.sort inlining does not exist in the shipped V8; to be confirmed with the upstream regression test |
| **CVE-2026-87491** | **affected** (source inspection, 2026-10-22) | Upstream fix v8 `36079c36283a` replaces the `WasmGetOwnProperty` builtin, which can run getters while Turboshaft treats it as side-effect free; the shipped V8 has the pre-fix code unchanged and Qt has no backport. Reachable from WebAssembly. Exploited in the wild. **Mitigated by default:** WebAssembly is unavailable in the engine (`--js-flags=--jitless`: the shipped V8 has no `--expose-wasm` and hides WebAssembly only without the JIT, so pages run slower; Settings › Security, tested by `--wasm-test`); the status stays affected and blocks the risk gate until a Qt WebEngine with the fix ships |

**Automatic `patch-level` status.** A match without an explicit decision is resolved as `patch-level` only when both facts hold: every affecting range is fixed upstream at or below the security patch level recorded in the SBOM, and `tests/security/vulns/qt-backports.json` lists a non-reverted `[Backport] CVE-...` commit for it on the Qt branch before the shipped build. The index is built by `tools/security/qt_backports.py --until <build time>` (1192 CVEs from 2100 commits up to 2026-09-28; `GITHUB_TOKEN` lifts the anonymous API limit). An explicit triage decision always wins.

**Evidence levels.** A resolution is only as strong as its evidence, and the gate records which level each one rests on:

| Level | Meaning | Accepted by |
| --- | --- | --- |
| `binary` (`method: binary-test`) | Checked against the shipped engine, e.g. the upstream regression test run on it | risk, release |
| `source` (`method: source-inspection`) | The shipped Qt sources inspected: fix present or vulnerable code absent | risk, release |
| `branch` (`method: backport-commit`) | A backport commit exists on the Qt branch before the build; its presence in the binary is not shown | risk only |
| `branch+declared` (`patch-level`) | Vendor patch level plus a backport commit | risk only |

`fixed` and `not_affected` need a `method` and `evidence` links; `not_affected` also needs a justification. `--mode release` rejects `branch` and `branch+declared`, so the current 1100 resolutions keep a release BLOCKED until they are confirmed at source or binary level. `snapshot.json`, `kev.json` and `qt-backports.json` carry a `contentSha256` written at import; an edit after import blocks the risk gate. A `source-inspection` decision carries a `sourceBinding`: the Qt WebEngine version and patch level of the inspected tree and the SHA-256 of every file read. The gate rejects a binding that names another release than the SBOM, and where that release's source tree is present (`--qt-src`, default `C:/Qt/<qt>/Src/qtwebengine`, identified by its `.cmake.conf` and `CHROMIUM_VERSION`) it re-hashes the files; release mode requires that verification. In `test` and `release` mode the `ENGINE` line that `cybou_tests` prints from the Qt runtime API (`qVersion`, `qWebEngineVersion`, `qWebEngineChromiumVersion`, `qWebEngineChromiumSecurityPatchVersion`) must equal the versions the SBOM read from the staged binaries, which closes the gap between parsed binaries and the running engine.

The framework gate also regenerates the [N/A report](NA_REPORT.md): each N/A category rests on architecture facts (patterns the product sources must not contain, defined in `tests/security/na-categories.json`); a broken fact or a stale report fails the gate. It also checks that every `testSources` file of a case exists and that cases marked in progress or verified name their implementation.

| Status of the 3470 matches | Count | Critical / high |
| --- | --- | --- |
| `patch-level` (declared level + backport commit) | 1093 | 136 / 572 |
| Triaged by hand (KEV) | 9 | 0 / 9 |
| Open: fixed upstream within the patch level, no `[Backport] CVE-...` commit found | 1470 | 141 / 502 |
| Open: fixed upstream after the patch level (Chrome > 152.0.7977.83) | 898 | 116 / 367 |

Open within the level does not mean unpatched: Qt also lands fixes as `Security bug <crbug>` commits, as engine version updates, or not at all for code it does not ship (Android, iOS, ChromeOS, Chrome-only features). Those need a broader matcher (crbug IDs from NVD references) or source inspection. Open after the level waits for a newer Qt WebEngine patch release. The risk gate stays BLOCKED on 2368 open matches.

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

- **0.4:** official inventories pinned; every control decided or carrying a reasoned proposal; threat catalog and known-vulnerability gate in place; `--mode framework` passes.
- **0.5:** every P0 `execution` case has an executable negative test with preconditions, expected result, assertions and evidence; known critical engine vulnerabilities triaged. `framework` and `process` cases (`kind` in `cases.json`) gate later stages.
- **0.6:** planned 1.0 functionality frozen, browser and desktop integration suites reliable; coverage and failures independently reviewed.
- **RC:** every applicable requirement has fresh evidence, review/N/A signed off, no outstanding critical/high, actual executable sandbox, updates and packaging validated, external audit complete or explicit release BLOCKED.
- **1.0:** same candidate passes all checks and support process. No blanket certification language.

This tool **does not itself generate all OWASP tests**. Maintaining the official inventory, mapping and real fixtures is a substantial engineering obligation. **Unknown remains BLOCKED, never automatically PASS.**

## Local baseline diagnostics (2026-10-18)

`cybou_tests` emits Qt diagnostics to stderr as well as the local aggregate `build/test_log.txt`. The gate captures stderr/stdout separately for each suite under the report directory's `security-logs/`. A zero process exit with an empty log is a failure. Keep the JSON report and these logs together; the report includes each log's SHA-256 and byte count, binary SHA-256, source commit, working-tree state and tracked diff SHA-256.

A dirty-tree baseline is diagnostic evidence for a development build. Its commit field alone does not identify the tested source; preserve the patch as well. Release mode requires a verified clean tree. Untracked source must be committed before qualifying a candidate. The source-diff hash is an identifier, not a saved copy of the patch or proof of reproducible compilation.

Run from the repository root after a successful Release build:

```powershell
python tools/security/test_owasp_gate.py
python tools/security/owasp_gate.py --mode baseline --exe build/Release/cybou_tests.exe --out build/security-baseline.json
```

All six suites passing proves only this local baseline. It does not disposition official requirements or qualify hardware, cloud services, signing, sandbox isolation or independent review.

## Vendored official inputs (2026-10-18)

Default inventory inputs are now available offline in `tests/security/standards/`; no manual dataset download is needed for a normal local run. Use `python tools/security/owasp_gate.py --mode inventory --out build/owasp-inventory/report.json`. Explicit `--asvs-csv` and `--wstg-ids` overrides still require matching lock hashes. The official ASVS CSV uses the `req_id` header and `V1.1.1` IDs. Import checks exact counts (345 ASVS / 97 WSTG), duplicates, malformed rows, source hashes and WSTG source-to-index agreement. See the standards README for provenance, licensing and the upstream JSON discrepancy.

Coverage currently imports all 442 controls as `untriaged`. This is an inventory completion step, not completion of the 0.4 maturity gate. Assign applicability, owners, case mappings and current evidence through review; keep unknown decisions blocked.
