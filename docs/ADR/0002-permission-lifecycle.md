# ADR 0002: temporary permission lifecycle

Both Qt profiles use AskEveryTime; controller queryPermission cannot grant. Initial coordinator lives in QML: one pending object, top/requester/profile/view checks, 15-second timeout. View timer limits document lifetime to 60 seconds after last grant and navigates to about:blank. Navigation/close destroys the granted document. Independent C++ per-request leases and epoch-bound transactions remain planned. Grant is not evidence of device activity.
