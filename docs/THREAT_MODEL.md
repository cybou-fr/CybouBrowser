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
