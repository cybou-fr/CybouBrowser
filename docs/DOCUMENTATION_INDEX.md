# CYBOU — active documentation

**Current source: 0.4.0-alpha.1. No public release.** All pages below describe the existing code, accepted product requirements, open verification work or explicit future release gates. Git provides the change history.

| Document | Purpose |
| --- | --- |
| [Architecture Overview](ARCHITECTURE_OVERVIEW.md) | Reviewer-friendly components, trust boundaries and flows |
| [Engineering Decisions](ENGINEERING_DECISIONS.md) | Trade-offs and pointers to binding ADRs |
| [Engineering Quality](ENGINEERING_QUALITY.md) | Test/evidence model and reproducibility limits |
| [Étude de cas (FR)](CASE_STUDY_FR.md) | Professional case study for clients and technical reviewers |
| [Product Vision](PRODUCT_VISION.md) | Fixed product behavior and priorities |
| [Architecture](ARCHITECTURE.md) | Current C++ / QML / WebEngine modules and boundaries |
| [Capability Contract](CAPABILITY_CONTRACT.md) | Observable and controllable site capabilities |
| [Control Bar UX](CONTROL_BAR_UX.md) | Permanent single-row controls and glass panels |
| [Security Model](SECURITY_MODEL.md) | Implemented defenses, limitations, privilege boundaries |
| [Threat Model](THREAT_MODEL.md) | Threats and negative verification work |
| [Cloud Services](ADR/0007-selectable-cloud-services.md) | Provider-neutral C++/QML web-app chooser and next verification |
| [Cloud Compatibility](CLOUD_COMPATIBILITY_MATRIX.md) | Catalog and real-account qualification checklist |
| [No Extensions](ADR/0006-no-third-party-extensions.md) | Mandatory zero-third-party-extensions policy |
| [Versioning](VERSIONING.md) | Source version and promotion gates |
| [Roadmap](ROADMAP.md) | Required engineering work before 1.0.0 |
| [Test Plan](TEST_PLAN.md) | Executable suites and additional required campaigns |
| [Security Verification Framework](SECURITY_VERIFICATION_FRAMEWORK.md) | OWASP imports, case coverage, execution evidence |
| [OWASP Triage](OWASP_TRIAGE.md) | Browser-specific applicability, mappings and remaining verification gaps |
| [Validation](VALIDATION.md) | Current verification status, without unsupported PASS claims |
| [Release Criteria](RELEASE_CRITERIA.md) | Formal GO/NO-GO |
| [ADR 0005](ADR/0005-origin-scoped-tracker-exceptions.md) | Current origin/profile tracker-exception policy |
| [ADR 0008](ADR/0008-evidence-driven-versioning.md) | Mandatory evidence-driven release decision |
| [Data Classification](DATA_CLASSIFICATION.md) | Draft data inventory and proposed protection levels (unapproved) |

**Source code is authoritative for what exists. Executed, accessible test artifacts are authoritative for what passed.** Unimplemented or unverified functionality must be explicitly labelled.
