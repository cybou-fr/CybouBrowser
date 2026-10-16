#pragma once
#include "PermissionLease.h"
#include <QMutex>
#include <optional>

// Install only on a profile owned by one view. A new document needs fresh consent.
class TrackerExceptionLease {
public:
    bool start(QUrl target, bool testHttp = false) {
        QMutexLocker lock(&mutex);
        deadline.reset(); consumed = false;
        if ((target.scheme() != "https" && !(testHttp && target.scheme() == "http"))
            || target.host().isEmpty() || !target.userInfo().isEmpty()) return false;
        document = canonical(target); deadline.emplace(60000); return true;
    }
    bool canStart(const QUrl &url) const {
        QMutexLocker lock(&mutex);
        return deadline && !deadline->expired() && !consumed && canonical(url) == document;
    }
    bool consume(const QUrl &url) {
        QMutexLocker lock(&mutex);
        if (!deadline || deadline->expired() || consumed || canonical(url) != document) return false;
        consumed = true; return true;
    }
    bool allows(const QUrl &firstParty) const {
        QMutexLocker lock(&mutex);
        return consumed && deadline && !deadline->expired()
            && SiteContext::origin(firstParty) == SiteContext::origin(document);
    }
    qint64 remainingMs() const { QMutexLocker lock(&mutex); return deadline ? deadline->remainingTime() : 0; }
    void revoke() { QMutexLocker lock(&mutex); deadline.reset(); }
private:
    static QUrl canonical(QUrl url) {
        url = url.adjusted(QUrl::RemoveFragment | QUrl::NormalizePathSegments);
        if (url.path().isEmpty()) url.setPath("/");
        if ((url.scheme() == "https" && url.port() == 443) || (url.scheme() == "http" && url.port() == 80)) url.setPort(-1);
        return url;
    }
    mutable QMutex mutex;
    QUrl document;
    std::optional<PermissionDeadline> deadline;
    bool consumed = false;
};
