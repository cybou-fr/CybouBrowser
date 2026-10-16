# ADR 0003: transport and isolation

Strict HTTPS defaults on for new settings; existing explicit preferences still apply. Until request-scoped transport exceptions exist, strict mode fails closed with no HTTP opt-in. Legacy fallback when strict is off is a known gap. Tests explicitly opt out inside their separate test organization/profile. An application proxy cannot establish per-tab isolation. Future isolation requires an independent process and route/DNS/WebRTC leak tests; no production isolation claim is made.
