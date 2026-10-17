#pragma once
#include <QUrl>
#include <QString>
#include <QStringList>
#include <QtWebEngineCore/QWebEnginePermission>

enum class PermissionDecision {
    Ask = 0,
    Deny = 2,
    Allow = 1
};

class EffectivePermissionPolicy {
public:
    struct Evaluation {
        PermissionDecision decision = PermissionDecision::Ask;
        bool deniedByTop = false;
        bool deniedByRequester = false;
        bool isCrossOrigin = false;
        QString key;
        QString topOrigin;
        QString requesterOrigin;
        QString reason;
    };

    static QString normalizeOrigin(const QUrl &url) {
        if (!url.isValid() || url.scheme().isEmpty() || url.host().isEmpty()) return {};
        const QString scheme = url.scheme().toLower();
        const QString host = url.host().toLower();
        const int port = url.port();
        if (port == -1 || (scheme == QLatin1String("https") && port == 443)
            || (scheme == QLatin1String("http") && port == 80)) {
            return QStringLiteral("%1://%2").arg(scheme, host);
        }
        return QStringLiteral("%1://%2:%3").arg(scheme, host).arg(port);
    }

    static Evaluation evaluate(const QString &key,
                               const QUrl &topUrl,
                               const QUrl &requesterUrl,
                               int topPolicyMode,
                               int requesterPolicyMode) {
        Evaluation eval;
        eval.key = key;
        eval.topOrigin = normalizeOrigin(topUrl);
        eval.requesterOrigin = normalizeOrigin(requesterUrl);
        eval.isCrossOrigin = !eval.topOrigin.isEmpty() && !eval.requesterOrigin.isEmpty()
                             && (eval.topOrigin != eval.requesterOrigin);

        // Screen sharing is permanently unsupported/denied across all origins
        if (key == QStringLiteral("screen")) {
            eval.decision = PermissionDecision::Deny;
            eval.reason = QStringLiteral("Screen capture is permanently disabled.");
            return eval;
        }

        // Top origin explicit deny (mode 2) cascades to all embedded iframes
        if (topPolicyMode == 2) {
            eval.deniedByTop = true;
            eval.decision = PermissionDecision::Deny;
            eval.reason = QStringLiteral("Denied by top-level origin policy.");
            return eval;
        }

        // Requester origin explicit deny (mode 2) blocks this specific frame
        if (requesterPolicyMode == 2) {
            eval.deniedByRequester = true;
            eval.decision = PermissionDecision::Deny;
            eval.reason = QStringLiteral("Denied by requester origin policy.");
            return eval;
        }

        eval.decision = PermissionDecision::Ask;
        return eval;
    }

    static Evaluation evaluateCompound(const QStringList &keys,
                                       const QUrl &topUrl,
                                       const QUrl &requesterUrl,
                                       const std::function<int(const QString &key, const QUrl &origin)> &getPolicyMode) {
        Evaluation netEval;
        netEval.topOrigin = normalizeOrigin(topUrl);
        netEval.requesterOrigin = normalizeOrigin(requesterUrl);
        netEval.isCrossOrigin = !netEval.topOrigin.isEmpty() && !netEval.requesterOrigin.isEmpty()
                                && (netEval.topOrigin != netEval.requesterOrigin);
        netEval.decision = PermissionDecision::Ask;

        for (const auto &k : keys) {
            const int topMode = getPolicyMode(k, topUrl);
            const int reqMode = getPolicyMode(k, requesterUrl);
            const auto singleEval = evaluate(k, topUrl, requesterUrl, topMode, reqMode);
            if (singleEval.decision == PermissionDecision::Deny) {
                netEval.decision = PermissionDecision::Deny;
                netEval.key = k;
                netEval.deniedByTop |= singleEval.deniedByTop;
                netEval.deniedByRequester |= singleEval.deniedByRequester;
                netEval.reason = singleEval.reason;
                return netEval;
            }
        }
        return netEval;
    }
};
