#pragma once
#include "SiteContext.h"
#include <QMutex>

// One profile belongs to one temporary HTTP view. Never install this on a shared profile.
class HttpNavigationLease {
public:
    bool start(const QUrl &url) {
        QMutexLocker lock(&m_mutex);
        m_active = false;
        m_consumed = false;
        if (url.scheme() != "http" || url.host().isEmpty() || !url.userInfo().isEmpty()) return false;
        m_target = canonical(url);
        m_active = true;
        return true;
    }
    bool canStart(const QUrl &url) const {
        QMutexLocker lock(&m_mutex);
        return m_active && !m_consumed && canonical(url) == m_target;
    }
    bool allows(const QUrl &url, const QUrl &firstParty, bool mainFrame) {
        QMutexLocker lock(&m_mutex);
        if (!m_active || url.scheme() != "http" || !url.userInfo().isEmpty()) return false;
        if (mainFrame) {
            if (m_consumed || canonical(url) != m_target) return false;
            m_consumed = true;
            return true;
        }
        return m_consumed && SiteContext::origin(firstParty) == SiteContext::origin(m_target)
            && SiteContext::origin(url) == SiteContext::origin(m_target);
    }
    void revoke() { QMutexLocker lock(&m_mutex); m_active = false; }
private:
    static QUrl canonical(QUrl url) {
        url = url.adjusted(QUrl::RemoveFragment | QUrl::NormalizePathSegments);
        if (url.path().isEmpty()) url.setPath("/");
        if (url.port() == 80) url.setPort(-1);
        return url;
    }
    mutable QMutex m_mutex;
    QUrl m_target;
    bool m_active = false;
    bool m_consumed = false;
};
