# OWASP applicability review — first browser boundary batch

Date: 2026-10-18. Source analysis and planned verification only; no control is declared verified by this review. Scope is the Windows desktop browser, including native policy, embedded renderer and local files. Web-server terminology is explicitly adapted to that boundary; this is not an assessment of websites opened by users.

Of 442 pinned controls, 60 are classified applicable and 382 remain untriaged (fifth batch below; ASVS 3.7.3 was added after the fourth, see VALIDATION). No N/A decisions are approved. Owner `CYBOU security maintainer (role)` is a responsibility role, not evidence that a named human has accepted the work. Independent review and named ownership remain pending.

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

## Follow-up: first real network fixture (2026-10-18)

The initial CORS and ws subset is now implemented in `src/security/NetworkBoundaryTests.h` and passed in the Release security suite. The two cases moved from planned to in_progress with covered and remaining scenario lists; control verification status remains not_verified. See [VALIDATION](VALIDATION.md) for scope and evidence. WSS, production temporary-profile behavior, public-to-private DNS routing, cross-site cookies and already-open socket revocation are still open.

## Second batch — TLS and WebRTC (Secure Communication, WebRTC)

Date: 2026-10-18. The 20 controls of these two ASVS domains were reviewed; **6 are classified applicable** in `tests/security/coverage.json` (owner role, case mapping, source references, `not_verified`, remaining work). Nothing is verified: the inventory gate reports "missing current-revision PASS evidence" for each, as it should.

| Control | Browser scope (adapted) | Open verification work |
| --- | --- | --- |
| ASVS:v5.0.0-12.1.3 | Server may request a TLS client certificate; the source has no selection handler, so the engine default decides. | mTLS fixture: with and without an installed certificate; add explicit prompt/deny policy if the default selects silently. |
| ASVS:v5.0.0-12.1.4 | Revocation checking is done by the embedded engine. | Document engine behaviour (OCSP stapling, CRLSets, soft-fail); revoked-certificate fixture. |
| ASVS:v5.0.0-12.1.5 | ECH protects SNI of visited sites; engine capability. | Determine support/default in Chromium 140 and record the decision. |
| ASVS:v5.0.0-12.3.1 | All outbound connections use TLS without silent fallback. | Inventory the app's own outbound connections; the single-navigation HTTP exception is a documented deviation. |
| ASVS:v5.0.0-17.1.1 | TURN wording is server-side; the browser analogue is WebRTC/ICE not reaching loopback or private ranges (`webRTCPublicInterfacesOnly` is set). | Real RTCPeerConnection ICE probe with the setting on and off; none exists. |
| ASVS:v5.0.0-17.2.8 | DTLS fingerprint vs SDP check is engine behaviour. | Confirm for the pinned engine; mismatching-fingerprint fixture if feasible. |

### Proposed not-applicable (NOT approved)

These 14 describe server-side infrastructure CYBOU does not operate. They stay `untriaged` until a named human reviewer accepts the rationale; no N/A is recorded in the coverage file: ASVS 12.2.2 (publicly trusted certificates for externally facing services; revisit when an update server exists), 12.3.3, 12.3.4, 12.3.5 (internal service-to-service TLS and authentication; no internal services), 17.1.2 (TURN exhaustion), 17.2.1, 17.2.2 (media-server DTLS key management and ciphers; the browser uses engine-generated ephemeral DTLS), 17.2.3, 17.2.4, 17.2.5, 17.2.6, 17.2.7 (media-server SRTP/DTLS handling) and 17.3.1, 17.3.2 (signaling-server flood and malformed input).

## Third batch - Web Frontend Security (ASVS chapter 3)

Date: 2026-10-18. The 31 untriaged controls of this chapter describe the security of a *web application served to browsers*; CYBOU is the browser, so most are server-side. **3 are classified applicable** (adapted) in `coverage.json`; 28 are proposed not-applicable and need a named reviewer.

| Control | Browser scope (adapted) | Open verification work |
| --- | --- | --- |
| ASVS:v5.0.0-3.2.2 | Untrusted titles, URLs, file names and origins shown in native QML UI must render as plain text. A real defect was found and fixed (see VALIDATION); case CYB-UI-004. | End-to-end hostile-title UI test; text outside QML; Qt default coverage. |
| ASVS:v5.0.0-3.4.1 | The engine must honour HSTS for later http:// navigations and the HTTP exception must not bypass it. | HSTS upgrade fixture incl. lease and off-the-record persistence. |
| ASVS:v5.0.0-3.7.1 | CYBOU's own client-side stack: embedded Chromium 140 + patch level, no legacy plugins. | Engine age/cadence record, CVE triage, plugin absence. |

### Proposed not-applicable (NOT approved)

Server-set headers, cookies and hosting practices that CYBOU does not serve; the browser merely enforces some of them through the engine, and enforcement is covered by existing cases (CORS, transport, redirects) rather than by these controls: ASVS 3.1.1, 3.2.1, 3.2.3, 3.3.1, 3.3.2, 3.3.3, 3.3.4, 3.3.5, 3.4.2, 3.4.3, 3.4.4, 3.4.5, 3.4.6, 3.4.7, 3.4.8, 3.5.1, 3.5.2, 3.5.3, 3.5.4, 3.5.5, 3.5.6, 3.5.7, 3.5.8, 3.6.1, 3.7.2, 3.7.4, 3.7.5. 3.4.5 (browser default referrer policy is a CYBOU privacy decision) deserves explicit reviewer attention before N/A. ASVS 3.7.3 was removed from this list and classified applicable after the external-protocol suite was written. The rest remain `untriaged`; no N/A is recorded.

## Fourth batch - File Handling and Data Protection

Date: 2026-10-18. 25 untriaged ASVS controls (chapters 5 and 14) were reviewed; **15 are classified applicable** (adapted to downloads, the wallpaper file picker and the application's own stores), 10 are proposed not-applicable. Applicable controls now total 44 of 442.

| Control | Browser scope (adapted) | Open verification work |
| --- | --- | --- |
| ASVS 5.1.1 | Permitted file types and how files are made safe, documented for wallpaper and downloads. | One document; none exists. |
| ASVS 5.2.1, 5.2.2, 5.2.6 | Wallpaper images are checked by extension only; no size, content or pixel limit in the application. | Oversize/pixel-flood/mismatched-content fixtures. |
| ASVS 5.4.1, 5.4.2 | Content-Disposition names are attacker-controlled. Engine fixture passes; a real policy defect (reserved device names) was fixed. | Trailing dot/space anomaly, collisions, long/Unicode names, app accept path. |
| ASVS 5.4.3 | Antivirus analogue: Mark-of-the-Web so the OS can scan. Present on normal downloads, missing in one anomaly. | ZoneId content, PDF export, non-NTFS. |
| ASVS 14.1.1, 14.1.2, 14.2.4 | No data inventory, protection levels or retention requirements exist. | Write them, then verify. |
| ASVS 14.2.1 | Sensitive URL parts must not persist in history, session, samples or logs. | Secret-canary test. |
| ASVS 14.2.3 | Data must not flow to trackers. | Filter effectiveness measurement. |
| ASVS 14.2.7, 14.3.1 | Retention and clearing of site data and private sessions. | Completeness and Clear-Site-Data fixtures. |
| ASVS 14.3.3 | QSettings stores history and session in plaintext in the user profile. | Decide stripping and at-rest protection. |

### Proposed not-applicable (NOT approved)

Server-side or upload-processing requirements that CYBOU does not implement: ASVS 5.2.3 (compressed-file limits; the browser does not unpack), 5.2.4 (per-user quotas), 5.2.5 (symlinks in archives), 5.3.1 (server-side execution of uploads), 5.3.3 (zip slip in server processing), 14.2.2 and 14.2.5 (server/CDN caches), 14.2.6 (minimum data returned by an API), 14.2.8 (metadata stripping of user-submitted files), 14.3.2 (anti-caching headers set by a server). 5.2.4 may deserve review as a disk-fill concern for downloads. They stay `untriaged`; no N/A is recorded.

## Fifth batch - Configuration, Logging and Error Handling

Date: 2026-10-18. 35 untriaged ASVS controls (chapters 13 and 16) were reviewed; **15 are classified applicable** (adapted), 20 are proposed not-applicable. Applicable controls now total 60 of 442.

| Control | Browser scope (adapted) | Open verification work |
| --- | --- | --- |
| ASVS 13.1.1, 13.2.4 | The application's own outbound communications (search providers, cloud catalog, user-supplied Nextcloud address) need an inventory and allowlist. | Write the inventory, capture idle and active traffic. |
| ASVS 13.4.1 | A package must not ship `.git`, logs or private test keys. | Package-content check once an installer exists. |
| ASVS 13.4.2 | No debug/test mode in production; test-harness QML ships in the module and the engine honours `QTWEBENGINE_REMOTE_DEBUGGING`/`QTWEBENGINE_CHROMIUM_FLAGS`. | Verify on the packaged binary and document honoured variables. |
| ASVS 13.4.6 | The default user agent reveals engine and Qt versions to sites. | Record and decide. |
| ASVS 16.1.1, 16.2.1, 16.2.2, 16.2.3 | There is no production log file; events live in a bounded in-memory sample. History times are local without offset. No telemetry intended, unproven. | Logging inventory, UTC/offset times, traffic capture. |
| ASVS 16.3.2, 16.3.3, 16.3.4 | Blocked requests carry reasons; certificate errors are rejected but not visibly recorded. | Event list, TLS failure visibility. |
| ASVS 16.5.1, 16.5.2, 16.5.4 | User-visible errors must not leak internals; controls must fail closed (known fail-open: WebSocket resolver); no last-resort handler beyond exit on QML load failure. | Fault injection, crash policy. |

### Proposed not-applicable (NOT approved)

Back-end infrastructure and server hosting requirements that a desktop browser does not have, or log pipelines the product intentionally lacks (no central log processor, no log transmission by design): ASVS 13.1.2, 13.1.3, 13.1.4, 13.2.1, 13.2.2, 13.2.3, 13.2.5, 13.2.6, 13.3.1, 13.3.2, 13.3.3, 13.3.4, 13.4.3, 13.4.4, 13.4.5, 13.4.7, 16.2.4, 16.3.1, 16.4.2, 16.4.3. 16.4.3 (no remote log shipping) is a deliberate privacy choice rather than a gap; 13.4.3 (directory listings) does touch the engine's file: listings but those are a browsing feature. They remain `untriaged`; no N/A is recorded.
