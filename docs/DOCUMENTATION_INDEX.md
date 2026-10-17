# CYBOU documentation index and authority

**Current declared product version: `0.3.0-alpha.1` (engineering alpha, not qualified).** The previously used "0.8" is a historical design/prototype identifier only. Existing dated local test logs do not qualify the new version. See [VERSIONING](VERSIONING.md).

## Normative product and security sources

- [VERSIONING](VERSIONING.md) · [ADR 0008](ADR/0008-evidence-driven-versioning.md): status, naming and release evidence.
- [ROADMAP](ROADMAP.md): gated stages through 1.0.0.
- [RELEASE_CRITERIA](RELEASE_CRITERIA.md): strict release GO/NO-GO.
- [SECURITY_VERIFICATION_FRAMEWORK](SECURITY_VERIFICATION_FRAMEWORK.md): OWASP applicability inventory, test orchestration and evidence requirements.
- [THREAT_MODEL](THREAT_MODEL.md): trust boundaries and threats.
- [TEST_PLAN](TEST_PLAN.md): concrete test suites and execution.
- [VALIDATION](VALIDATION.md): historic evidence, not current release proof.
- [PRODUCT_VISION](PRODUCT_VISION.md), [CONTROL_BAR_UX](CONTROL_BAR_UX.md), [CAPABILITY_CONTRACT](CAPABILITY_CONTRACT.md): feature and UX contracts.
- [ADR 0006](ADR/0006-no-third-party-extensions.md), [ADR 0007](ADR/0007-selectable-cloud-services.md): trusted execution / selectable cloud ecosystem.

## Other active supporting docs

[ARCHITECTURE](ARCHITECTURE.md), [SECURITY_MODEL](SECURITY_MODEL.md), [SECURITY_UX_EVOLUTION](SECURITY_UX_EVOLUTION.md), [API_CAPABILITY_MATRIX](API_CAPABILITY_MATRIX.md), [CLOUD_COMPATIBILITY_MATRIX](CLOUD_COMPATIBILITY_MATRIX.md).

## Historical design and implementation snapshots

Documents in `docs/*-source/` and original reports named `OMNIBOX_071_INTEGRATION`, `CONTROL_BAR_INTEGRATION` and `OBSERVABLE_BROWSING_PROPOSAL_RU` are **historical**. Their internal version names and historical PASS claims are not current engineering or release evidence. Design images are likewise reference captures, not screenshots from a verified 1.0.0 release.

**Precedence on conflict:** owner decisions / ADRs → current Versioning, Release Criteria, Roadmap and Test Framework → feature contracts → dated implementation/validation reports → historical snapshots. Code remains the source for what actually exists; evidence remains the source for what was verified.
