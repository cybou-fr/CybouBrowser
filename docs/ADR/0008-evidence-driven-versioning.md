# ADR 0008 — evidence-driven versioning and release qualification

**Status: ACCEPTED as owner-directed product/security policy, 2026-10-17.**

## Context

The former `0.8.0` identifier represented UI/prototype progress, not systematic verification of the safety of a desktop browser. Partial native tests were sometimes presented as broad security proof. This mismatch must end.

## Decision

1. Rebase declared current development version to **`0.3.0-alpha.1`**. This is a **versioning reset**, not a rollback of features, Git history or user profiles.
2. Version milestones are constrained by security evidence and release criteria, not task-count percentages. Never treat existing PASS logs from older revisions as full-application verification.
3. Adopt the threat-driven [CYBOU verification framework](../SECURITY_VERIFICATION_FRAMEWORK.md), covering **every requirement in pinned official OWASP ASVS v5.0.0** and **every test ID in pinned WSTG v4.2** by applicability disposition, with written rationale for each not-applicable item; OWASP Top 10:2025, SCVS 1.0 and SAMM 2 reviewed at the appropriate level. Supplement OWASP with browser-specific negative tests, sandbox, DNS/WebRTC, native UI and supply-chain checks.
4. Require reproducible reports tied to source SHA, binary hash and environment. Unknown, blocked, not run, stale or untriaged **fails the 1.0.0 gate**.
5. Explicit release scope is supported **Windows x64** first. Other platforms are separately qualified; no implied compatibility.
6. No browser extensions or unreviewed external executables (ADR 0006). Continuous integration is **optional** while unavailable; a **local executable gate** is mandatory.

## Consequences

The new runner, catalogs and test-case inventory are a **framework scaffold**, not a claim that ASVS/WSTG has been executed. Official standards are not yet pinned or exhaustively classified; tests for hardware, fuzzing, credentials, release signing and real web app compatibility remain pending. Therefore the release gate is intentionally BLOCKED today.

See [VERSIONING](../VERSIONING.md), [ROADMAP](../ROADMAP.md), [TEST_PLAN](../TEST_PLAN.md), [RELEASE_CRITERIA](../RELEASE_CRITERIA.md), [VALIDATION](../VALIDATION.md).
