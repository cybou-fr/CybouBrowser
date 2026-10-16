#pragma once
#include "SiteContext.h"
#include <QMutex>

// Install only on a profile owned by one view. A new document needs fresh consent.
class TrackerExceptionLease {
public:
    bool start(QUrl target, bool testHttp = false) {
        QMutexLocker lock(&mutex);
        active = false; consumed = false;
        if ((target.scheme() != "https" && !(testHttp && target.scheme() == "http"))
            || target.host().isEmpty() || !target.userInfo().isEmpty()) return false;
        document = canonical(target); active = true; return true;
    }
    bool canStart(const QUrl &url) const {
        QMutexLocker lock(&mutex);
        return active && !consumed && canonical(url) == document;
    }
    bool consume(const QUrl &url) {
        QMutexLocker lock(&mutex);
        if (!active || consumed || canonical(url) != document) return false;
        consumed = true; return true;
    }
    bool allows(const QUrl &firstParty) const {
        QMutexLocker lock(&mutex);
        return consumed && active
            && SiteContext::origin(firstParty) == SiteContext::origin(document);
    }
    void revoke() { QMutexLocker lock(&mutex); active = false; }
private:
    static QUrl canonical(QUrl url) {
        url = url.adjusted(QUrl::RemoveFragment | QUrl::NormalizePathSegments);
        if (url.path().isEmpty()) url.setPath("/");
        if ((url.scheme() == "https" && url.port() == 443) || (url.scheme() == "http" && url.port() == 80)) url.setPort(-1);
        return url;
    }
    mutable QMutex mutex;
    QUrl document;
    bool active = false;
    bool consumed = false;
};
