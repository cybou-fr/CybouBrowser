# CYBOU — versioning policy

**Current product source version: 0.3.0-alpha.2.** No public release has been issued.

`CMakeLists.txt` is the version source of truth. It sets product version `0.3.0` plus prerelease `alpha.2`; both `cybou` and `cybou_tests` embed the same `CYBOU_PRODUCT_VERSION`. Alpine Dawn displays `Qt.application.version`. The `Cybou` QML module `VERSION 1.0` is a separate API/module identifier.

## Release stages

| Stage | Required verification |
| --- | --- |
| **0.3.0-alpha.2 — current** | Implemented functional foundation; verification incomplete |
| **0.4.0-alpha.N** | Official OWASP requirements/test inventories pinned and triaged, executable coverage/evidence framework |
| **0.5.0-alpha.N** | Adversarial native/browser security and hardware tests, fuzzing and remediation |
| **0.6.0-beta.N** | Windows feature freeze, integration, usability/accessibility, stability and package testing |
| **1.0.0-rc.N** | Candidate-specific security proof, external review, signed package and full release gates |
| **1.0.0** | First supported public Windows x64 release |

Versions indicate **passed maturity gates, not completion percentages**. No milestone passes because code was merged or a local test name appeared in documentation. Candidate qualification requires logs bound to the exact commit, binary hash and environment.

No branches: development changes go to `main`. A published version requires a qualifying immutable commit and signed artifact; do not infer a release from a source-version string. No automatically implied support for Linux/macOS.

[ROADMAP](ROADMAP.md) · [RELEASE_CRITERIA](RELEASE_CRITERIA.md) · [SECURITY_VERIFICATION_FRAMEWORK](SECURITY_VERIFICATION_FRAMEWORK.md)
