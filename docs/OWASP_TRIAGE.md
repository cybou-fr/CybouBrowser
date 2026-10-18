# OWASP applicability review — first browser boundary batch

Date: 2026-10-18. Source analysis and planned verification only; no control is declared verified by this review. Scope is the Windows desktop browser, including native policy, embedded renderer and local files. Web-server terminology is explicitly adapted to that boundary; this is not an assessment of websites opened by users.

Of 442 pinned controls, 119 are classified applicable and 323 remain untriaged (ninth batch below). No N/A decisions are approved. Owner `CYBOU security maintainer (role)` is a responsibility role, not evidence that a named human has accepted the work. Independent review and named ownership remain pending.

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

## Sixth batch - Encoding, Sanitization, Validation and Business Logic (ASVS chapters 1 and 2)

Date: 2026-10-18. 43 untriaged ASVS controls were reviewed; **19 are classified applicable** (adapted to the browser's own parsing, native UI, OS interaction and consent flows), 24 are proposed not-applicable. Applicable controls now total 79 of 442.

Two real defects came from this review and were fixed with tests: the host `127.0.0.1.` (IP literal with a trailing dot) reached loopback in the engine but was not classified private by `NavigationGuard` (ASVS 1.5.3, found by the new differential suite `url-parser`), and `stopMediaTracks` built JavaScript by concatenating a value into a quoted literal (ASVS 1.2.3, now `JSON.stringify` plus a static guard). See [VALIDATION](VALIDATION.md).

Applicable: ASVS 1.1.1, 1.1.2, 1.2.2, 1.2.3, 1.2.5, 1.3.2, 1.3.6, 1.3.10, 1.3.12, 1.4.1, 1.4.2, 1.4.3, 1.5.2, 1.5.3, 2.1.1, 2.2.1, 2.2.2, 2.3.1, 2.4.1. Per-control rationale, case mapping and remaining work are in `tests/security/coverage.json`; none is verified.

### Proposed not-applicable (NOT approved)

Injection classes and business-transaction mechanics that the application does not contain (no SQL/LDAP/XPath/LaTeX/JNDI/memcache/mail use, no CSV export, no XML parsing of its own, no server-side templates, no multi-user approvals, locking or transactions): ASVS 1.2.1, 1.2.4, 1.2.6, 1.2.7, 1.2.8, 1.2.9, 1.2.10, 1.3.1, 1.3.3, 1.3.4, 1.3.5, 1.3.7, 1.3.8, 1.3.9, 1.3.11, 1.5.1, 2.1.2, 2.1.3, 2.2.3, 2.3.2, 2.3.3, 2.3.4, 2.3.5, 2.4.2. 1.2.1 and 1.3.3 are generic restatements covered by 1.1.2 and the UI text work; 1.2.9 (regular-expression escaping) should be re-checked if user input is ever compiled into a regex. They remain `untriaged`; no N/A is recorded.

## Seventh batch - Session Management, Cryptography and Secure Coding (ASVS chapters 7, 11 and 15)

Date: 2026-10-18. 64 untriaged ASVS controls were reviewed; **19 are classified applicable**, 45 are proposed not-applicable. Applicable controls now total 98 of 442.

Facts the classification rests on, from the source: the application contains **no cryptographic code, random-number generation or hashing of its own** (all cryptography is the engine's); consent "tokens" are counters of the form `tab-N@epoch`, used only as staleness guards, and no bridge (WebChannel, postMessage) lets a page reach or present them, so no non-guessable secret exists to protect; and QML uses strict equality everywhere (a new lint enforces it).

| Control | Browser scope (adapted) | Open verification work |
| --- | --- | --- |
| ASVS 7.4.1, 7.6.2 | Ending a browsing session must stop it being usable; site sessions must not start without user action (tab restore on start is on by default). | Teardown completeness; restore-on-start decision. |
| ASVS 11.1.2, 11.2.1, 11.2.3 | Cryptography inventory (engine TLS only), validated implementations (BoringSSL via the engine), 128-bit minimum via TLS policy. | Inventory document; measure accepted TLS versions and ciphers. |
| ASVS 15.1.1-15.1.5 | Documentation of remediation time frames, SBOM, resource-heavy and dangerous functionality, risky components (the engine). | Write them; no SBOM exists. |
| ASVS 15.2.1-15.2.5 | Engine currency (Chromium 140 base), defences against availability loss, test harness shipped in production, dependency provenance, extra protection around dangerous functionality. | Define time frames and compare; stress cases; provenance of Qt. |
| ASVS 15.3.5 | Strict equality in QML: lint added, 0 findings. | Keep in the self-test set. |
| ASVS 15.4.1-15.4.3 | Shared state between the interceptor thread and UI; check-then-act on downloaded files (`canOpenDownloadedFile`); lock discipline. | Thread-sanitizer run, race fixture, lock review. |

### Proposed not-applicable (NOT approved)

Server-side session, key-management and web-framework mechanics that the product does not implement: ASVS 7.1.1, 7.1.2, 7.1.3, 7.2.1, 7.2.2, 7.2.3, 7.2.4, 7.3.1, 7.3.2, 7.4.2, 7.4.3, 7.4.4, 7.4.5, 7.5.1, 7.5.2, 7.5.3, 7.6.1 (account and session administration; the browser does not issue or manage application sessions); 11.1.1, 11.1.3, 11.1.4, 11.2.2, 11.2.4, 11.2.5, 11.3.1, 11.3.2, 11.3.3, 11.3.4, 11.3.5, 11.4.1, 11.4.2, 11.4.3, 11.4.4, 11.5.1, 11.5.2, 11.6.1, 11.6.2, 11.7.1, 11.7.2 (no application-level encryption, MACs, password storage, key derivation, nonces or random numbers - all cryptography is inherited from the engine, covered through 12.1.x and 11.2.x; 11.5.1 is classified this way because the only identifiers are non-secret counters that no page can reach, and must be revisited if any capability token is ever exposed to page content; 11.1.4 and 11.6.2 touch post-quantum migration and key exchange, which are engine decisions to be recorded in the cryptographic inventory); 15.3.1, 15.3.2, 15.3.3, 15.3.4, 15.3.6, 15.3.7, 15.4.4 (data-object field filtering, mass assignment, proxy IP forwarding, prototype pollution, HTTP parameter pollution, backend redirect following, thread starvation). They remain `untriaged`; no N/A is recorded.

## Eighth batch - WSTG 4.2 scenarios

Date: 2026-10-18. The 91 untriaged WSTG scenarios were reviewed. WSTG is a procedure for testing a *web application served to browsers*; CYBOU is the browser, so most scenarios have no counterpart. **19 are classified applicable** where the browser exposes an equivalent behaviour or where CYBOU's own native UI plays the role of the application. Applicable controls now total 117 of 442 (ASVS plus WSTG); 72 WSTG scenarios are proposed not-applicable.

Applicable: WSTG CONF-05, CONF-07, CONF-09, ATHN-01, ATHN-06, ATHZ-01, SESS-02, INPV-11, INPV-19, ERRH-01, ERRH-02, BUSL-05, BUSL-06, BUSL-07, BUSL-08, BUSL-09, CLNT-03, CLNT-06, CLNT-09 (six more were classified earlier: CRYP-01, CRYP-03, CLNT-04, CLNT-07, CLNT-10, CLNT-12). Rationale, case mapping and remaining work per scenario are in `tests/security/coverage.json`; none is verified.

### Proposed not-applicable (NOT approved)

Server, account, session-server, injection-against-an-application and hosting scenarios: WSTG INFO-01, INFO-02, INFO-03, INFO-04, INFO-05, INFO-06, INFO-07, INFO-08, INFO-09, INFO-10, CONF-01, CONF-02, CONF-03, CONF-04, CONF-06, CONF-08, CONF-10, CONF-11, IDNT-01, IDNT-02, IDNT-03, IDNT-04, IDNT-05, ATHN-02, ATHN-03, ATHN-04, ATHN-05, ATHN-07, ATHN-08, ATHN-09, ATHN-10, ATHZ-02, ATHZ-03, ATHZ-04, SESS-01, SESS-03, SESS-04, SESS-05, SESS-06, SESS-07, SESS-08, SESS-09, INPV-01, INPV-02, INPV-03, INPV-04, INPV-05, INPV-06, INPV-07, INPV-08, INPV-09, INPV-10, INPV-12, INPV-13, INPV-14, INPV-15, INPV-16, INPV-17, INPV-18, CRYP-02, CRYP-04, BUSL-01, BUSL-02, BUSL-03, BUSL-04, CLNT-01, CLNT-02, CLNT-05, CLNT-08, CLNT-11, CLNT-13, APIT-01. Several have a browser angle that is already tracked under another control rather than as a separate scenario: INPV-12 and INPV-13 (command and format-string injection - ASVS 1.2.5 and 1.3.10), CLNT-11 (web messaging - the product has no bridge and the engine enforces origins), SESS-06 (logout - ASVS 7.4.1/14.3.1), SESS-05 (CSRF - SameSite is engine enforcement), INFO-05 (information in page content), CONF-04 (backup files - ASVS 13.4.1). They remain `untriaged`; no N/A is recorded.

## Ninth batch - remaining chapters, and machine-readable proposals

Date: 2026-10-19. The remaining domains (Authentication, OAuth/OIDC, API and web service, Authorization, Self-contained tokens) were reviewed. Only two controls have a browser counterpart and are now **applicable**: ASVS 4.1.2 (HTTP-to-HTTPS upgrade: CYBOU's HTTPS-First) and ASVS 4.4.1 (all WebSockets over TLS). Applicable controls now total 119 of 442.

Every other untriaged row has been reviewed, and `tests/security/coverage.json` now records the engineering proposal on the row itself: `proposedDisposition: not_applicable`, a chapter-level `proposalRationale` and `proposalStatus`. These fields are **not read by the gate and are not an approval**: all 323 rows stay `untriaged` until a named reviewer records a reviewed disposition with a reason. 9 rows are marked `needs explicit review` with a note on why the proposal is not routine (for example ASVS 3.4.5 referrer policy, 5.2.4 download quotas, 12.2.2, 13.4.3, 16.4.3, 1.2.9, 11.1.4, 11.5.1, 11.6.2). The per-batch lists above remain as the narrative record; this file is the authoritative count.

| Chapter | Proposed not-applicable rows |
| --- | --- |
| ASVS 1 | 16 |
| ASVS 2 | 8 |
| ASVS 3 | 27 |
| ASVS 4 | 14 |
| ASVS 5 | 5 |
| ASVS 6 | 47 |
| ASVS 7 | 17 |
| ASVS 8 | 7 |
| ASVS 9 | 7 |
| ASVS 10 | 36 |
| ASVS 11 | 21 |
| ASVS 12 | 4 |
| ASVS 13 | 16 |
| ASVS 14 | 5 |
| ASVS 15 | 7 |
| ASVS 16 | 4 |
| ASVS 17 | 10 |
| WSTG | 72 |
