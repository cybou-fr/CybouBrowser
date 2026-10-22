# Security policy

## Status and supported versions

CYBOU Browser is currently **0.3.0-alpha.2 source under development**. It has **no publicly released, independently security-qualified version**. This repository's presence on GitHub and its Apache-2.0 license do not imply that it is safe for sensitive everyday browsing.

| Version | Security-supported release? |
| --- | --- |
| 0.3.0-alpha.2 and other development builds | No |
| 1.0.0 | Not released / subject to explicit gates |

See [current validation](docs/VALIDATION.md) and [release criteria](docs/RELEASE_CRITERIA.md).

## Reporting a vulnerability

**Do not publish a vulnerability, exploit, credentials, sensitive browsing information or a working proof of concept in a public GitHub issue.**

If GitHub's **Report a vulnerability** / private vulnerability reporting is enabled for this repository, use that channel. If it is not available, open a **public issue requesting a private security contact without including vulnerability details**. Wait for a private channel before sharing technical details.

Please include the affected source version/commit, operating system, reproduction steps (privately), potential impact and whether third-party data could be exposed. Do not test against third-party systems without permission.

No response-time SLA, bounty program, independent penetration-test coverage or coordinated disclosure deadline is promised. Security reports will be triaged based on evidence and impact.

## Areas that deserve particular care

WebEngine/Chromium sandboxing and updates; navigation and origin boundaries; iframe/redirect permissions; real microphone/camera/screen capture revocation; WebRTC/DNS/PNA/WebSocket; TLS and certificate handling; downloads and local file access; browser storage; cloud-provider authentication.

## Evidence policy

Only executable, reproducible checks tied to the candidate source and runtime can support a tested claim. Do not treat an ADR, theoretical threat analysis or unit-test fixture as proof of end-to-end security. The full release security gate remains open.
