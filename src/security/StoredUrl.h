#pragma once
#include <QRegularExpression>
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
// their exact stored form. A string QUrl cannot parse is cut at the first '?' or '#': its query and
// fragment cannot be inspected, so they are not kept.
inline QString sanitize(const QString &input) {
    QUrl url(input, QUrl::TolerantMode);
    if (!url.isValid()) {
        const int cut = input.indexOf(QRegularExpression(QStringLiteral("[?#]")));
        return cut < 0 ? input : input.left(cut);
    }
    if (url.scheme().isEmpty()) return input;
    bool changed = false;
    if (url.hasFragment()) { url.setFragment(QString()); changed = true; }
    if (!url.userInfo().isEmpty()) { url.setUserInfo(QString()); changed = true; }
    if (url.hasQuery()) {
        // Parameters are separated by '&' and, in older servers, ';'. Separators are kept as written.
        const QString query = url.query(QUrl::FullyEncoded);
        QString rebuilt, pair;
        bool redacted = false;
        auto flush = [&](QChar separator) {
            const int eq = pair.indexOf(QLatin1Char('='));
            // A secret can also sit in a nested query inside a parameter (continue=/a?token=X, also
            // percent-encoded), so every "name=value" piece of the decoded parameter is checked.
            bool secret = false;
            const QStringList nested = QUrl::fromPercentEncoding(pair.toUtf8()).split(QRegularExpression(QStringLiteral("[?#&;]")));
            for (const QString &piece : nested) {
                const int e = piece.indexOf(QLatin1Char('='));
                if (e > 0 && isSensitiveParameter(piece.left(e))) { secret = true; break; }
            }
            if (secret) { rebuilt += eq > 0 ? pair.left(eq) + QStringLiteral("=redacted") : QStringLiteral("redacted"); redacted = true; }
            else rebuilt += pair;
            if (!separator.isNull()) rebuilt += separator;
            pair.clear();
        };
        for (const QChar ch : query) {
            if (ch == QLatin1Char('&') || ch == QLatin1Char(';')) flush(ch); else pair += ch;
        }
        flush(QChar());
        if (redacted) { url.setQuery(rebuilt, QUrl::StrictMode); changed = true; }
    }
    return changed ? url.toString() : input;
}

} // namespace StoredUrl
