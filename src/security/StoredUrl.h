#pragma once
#include <QString>
#include <QStringList>
#include <QUrl>

// URLs written to persistent browsing stores (history, bookmarks) must not keep secrets that sites
// put in them: fragments (implicit OAuth: #access_token=...), embedded credentials, and the values
// of well-known secret-bearing query parameters. Heuristic by name, so it closes the common leaks
// (OAuth, password reset, signed links) and is not a guarantee. The open-tab session keeps the
// full URL on purpose: restoring a page needs it. See docs/DATA_CLASSIFICATION.md.
namespace StoredUrl {

inline bool isSensitiveParameter(const QString &name) {
    // Compare ignoring case, dashes and underscores: access_token, Access-Token and accessToken match.
    const QString key = name.toLower().remove(QLatin1Char('-')).remove(QLatin1Char('_'));
    static const QStringList exact{
        "code", "key", "sig", "sid", "pin", "otp", "auth", "jwt", "pwd", "ticket", "session", "bearer",
        "assertion", "verifier", "authcode", "samlresponse", "samlrequest", "accesskey", "apikey"};
    if (exact.contains(key)) return true;
    static const QStringList parts{"token", "password", "passwd", "secret", "signature", "credential", "sessionid", "authkey"};
    for (const auto &part : parts) if (key.contains(part)) return true;
    return false;
}

// Returns the input unchanged (same string) when there is nothing to remove, so ordinary URLs keep
// their exact stored form.
inline QString sanitize(const QString &input) {
    QUrl url(input, QUrl::TolerantMode);
    if (!url.isValid() || url.scheme().isEmpty()) return input;
    bool changed = false;
    if (url.hasFragment()) { url.setFragment(QString()); changed = true; }
    if (!url.userInfo().isEmpty()) { url.setUserInfo(QString()); changed = true; }
    if (url.hasQuery()) {
        QStringList rebuilt;
        for (const QString &pair : url.query(QUrl::FullyEncoded).split(QLatin1Char('&'))) {
            const int eq = pair.indexOf(QLatin1Char('='));
            if (eq > 0 && isSensitiveParameter(QUrl::fromPercentEncoding(pair.left(eq).toUtf8()))) {
                rebuilt << pair.left(eq) + QStringLiteral("=redacted");
                changed = true;
            } else {
                rebuilt << pair;
            }
        }
        if (changed) url.setQuery(rebuilt.join(QLatin1Char('&')), QUrl::StrictMode);
    }
    return changed ? url.toString() : input;
}

} // namespace StoredUrl
