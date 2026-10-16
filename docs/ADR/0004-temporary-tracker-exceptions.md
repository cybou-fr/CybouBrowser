# ADR 0004: temporary tracker exceptions

A shared profile interceptor cannot reliably attribute a request to a tab. Never disable tracker filtering on that shared profile for a tab-scoped action.

Explicit consent recreates one loaded HTTPS URL in a fresh off-the-record AskEveryTime profile. Ordinary cookies/cache are not copied. A mutex-protected native lease consumes the exact main-frame URL once and permits tracker-list matches only when firstParty origin matches that document. HTTPS strict and the other profile policies still apply. The exception does not combine with an HTTP exception.

The native lease lasts at most 60 seconds and checks monotonic and wall-clock deadlines. A separate native timer tears down the view/profile on expiry, including in a background tab. Reload, another main-frame navigation, manual revoke and tab closure also end the exception. Page-controlled navigations are not replayed with ordinary cookies. Press/release actions carry tab UID/navigation epoch tokens. The panel shows remaining seconds; UI refresh never extends the lease.

Network events retain sanitized origin/firstParty, type, time, blocked reason and explicit tracker-exception decisions. Labels and categories describe filter outcomes, not successful downloads or total page activity. Shared-profile counts remain profile-scoped. The fixed suffix list is still a limited filter, not a full ad-block engine. Profile data separation is not process/proxy isolation.

The dedicated --tracker-test harness alone permits a loopback HTTP fixture in a temporary tracker profile and maps the tracker test hostname to loopback before WebEngine startup. Production rejects this HTTP path. The fixture observes actual blocked and successful requests without contacting a public tracker or ignoring TLS errors.
