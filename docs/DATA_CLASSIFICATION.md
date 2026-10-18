# CYBOU — data inventory and protection levels (DRAFT)

**Status: draft proposed by engineering analysis on 2026-10-18. Not reviewed or approved by any named human; levels and requirements below are proposals.** Locations and behaviours are taken from the source (`src/browserstore.h`, `qml/Main.qml`, `qml/BrowserTab.qml`) and from the files and registry keys observed on the development machine. It satisfies the *documentation* half of ASVS 14.1.1 and 14.1.2 only; nothing here is a verified control. See [OWASP_TRIAGE](OWASP_TRIAGE.md).

## Protection levels (proposed)

| Level | Meaning | Required protection (proposed) |
| --- | --- | --- |
| **P0 — public/config** | Preferences and UI state that identify no site or person. | Integrity only; no retention limit. |
| **P1 — browsing metadata** | Where and when the user went: full URLs (may contain tokens in query or fragment), titles, timestamps. | Not logged, not sent anywhere by the product, excluded from private mode, user-deletable, retention bounded, at-rest protection decision pending (currently none). |
| **P2 — site credentials and content** | Cookies, local storage, cache, permission state, anything a site stored on the user's behalf; may include session tokens. | Isolated per profile; destroyed with private profiles; never copied into P1 stores, logs or diagnostics; engine-managed. |
| **P3 — diagnostic** | Logs, crash output, test artifacts. | Must contain no P1/P2 data; bounded retention; tests use separate storage names. |

## Inventory

| Data | Where it lives | Level | Written when | Retention today | Notes / gaps |
| --- | --- | --- | --- | --- | --- |
| History (`history`: url, title, time) | `QSettings` → registry `HKCU\Software\CYBOU\CYBOU` | P1 | Each web (http/https) visit unless the tab is private or restricted (`BrowserTab.qml:197`) | Last 1000 entries, then oldest dropped | **Full URL stored, query and fragment included; plaintext in the registry.** |
| Open-tab session (`session`: url, pinned, muted) | same | P1 | On tab changes unless private mode (`Main.qml:725`) | Overwritten on each save; emptied by the clear-data dialog's session option (`Main.qml:387`) | Full URLs, plaintext. |
| Bookmarks (`bookmarks`: url, title) | same | P1 | User action | Until removed | Full URL; unlimited count. |
| Preferences (`preferences/*`, `ui/controlBar/*`) | same | P0 | User action | Persistent | Includes the opt-in `preferences/strictWebRtc`. `downloadsPath` and `customWallpaperPath` are local paths (mildly identifying). |
| Cloud service choices (`cloud/configuration`) | same | P0/P1 | User action | Persistent | Provider choices and an optional user-supplied Nextcloud base URL; no credentials are stored by the application (OAuth/login stay in the web profile). |
| Engine history and favicon databases (`History`, `Favicons` SQLite files) | Engine profile directory (below) | P1 | By the engine for visited pages | Engine-managed | **Second copy of browsing history** next to the registry one; whether the clear-history action empties it is untested. Observed on the development machine. |
| Cookies, local/session storage, IndexedDB, service workers | Engine profile `Cybou`: `%APPDATA%\CYBOU\CYBOU\QtWebEngine` | P2 | By sites | Engine-managed, persistent profile | Private windows use off-the-record profiles; completeness of clearing is unproven. |
| HTTP cache, GPU/shader caches | `%LOCALAPPDATA%\cybou\CYBOU\cache\QtWebEngine\Cybou` | P2 | By the engine | Engine-managed | Clear-cache action exists in settings; verification missing. |
| Downloaded files | Directory in `preferences/downloadsPath` (default: user Downloads) | P2 | User/accept path | Until deleted by the user | Mark-of-the-Web present on normal downloads; list of download items is in memory only. |
| Temporary HTTP-exception profiles | Off-the-record, one per exact address and tab context | P2 | On explicit user consent | Destroyed on revoke/reload (tested) | |
| Test diagnostics | `build/` (ignored), storage names `CYBOU-Tests`/`CybouSmokeTest` | P3 | Test runs only | Manual | Must not use the product's profile; sandboxed from production by name. |
| Application log | No production log file is written by the application | P3 | — | — | Qt message output goes to the console only; any future log needs the P3 rule. |

## Observations that need an owner decision

1. **P1 stores keep secrets that sites put in URLs.** History, session and bookmarks store the complete URL. A password-reset or OAuth link with a token in the query is persisted in plaintext for up to 1000 visits. Proposed rule: strip fragment and known token-bearing query values, or store origin and path only, for P1 stores.
2. **No at-rest protection.** `QSettings` data is plaintext in the user's registry hive. Any process running as the user can read it. Decide whether OS-level protection (DPAPI) is in scope.
3. **Shared registry location.** The key `HKCU\Software\CYBOU\CYBOU` also contained values unrelated to the browser (names that look like another CYBOU application's wallet and node settings) on the development machine. Whether another product writes there, or the browser's key is shared with it, is **unverified**; if shared, the other application can read browser history and the two can overwrite each other's keys. Needs a distinct organization/application name or key.
4. **Retention is partial.** Only history has a cap; bookmarks and engine data have none, and no scheduled deletion exists. Decide whether "user-controlled" is the intended policy and document it.
5. **Clear-data completeness is unproven** across `QSettings`, the engine profile (including its own `History` database) and caches (ASVS 14.2.7, 14.3.1).

## Verification still needed

Secret-canary test through history, session, bookmarks and diagnostics (ASVS 14.2.1); clear-data and private-window teardown completeness (14.2.7, 14.3.1); inspection of what ends up in the registry versus the engine profile for a given session (14.3.3); then approval of this document by a named reviewer, after which 14.1.1/14.1.2 can move toward verification and 14.2.4 can be checked against it.
