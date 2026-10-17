#pragma once
#include "SiteContext.h"
#include <QDateTime>
#include <QHash>
#include <QMutex>
#include <QObject>
#include <QPointer>
#include <optional>

// Temporary tracker exceptions for ONE profile, keyed by top-level origin.
//
// QtWebEngine 6 request interceptors carry no tab identity: firstPartyUrl is the
// visible URL of whichever WebContents issued the request (see
// proxying_url_loader_factory_qt.cpp), and the per-page interceptor is not
// exposed to QML. The enforceable scope on a shared profile is therefore
// "this origin in this profile". Every tab showing the origin shares the
// exception, and the UI must say so (ADR 0005).
//
// Each entry has an owner view. The exception ends when the owner leaves the
// origin or is destroyed, or when anyone revokes it. Nothing is persisted.
class TrackerExceptionRegistry {
public:
    static constexpr qsizetype MaxEntries = 16;
    struct Entry { QUrl origin; QPointer<QObject> owner; qint64 grantedAt = 0; };

    static bool eligible(const QUrl &document, bool testHttp = false) {
        return (document.scheme() == QLatin1String("https") || (testHttp && document.scheme() == QLatin1String("http")))
            && !document.host().isEmpty() && document.userInfo().isEmpty();
    }
    // Returns true when the origin is covered afterwards. An existing entry keeps its first owner.
    bool grant(const QUrl &document, QObject *owner, bool testHttp = false) {
        if (!owner || !eligible(document, testHttp)) return false;
        const QUrl origin = SiteContext::origin(document);
        if (origin.isEmpty()) return false;
        QMutexLocker lock(&m_mutex);
        dropOrphansLocked();
        const QString key = origin.toString();
        if (m_entries.contains(key)) return true;
        if (m_entries.size() >= MaxEntries) return false;
        m_entries.insert(key, {origin, owner, QDateTime::currentMSecsSinceEpoch()});
        return true;
    }
    bool allows(const QUrl &firstParty) const { return entryFor(firstParty).has_value(); }
    std::optional<Entry> entryFor(const QUrl &url) const {
        const QString key = SiteContext::origin(url).toString();
        if (key.isEmpty()) return std::nullopt;
        QMutexLocker lock(&m_mutex);
        const auto it = m_entries.constFind(key);
        if (it == m_entries.constEnd() || !it->owner) return std::nullopt;
        return *it;
    }
    bool revoke(const QUrl &url) {
        const QString key = SiteContext::origin(url).toString();
        if (key.isEmpty()) return false;
        QMutexLocker lock(&m_mutex);
        return m_entries.remove(key);
    }
    // The owner keeps only an exception for the origin it is now on; everything else it owns ends.
    qsizetype retainForOwner(const QObject *owner, const QUrl &currentUrl) {
        const QString keep = SiteContext::origin(currentUrl).toString();
        QMutexLocker lock(&m_mutex);
        return m_entries.removeIf([&](Entries::iterator item) {
            return !item->owner || (item->owner == owner && (keep.isEmpty() || item.key() != keep));
        });
    }
    qsizetype releaseOwner(const QObject *owner) { return retainForOwner(owner, QUrl()); }
    qsizetype dropOrphans() { QMutexLocker lock(&m_mutex); return dropOrphansLocked(); }
    qsizetype size() const { QMutexLocker lock(&m_mutex); return m_entries.size(); }
    void clear() { QMutexLocker lock(&m_mutex); m_entries.clear(); }
private:
    using Entries = QHash<QString, Entry>;
    qsizetype dropOrphansLocked() { return m_entries.removeIf([](Entries::iterator item) { return !item->owner; }); }
    mutable QMutex m_mutex;
    Entries m_entries;
};
