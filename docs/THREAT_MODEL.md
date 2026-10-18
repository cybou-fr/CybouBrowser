# CYBOU — threat model and security boundaries

**Status:** initial systematic threat inventory for `0.3.0-alpha.1`; security sign-off has **not** occurred. Revise on every architecture change.

## Valuable assets

Browser executable/integrity; OS execution boundaries; user credentials/session cookies/tokens; saved bookmarks/history/downloads/settings; camera/mic/screen content; location/clipboard; origin and frame permissions; private-profile separation; local network; cloud-provider launch targets; trusted update channel and Qt/Chromium binaries.

## Trust boundaries and attacker control

| Boundary | Untrusted input | Mandatory negative tests |
| --- | --- | --- |
| Renderer/site -> privileged QML/C++ | JS/HTML/URLs, redirects, file requests, permission callbacks, IPC | Unexpected input cannot acquire elevated permissions, bypass top/requester origin, or reach privileged objects |
| Top-level page -> iframe | Cross-origin and sandboxed frames, delayed/stale permission | Top deny inherited, requester deny distinct, stale IDs never grant |
| Profile -> tab | Shared interceptor and temporary exceptions, private profiles | No false per-tab network attribution, no unintended policy inheritance |
| Internet -> local machine | Private IP, DNS rebinding, CNAME, IPv6, link-local, redirects | Site cannot read intranet or localhost without supported user-driven navigation |
| Browser -> OS / device | Media capture, download/open, shell calls, clipboard | Explicit consent, revoke actually terminates capture, no code execution through path/URI abuse |
| WebEngine -> updater/dependencies | Qt/Chromium CVEs, sandbox bypass, deployment DLLs | Sandbox enabled and verified, binary provenance/SBOM, patch cadence, rollback/signatures |
| User -> cloud portal | Provider ID, custom Nextcloud URL, SSO redirects | No injected scripts, no privileged exceptions, no non-HTTPS launch, no prefetch and no silent token sharing |
| Test tooling -> real profile | Fixtures/logs/credentials | Temporary independent test directories, synthetic accounts, redaction, no real user data |

## OWASP risk coverage, not a promise of compliance

Top 10:2025 [A01 access control, A02 misconfiguration, A03 supply chain, A04 cryptography, A05 injection, A06 insecure design, A07 authentication, A08 integrity, A09 logging, A10 exceptions](https://top10.owasp.org/2025/) must each have reviewed tests and negative cases. For all official ASVS 5.0.0 and WSTG v4.2 items, record applicable or justified reviewed not applicable in [SECURITY_VERIFICATION_FRAMEWORK](SECURITY_VERIFICATION_FRAMEWORK.md). Browser-specific risk cases in `tests/security/cases.json` are extra, not replacement for the official inventories.

## Not presently demonstrated

Renderer sandbox and site/process isolation on packaged Windows binary; real camera/screen revoke across complex media tracks; DNS/WebRTC leak behavior; update signing/rollback; comprehensive external penetration test; login flows of all cloud vendors; multi-day soak. Existing suites may cover subsets but do not constitute evidence for all these claims.

Keep the ban on third-party extensions (ADR 0006) and provider-neutral cloud choice (ADR 0007). A commercial partner is **never** an exception to the permission or network model.

## Runtime DNS integration gap — source review 2026-10-18

`NavigationGuard::checkDnsRebind()` / `DnsRebindGuard::recordResolution()` currently have test callers only. `RequestObserver` invokes `blocksPrivateNetworkAccess()` against request URLs/first-party URLs, but no reviewed production call feeds actual WebEngine DNS answers or the connected address into the rebinding helper. Thus literal/private-domain classification and synthetic resolution-unit tests do not establish runtime protection against CNAME-to-private targets or a hostname changing addresses.

This is an implementation/verification gap, not a demonstrated exploit or a claim that Chromium provides no protection. Next work must probe the actual renderer/network service with controlled local DNS/resolver fixtures and inspect engine enforcement. An unrelated application-side DNS lookup would not bind the browser's later connection and must not be counted as a complete fix. Keep CYB-PNA-001/002 and the relevant release gates open until the actual connection boundary is verified.

### Probe result — 2026-10-18

The `--private-network-probe` diagnostic demonstrated the gap empirically for the loopback case: a public-looking hostname mapped to 127.0.0.1 by test-only resolver rules was reached by a page fetch, while literal loopback forms were denied. Fix must bind enforcement to the connected address (engine/network-service level), not URL classification. See [VALIDATION](VALIDATION.md).
