# Contributing to CYBOU Browser

Thank you for your interest in CYBOU Browser. It is a real product under development, not a security-certified browser or a generic extension platform.

## Start here

- [Product vision](docs/PRODUCT_VISION.md) and [architecture overview](docs/ARCHITECTURE_OVERVIEW.md)
- [Engineering decisions](docs/ENGINEERING_DECISIONS.md) and existing [ADRs](docs/DOCUMENTATION_INDEX.md)
- [Capability contract](docs/CAPABILITY_CONTRACT.md) and [security model](docs/SECURITY_MODEL.md)
- [Testing](docs/TEST_PLAN.md), [engineering quality](docs/ENGINEERING_QUALITY.md) and [validation status](docs/VALIDATION.md)
- [Build steps](README.md#build-and-run)

## Scope and review

Before implementing a major feature or changing a trust boundary, open a GitHub issue describing the problem, proposed behavior, alternatives, risks and observable acceptance tests. Small corrections may be proposed directly.

The maintainer develops on `main`. External contributors can submit a pull request **targeting `main`** from their own fork; a permanent feature-branch hierarchy is not required in this repository. Please keep each contribution focused and explain its impact on security, portability and user experience.

A useful PR includes:
1. The concrete user problem and intended behavior.
2. The architectural or security boundary affected and why the chosen design is preferable to alternatives.
3. Tests added or updated, with commands and **actual** observed results.
4. Documentation updates for changed behavior, known limitations and relevant ADRs.
5. The source of any third-party code, binary, asset or documentation plus applicable license terms.

## Non-negotiable design rules

- Native C++20 and Qt/QML; put policy and lifecycle decisions in the appropriate C++ layers rather than QML-only assumptions.
- Keep eleven individual capability controls in the existing navigation row.
- Never grant persistent positive permissions implicitly; distinguish policy, observed request, granted permission, confirmed block and hardware usage.
- No third-party browser extensions, stores, side-loading or privileged userscripts. [ADR 0006](docs/ADR/0006-no-third-party-extensions.md).
- Cloud providers are ordinary web sites. Do not grant them privileged treatment or read account credentials for the app launcher.
- Security claims require reproducing the corresponding relevant scenario. **Plans, unit-test stubs and a passing compilation are not release evidence.**

## Build, testing and validation

On Windows x64, use the build commands in [README](README.md#build-and-run) and the baseline gate described in [README](README.md#testing). Include the specific Qt version, commit and test command with the result.

A passing local test does not make the application production-ready. The requirements for release are in [Release criteria](docs/RELEASE_CRITERIA.md).

## Responsible security disclosure

Please do not publish vulnerability details or exploit instructions in a public issue or PR. Use [SECURITY.md](SECURITY.md) to arrange private disclosure.

## License and contribution rights

By contributing, you agree that contributions you have the legal right to submit are made under the repository's [Apache License 2.0](LICENSE). Do not submit third-party materials without compatible rights and their required notices. No separate contributor license agreement is asserted here.
