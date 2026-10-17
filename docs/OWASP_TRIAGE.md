# OWASP applicability review — first browser boundary batch

Date: 2026-10-18. Source analysis and planned verification only; no control is declared verified by this review. Scope is the Windows desktop browser, including native policy, embedded renderer and local files. Web-server terminology is explicitly adapted to that boundary; this is not an assessment of websites opened by users.

Of 442 pinned controls, 20 are classified applicable and 422 remain untriaged. No N/A decisions are approved. Owner `CYBOU security maintainer (role)` is a responsibility role, not evidence that a named human has accepted the work. Independent review and named ownership remain pending.

The machine-readable source of truth is `tests/security/coverage.json`. It records source references, rationale, case mappings, verification status and remaining work for each selected control. Existing tests provide partial implementation evidence; none of the broad release cases has been promoted to verified on the strength of a suite exit alone.

| Control | Browser scope | Open verification work |
| --- | --- | --- |
| ASVS:v5.0.0-5.3.2 | Untrusted download names and generated PDF paths cross the browser-to-filesystem boundary. | Add traversal, Windows reserved-name, symlink/race and download filename fixtures; extension allowlisting alone does not prove safe content. |
| ASVS:v5.0.0-8.1.1 | Origin, capability and document permissions need documented function/resource access rules. | Review contract against every capability and requesting iframe; existing source is not a completed documentation review. |
| ASVS:v5.0.0-8.1.3 | Profile, origin, tab, document epoch and request deadlines are contextual authorization attributes. | Trace each attribute from native request to decision and revocation, including navigation and background tabs. |
| ASVS:v5.0.0-8.2.1 | Native capability access must require the requesting site explicit authorization. | Complete hostile iframe and independent top/requester deny fixtures; do not infer coverage from one native permission test. |
| ASVS:v5.0.0-8.3.1 | Trusted native policy must enforce access despite hostile page JavaScript. | Exercise attempted JS bypass, clipboard, downloads and native request paths; trusted-service principle is adapted to the browser process. |
| ASVS:v5.0.0-8.3.2 | Permission changes must take effect in active documents and media sessions. | Real combined camera/microphone/screen capture-stop evidence is missing; a UI policy change is insufficient. |
| ASVS:v5.0.0-8.3.3 | A frame must not borrow a more privileged top-level or browser identity. | Run cross-origin frame and intermediary request tests using requester and top-level origin independently. |
| ASVS:v5.0.0-12.1.1 | The embedded TLS client protocol policy affects all HTTPS navigation. | Measure accepted/rejected TLS versions against local endpoints on the packaged engine; the source does not establish TLS configuration. |
| ASVS:v5.0.0-12.1.2 | Embedded engine cipher negotiation is within the product transport boundary. | Add controlled cipher negotiation tests and document the supported assurance level and engine policy. |
| ASVS:v5.0.0-12.2.1 | Browser client connections use strict HTTPS by default. | The explicit temporary HTTP exception conflicts with an unconditional all-TLS interpretation; review product scope and exception policy, not a PASS waiver. |
| ASVS:v5.0.0-12.3.2 | The browser is a TLS client and must reject invalid server certificates. | Add expired, hostname-mismatch, untrusted and redirect-chain TLS fixtures; static rejection handler alone is insufficient. |
| ASVS:v5.0.0-16.2.5 | Network diagnostics may encounter credentials, query tokens and browsing data. | Audit production and opt-in diagnostics separately; add secret-canary tests and retention/privacy checks. |
| ASVS:v5.0.0-16.4.1 | Hostile URLs and filenames can cross into diagnostic text. | Add CR/LF, control-character and structured-log injection fixtures; existing plain-text logs are not qualified. |
| ASVS:v5.0.0-16.5.3 | Permission and transport errors must not grant access or change a deny into allow. | Fault-inject disk/resource errors and interrupted permission flows; successful normal-path suites are insufficient. |
| WSTG:v4.2-CRYP-01 | Assess TLS from the embedded browser client boundary, not an owned web server. | Controlled TLS protocol/cipher/certificate endpoint matrix remains missing. |
| WSTG:v4.2-CRYP-03 | Check cleartext transport and sensitive data crossing temporary HTTP contexts. | Verify no unintended downgrade or secret transfer, including redirects and WebSocket resources. |
| WSTG:v4.2-CLNT-04 | Untrusted redirects must preserve transport, origin and restricted browsing boundaries. | Add script/meta/HTTP redirect chains and IDN/trailing-dot variants; parser assertions do not prove navigation behavior. |
| WSTG:v4.2-CLNT-07 | Client cross-origin fetches must preserve browser origin and local-network isolation. | PNA and CORS are distinct: add dedicated CORS/preflight/credential tests as well as DNS/redirect fixtures. |
| WSTG:v4.2-CLNT-10 | WebSocket connections must obey transport and private-network policy. | Add actual ws/wss handshake and local-network/redirect tests; URL classification alone is insufficient. |
| WSTG:v4.2-CLNT-12 | Storage must remain isolated across origins, normal/private profiles and temporary contexts. | Test cookies, localStorage, IndexedDB, service workers, deletion and crash/restart; private-window existence is only partial proof. |

## Findings that affect the next implementation work

1. **CORS is not PNA.** Added planned case `CYB-ORIGIN-001` for real preflight, response-read and credential isolation fixtures. Local-network address classification does not qualify CORS.
2. **WebSocket policy needs a real handshake fixture.** Added planned case `CYB-WS-001` for ws/wss transport, temporary context, origin and private-network revocation behavior.
3. **Hardware revoke is still open.** The permissions coordinator and policy UI cannot alone prove camera/microphone/screen recording has stopped.
4. **Temporary HTTP versus unconditional TLS requirements needs scope review.** Do not convert the explicit product exception into a silent waiver or a passing all-TLS assertion.
5. **File extensions do not validate content.** Download policy and path helpers need traversal, reserved Windows filename and symlink/race checks; antivirus and content-type claims are not established by the current allowlist.

## Evidence-gate repairs in this batch

Both browser-case evidence and applicable-control evidence now use the same validation: a nonempty readable log, matching SHA-256, PASS result, known source commit and nonempty matching candidate binary hash. Duplicate or unknown evidence case IDs block the gate. This closes the prior inconsistent paths where a correct hash of an empty log or a duplicate record could be accepted. Twelve self-tests passed, including valid evidence, tampering, empty/missing logs, stale source/binary identities and duplicates.

`build/owasp-triage/report.json` is a local diagnostic report. Import and mapping counts are correct, but the gate intentionally reports BLOCKED because reviewed applicability and candidate-specific verification are incomplete. No browser source changed in this batch; the earlier native baseline remains historical evidence for its recorded source/binary, not a new candidate qualification.
