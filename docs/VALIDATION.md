# CYBOU — current verification status

**Current product source: 0.3.0-alpha.1 · no public release.**

This is a status ledger, **not** a test-results archive. A test is PASS only with the executable, log, commit SHA, binary SHA-256 and environment available for review. This repository does not currently contain a complete test evidence set for the current build. Therefore **the 1.0.0 release decision is BLOCKED**.

| Verification target | Current status | Next required evidence |
| --- | --- | --- |
| Build (Windows x64) | Source and CMake targets present; no fresh build log attached here | Successful Release build tied to a current full commit |
| Gate self-tests | `tools/security/test_owasp_gate.py` exists; result not independently recorded here | Execute and save output |
| Native baseline | Six `cybou_tests` suites exist; current SHA-specific report not included | Run `owasp_gate.py --mode baseline` and preserve logs |
| OWASP ASVS/WSTG | Official inventory SHA pins and full requirement mappings incomplete | Pin canonical datasets, triage each item and verify applicable cases |
| Browser-specific security | Cases in `tests/security/cases.json` are plans, not proof | Deterministic fixtures, stress/fuzz, real devices and attack-driven negative testing |
| Chromium/Qt sandbox | Packaged binary runtime boundary not proved | Inspect renderer sandbox/processes and security patch level |
| Cloud interoperability | C++/QML implementation available; vendor login/2FA and media not qualified | Real supported-platform test accounts and logs |
| Supply chain | Signing/SBOM/update process not qualified | Signed package, SBOM, update/rollback tests and independent review |
| Independent security review | Not performed/evidenced for a release candidate | External pentest, risk review and retest |

Do not record a result by copying a previous note. Add verified machine-readable cases under `tests/security/` and generated logs under a dedicated build/test-artifact location. Use [TEST_PLAN](TEST_PLAN.md), [SECURITY_VERIFICATION_FRAMEWORK](SECURITY_VERIFICATION_FRAMEWORK.md), [RELEASE_CRITERIA](RELEASE_CRITERIA.md).
