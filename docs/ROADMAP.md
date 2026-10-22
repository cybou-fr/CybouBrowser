# CYBOU Browser — Roadmap to 1.0.0

**Current source: 0.4.0-alpha.1 · no public release · security qualification incomplete.** Work takes place on `main`.

## Actual code baseline

| Area | Currently implemented | Missing proof/work for 1.0 |
| --- | --- | --- |
| UI / Alpine Dawn | C++/QML browser, tabs, compact floating new-tab content, glass search, 11 permanent navigation controls, glass popovers and Cloud Glass 2.0 | Keyboard/screen-reader, DPI/resolution, many-tab and crash/recovery matrix |
| Browser tools | History, bookmarks, downloads, PDF export, Find, search providers, private profiles | Data cleanup, reliability and OS integration verification |
| Permissions | Native PermissionCoordinator, per-document decisions and explicit controls | Real capture stop, combined mic/camera, iframe and stale-request adversarial tests |
| Network | Strict HTTPS, temporary HTTP access, NavigationGuard, categorized RuleEngine, tracker exceptions | DNS/PNA/redirect/WebRTC threat tests, list updates/false positives, engine sandbox proof |
| Cloud | Provider-neutral C++ catalog/manager, Proton default and role overrides, Nextcloud HTTPS configuration, web app launcher and local icons | Real login/2FA compatibility, redirects, no-prefetch/permission regression, clarity of product vs web-link shortcuts |
| Test support | `cybou_tests`, 14 native suites, `tools/security/owasp_gate.py`, machine-readable risk cases | Pinned ASVS/WSTG inventory and mappings, evidence and independent pentest |
| Distribution | CMake Windows executable; release-candidate script with signing, CycloneDX SBOM, `/Brepro` reproducibility check, ZIP and IFW installer; online update checked with a local repository ([PACKAGING](PACKAGING.md)) | Trusted (non-self-signed) signing, published update server, rollback, independent review and response process |

No third-party extension platform or partner-granted browser privileges are permitted.

## Source audit — 2026-10-18

This review covers the local Windows implementation and test infrastructure. Implemented behavior is distinct from release qualification. Current milestone remains **0.4.0-alpha.1**; there is no stable release candidate.

| Work item | Code evidence | State / remaining work |
| --- | --- | --- |
| New-tab, tabs and cloud controls | `qml/Main.qml`, `NewTabPage.qml`, `CloudAppLauncher.qml` | Implemented, including 12 px new-tab gap and shared 36 px cloud-button sizing; complete keyboard/DPI/many-tab checks still required |
| Profiles, history, bookmarks, downloads and PDF | `src/browserstore.h`, `qml/BrowserTab.qml`, `tests/test_main.cpp` | Implemented; smoke fixtures exercise normal paths; crash recovery and data-deletion matrix still required |
| Temporary permissions and document context | `src/security/PermissionCoordinator.cpp`, `SiteContext.h`, `PermissionLease.h` | Native request queue, expiry, document/profile checks and revoke paths exist; hardware capture-stop and hostile iframe proof remain open |
| Restricted browsing | `NavigationGuard.h`, `BrowserTab.qml`, `Main.qml` | IDN/port detection, restricted settings, muted audio, permission/download deny and user override exist; IDN decoding regression addressed in this iteration; full navigation/override regression remains open |
| HTTPS and tracker exceptions | `NetworkPolicy.h`, `HttpNavigationLease.h`, `TrackerExceptionRegistry.h`, `TrackerBlocker.h` | Scoped temporary exceptions and local fixtures exist; TLS/DNS/PNA/redirect adversarial coverage remains incomplete |
| Filtering | `src/security/RuleEngine.h` | Small built-in categorized suffix list with protected endpoints; no qualified list-update pipeline or broad effectiveness claim |
| Cloud apps | `src/cloud/`, `qml/CloudSettingsSection.qml` | Local catalog, role resolver, Nextcloud configuration and launcher implemented; real vendor login/2FA and document/media workflows unverified |
| Test evidence | `tests/test_main.cpp`, `tools/security/owasp_gate.py` | 14 suites are wired into the runner (the 2026-10-18 baseline ran the first six); an earlier report had empty suite logs and two failures. Diagnostics capture, empty-log rejection and source-diff identification added; fresh results tracked in VALIDATION |
| Release infrastructure | `CMakeLists.txt`, `tests/security/standards.lock.json` | Windows build exists; official ASVS/WSTG files, hashes and 442 coverage entries are now present; all applicability decisions remain untriaged. installer, signing, SBOM and update pipeline exist locally ([PACKAGING](PACKAGING.md)); R5 is not passed |

### Immediate delivery queue

- [x] **R1 — repeatable baseline (local development, 2026-10-18):** fix IDN failure, wait for the real hover delay, capture nonempty suite logs, record current source/binary identity, obtain six green suites.
- [x] **R2 — risk-based inventory (passed 2026-10-22; R2 aims at a verifiable control system for real browser risks, not at closing every OWASP row; item-by-item N/A approval moved to the 1.0.0-rc gate):** official ASVS 5.0.0 (345) and WSTG 4.2 (97) inputs vendored and SHA-pinned; parsers and exact counts tested. 122 controls now classified applicable with source references and explicit verification gaps; 320 remain untriaged. Remaining: review applicability, assign owners and map cases/evidence. No automatic N/A or fabricated PASS.
- [ ] **R3 — browser-specific negative fixtures (active):** local CORS/preflight/cookie and ws handshake/strict-policy fixtures implemented and passed in security suite; actual temporary HTTP profile ws-denial and live HTTP stream teardown also passed. self-signed HTTPS/WSS rejection, isolated positive WSS message and page-destruction socket teardown also passed. valid/private-CA, expiry, hostname and unknown-CA matrix now also passes with exact errors. the actual temporary-profile UI reload and both revoke paths now close live WSS as well, with a fresh 6/6 baseline. uncooperative WSS close-peer teardown and real HTTP 302 consent boundaries now pass too. 301/302/303/307/308 GET/POST consent-boundary and HTTPS→HTTP downgrade matrices also pass (64 cases). the actual three-hop 302→307 cross-origin UI consent chain now passes with stale-token rejection and profile teardown. Certificate revocation, protocol/cipher, renderer/download stress, mixed TLS/UI chains, runtime DNS/CNAME integration and packaged-profile scenarios remain open. Continue with restricted-mode origin/redirect/override lifecycle, cross-frame permissions, real capture revoke, downloads and local-network boundaries.
- [ ] **R4 — Windows product qualification:** cloud login/2FA, keyboard/FR/RU/DPI, many tabs, crash/session recovery and cleanup.
- [ ] **R5 — distributable candidate:** reproducible installer, licenses/SBOM, signing, safe update/rollback, independent review and matching candidate evidence.

R1 passed locally: Release built, six gate self-tests and six native suites passed with nonempty logs and a saved tested patch. R2 passed on 2026-10-22 as a risk-based framework stage: import/pinning complete, nine applicability batches (122 applicable, 320 N/A proposals awaiting item-specific human approval; the 9 flagged rows were reviewed on 2026-10-22: 3 moved to applicable, 6 kept as conditional proposals) recorded in [OWASP_TRIAGE](OWASP_TRIAGE.md), threat catalog and known-vulnerability gate added, evidence still pending; R3–R5 remain open. This does not authorize a 1.0 claim. See [VALIDATION](VALIDATION.md) for current executed results.

## Gates

| Version | Work to complete | Exit condition |
| --- | --- | --- |
| 0.3.0-alpha.N — passed | Keep implementation and limitations aligned with source | Not a release |
| **0.4.0-alpha.N — CURRENT** | Risk-based R2: pinned ASVS 5.0.0/WSTG 4.2 inventory; browser risk register (`cases.json`) with tier and method; every applicable requirement mapped to an owner and cases; every other requirement carries a recorded N/A proposal with reason; fail-closed gate that rejects unreviewed N/A and unverified requirements | Met on 2026-10-22: 442/442 rows carry a decision or a reasoned proposal (122 applicable, 320 N/A proposals); `owasp_gate.py --mode release` stays BLOCKED until evidence and N/A approvals exist |
| **0.5.0-alpha.N** | Fix origin/iframe/permissions lifecycle, actual media revocation, TLS/PNA/DNS/redirect issues, tracking-rule limits, sandbox process constraints; add fuzz/system/hardware tests | All P0 risk cases implemented and tested; no open unreviewed critical/high |
| **0.6.0-beta.N** | Freeze Windows 1.0 scope; complete cloud compatibility, accessibility, localization, browser lifecycle and long-running stress | Full regression, repeated real-device tests, tested Windows build package |
| **1.0.0-rc.N** | Item-by-item approval of every N/A by a named reviewer, independent security review, dependency/SBOM audit, signed binary/update/rollback, candidate-specific OWASP evidence | Every applicable pinned ASVS/WSTG requirement and browser-specific test verified; all formal release gates GO |
| **1.0.0** | Public Windows x64 product and maintenance process | Exact qualified candidate signed and published |

No estimated date or percentage stands in place of actual completion.

## Current execution order

1. **Testing foundation (P0):** review the 442 imported controls and their applicability, implement deterministic local fixtures and connect each applicable requirement to executed evidence.
2. **Browser security (P0):** renderer and OS boundaries, permissions, WebRTC/media revoke, local/private network, TLS/redirects, download and process abuse, fuzzing and negative tests.
3. **Engine/release supply chain (P0):** Qt/Chromium vulnerability management, actual sandbox, SBOM, signing, update/rollback.
4. **Product quality (P1):** cloud account compatibility, realistic ad/tracker filtering, UX/FR/RU accessibility, keyboard navigation, device and stress regressions.
5. **Release decision (P0):** peer review, external pentest, audit findings, signed candidate, same-SHA results and published support/incident process.

**Only actual tests on the target source/binary may be recorded as PASS.** Missing report, unknown, untested or blocked remains a release blocker.

[TEST_PLAN](TEST_PLAN.md) · [THREAT_MODEL](THREAT_MODEL.md) · [RELEASE_CRITERIA](RELEASE_CRITERIA.md) · [VERSIONING](VERSIONING.md)

## Next network boundary priority

A resolver-alias probe (`--private-network-probe`, see [VALIDATION](VALIDATION.md)) now confirms literal-loopback fetches are denied natively but a hostname resolving to loopback is **not** blocked (known gap, exit 3). Source review found no production caller feeding resolved WebEngine addresses into `checkDnsRebind()`; the helper is currently exercised by unit tests only. Next R3 work must probe actual renderer/network-service behavior against controlled DNS/CNAME/private-address fixtures and determine the enforceable runtime boundary. Do not promote URL classification or a separate preflight DNS lookup into a DNS-rebinding PASS. See [THREAT_MODEL](THREAT_MODEL.md#runtime-dns-integration-gap--source-review-2026-10-18).

Engine experiment (`--engine-pna-probe`): Chromium's `BlockInsecurePrivateNetworkRequests` blocks the hostname→loopback alias at connection level for insecure public-space sources. Enabled in production with a compatibility suite (`pna`, gate now 7 suites). HTTPS public sources are now covered by engine preflight enforcement (`PrivateNetworkAccessSendPreflights`/`RespectPreflightResults`, also enabled). Redirects public->local are engine-blocked; **WebSocket public->local is an open engine gap** (logged as known gap in `pna`). The app interceptor stops literal `ws://` and, since the best-effort WebSocket resolver, hostname aliases (stub-tested; not connection-bound). Dedicated workers inherit the block (tested). **WebRTC is an open gap:** injected remote ICE candidates make the browser send STUN checks to loopback and `WebRTCPublicInterfacesOnly` does not prevent it (`--webrtc-probe`); only `--force-webrtc-ip-handling-policy=disable_non_proxied_udp` stops it, at the price of all direct UDP, and it is the "WebRTC strict" setting, **on by default** (owner decision; the user can turn it off, takes effect after restart), with its own `webrtc` gate suite (see [VALIDATION](VALIDATION.md)). Next: connected-address enforcement for WebSocket, real DNS/CNAME/changing-answer tests, WebRTC, shared/service workers. See [VALIDATION](VALIDATION.md).
