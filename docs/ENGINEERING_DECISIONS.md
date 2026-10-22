# CYBOU Browser — Engineering decisions and trade-offs

This page is a **reviewer-friendly index of decisions**, not a replacement for source code or binding ADRs. The current baseline is 0.4.0-alpha.1. **Implemented**, **partially proven**, and **future** are intentionally different states.

| Decision | Why / benefit | Cost or limitation | Authority |
| --- | --- | --- | --- |
| C++20 + Qt WebEngine for native browser policy | Direct native lifecycle integration with an existing browser engine without reimplementing Chromium | Depends on Qt/Chromium updates, platform integration and runtime security | [Architecture](ARCHITECTURE.md), `CMakeLists.txt` |
| QML/Qt Quick for the user-facing shell | Rapid iteration of tabs, controls, glass UI, settings and localization | QML must not become the sole security decision maker; object ownership remains important | [Architecture](ARCHITECTURE.md) |
| Native PermissionCoordinator with document context | Binding user intent to an actual pending engine request reduces stale permission risks | Complex frame/profile/navigation lifetime interactions still need hostile integration tests | [Capability contract](CAPABILITY_CONTRACT.md), [Security model](SECURITY_MODEL.md) |
| Eleven always-visible capability controls | Makes capability requests discoverable at the point of use rather than hidden in many menus | Toolbar space, accessibility and narrow-window behavior must be qualified | [Control Bar UX](CONTROL_BAR_UX.md) |
| Explicit distinction between policy/request/grant/block/use | Avoids promising that a green/red icon proves actual hardware behavior | Some observations are unavailable or engine-limited; UI must display uncertainty | [Capability contract](CAPABILITY_CONTRACT.md) |
| No third-party browser extension platform | Reduces privileged third-party code paths and extension supply-chain surface | Users cannot install Chrome/WebExtensions or rely on extension ecosystems | [ADR 0006](ADR/0006-no-third-party-extensions.md) |
| Limited HTTP exception in temporary off-record profile | Supports explicit user-directed legacy access without silently relaxing the normal HTTPS policy | Off-record profile is **not** independent sandbox isolation; network edge cases require proof | [Architecture](ARCHITECTURE.md), [Security model](SECURITY_MODEL.md) |
| Provider-neutral cloud shortcut catalog | Lets users select preferred web services without granting providers new capabilities | A shortcut is not a native integration; authentication and compatibility require separate tests | [ADR 0007](ADR/0007-selectable-cloud-services.md) |
| Evidence-driven release gates | Separates planned tests from reproducible execution and a release claim | Higher verification cost and slower route to public distribution | [Validation](VALIDATION.md), [Release criteria](RELEASE_CRITERIA.md) |

## How to evaluate a decision

For any material change: write down the user goal, alternative designs, security boundary, source-of-truth owner, lifetime behavior, measurable acceptance tests and rollback or failure behavior. Where an ADR exists, preserve the accepted decision unless explicitly superseded.

## Work still open

- Real camera/microphone/screen/location permissions, combined requests and full revoke semantics.
- Runtime DNS/PNA/WebSocket/WebRTC edge cases and actual packaged sandbox behavior.
- Browser reliability, accessibility, security-update channel, signed installers, third-party license/SBOM review and independent security assessment.

These are *known work items*, not assurances that the related security properties already hold.

[Architecture overview](ARCHITECTURE_OVERVIEW.md) · [Roadmap](ROADMAP.md) · [Threat model](THREAT_MODEL.md) · [Validation](VALIDATION.md)
