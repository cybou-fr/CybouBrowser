# CYBOU Browser — Engineering quality and verifiable evidence

**Purpose:** give external reviewers a clear path from a quality claim to its source, test and limitation. This page is not a security certification or an independent audit.

## Claims and evidence levels

| Evidence level | What it establishes | What it does NOT establish |
| --- | --- | --- |
| Architecture / ADR | Intent, responsibilities and trade-offs are documented | Runtime implementation and correctness |
| Source inspection | A mechanism exists in code | It behaves safely in every engine/OS state |
| Isolated test / fixture | A defined scenario passed in a controlled environment | All browser-origin or hardware attack variants |
| Native end-to-end run with artifacts | A specific binary, environment and scenario passed | A general security guarantee |
| Signed release + reviewed coverage | Exact packaged candidate meets stated gates | Perpetual safety against future threats |

## Project test infrastructure

- `cybou_tests` exposes native/integration scenarios for permission controls, transport, tracker policies, surfaces and smoke behavior.
- `tools/security/owasp_gate.py` manages repeatable baseline and stricter release checks, with required evidence tied to source and binary identity.
- Official OWASP ASVS/WSTG inputs are recorded in the repository; importing identifiers is not the same as verifying applicability or proving compliance.
- [Validation status](VALIDATION.md) records locally reported checks, source hashes, exceptions, outstanding security questions and the blocked release gate.

### Reproduction (Windows development environment)

Follow the exact Qt/MSVC build requirements in the [root README](../README.md#build-and-run), then:

```powershell
python tools/security/test_owasp_gate.py
python tools/security/owasp_gate.py --mode baseline --exe build/Release/cybou_tests.exe --out build/security-baseline.json
```

These are instructions, **not a claim that the commands were executed for this documentation change**. The release mode has stricter requirements; see [Testing plan](TEST_PLAN.md).

## Current limits to confidence

- Local passing suites are evidence for specific tested paths, not proof of perfect permission revocation, anti-tracking, full process sandbox or anonymity.
- Runtime WebRTC/DNS/PNA/WebSocket boundaries, hardware use/revoke, iframe/redirect adversarial conditions and packaged Qt/Chromium behavior need additional qualification.
- Signed distribution, SBOM/third-party license review, secure updates/rollback and independent assessment remain release requirements.
- The repository's `docs/VALIDATION.md` is a status ledger and must not be reworded as an already completed independent audit.

## Reviewer checklist

1. Compare the product claim with source and [Capability contract](CAPABILITY_CONTRACT.md).
2. Check the relevant [threat](THREAT_MODEL.md), boundary and [decision](ENGINEERING_DECISIONS.md).
3. Run or request native test evidence for the exact commit, executable hash, platform and scenario.
4. Read the documented gaps in [Validation](VALIDATION.md).
5. Do not promote a development snapshot to a safe-to-deploy product without [Release criteria](RELEASE_CRITERIA.md).

[Architecture overview](ARCHITECTURE_OVERVIEW.md) · [Security policy](../SECURITY.md) · [Contribution guidelines](../CONTRIBUTING.md)
