# Third-party components, assets and licensing

The repository's original source code is offered under [Apache-2.0](LICENSE).
**Do not assume that a repository-wide license replaces the licenses of third-party dependencies, trademarks, artwork, fonts or imported assets.**

This is a **pre-release licensing inventory**, not a complete shipping SBOM or redistribution clearance.

## Known external boundaries

| Component / material | How it is used | Release obligations |
| --- | --- | --- |
| Qt 6 (Quick, QML, Network, Positioning, Test) | Application runtime and build dependencies | Confirm specific Qt module versions, exact applicable LGPL/GPL/commercial terms, distribution method, notices and license obligations |
| Qt WebEngine / Chromium | Browser engine and related runtime processes | Review Chromium third-party licenses, copyright notices, bundled component lists and redistributed binaries |
| Platform components / Microsoft runtimes | Windows deployment and system integration | Follow the applicable upstream redistribution terms |
| Cloud-service names and brand icons | Provider-selection and web shortcuts | Respect provider trademarks and permitted use; no endorsement or partnership is implied |
| CYBOU branding, screenshots, wallpaper, SVG/PNG/JPG assets | Product identity and documentation | Establish provenance and rights per asset; source-code licensing alone is not proof of rights to third-party images or marks |
| OWASP / other standard reference data | Test planning and review fixtures | Retain upstream attribution and applicable license terms |

## Source and release policy

- Do not copy code, brand graphics, icon packs, wallpaper or standards from other projects without documenting the exact origin and terms.
- The root Apache-2.0 license covers original project contributions supplied under it. Files with explicit separate notices keep their own terms.
- Project branding is not licensed as a trademark under Apache-2.0 (see section 6). Permission to imply CYBOU endorsement is not granted.
- Before distributing installers, generate a dependency/component inventory with versions, license identifiers, source URLs and required notice text, and review runtime deployment terms.
- Record provenance for `assets/cybou.png`, `assets/cybou-alpine-dawn-1920x1080.jpg`, `assets/cloud/` and `assets/controlbar/` before release. Until the review is complete, do not claim every asset is cleared for unrestricted reuse.
- Keep third-party licenses and required notices with source and binary distributions as applicable.

**Status:** incomplete; not a statement of signed/qualified, third-party-cleared release. See [Release criteria](docs/RELEASE_CRITERIA.md), [Validation](docs/VALIDATION.md) and [Security model](docs/SECURITY_MODEL.md).
