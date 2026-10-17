#pragma once
#include "SiteContext.h"
#include "RuleEngine.h"
#include <QDateTime>
#include <QHash>
#include <QMutex>
#include <QObject>
#include <optional>
#include <cstdint>

// Temporary granular network exceptions for ONE profile, keyed by top-level origin.
//
// QtWebEngine 6 request interceptors carry no tab identity: firstPartyUrl is the
// visible URL of whichever WebContents issued the request (see
// proxying_url_loader_factory_qt.cpp), and the per-page interceptor is not
// exposed to QML. The enforceable scope on a shared profile is therefore
// "this origin in this profile". Every tab showing the origin shares the
// exception, and the UI must say so (ADR 0005).
//
// Granular capability (S1.2):
// Allows independent temporary exceptions for Trackers and Ads on the same origin.
class TrackerExceptionRegistry {
public:
    enum ExceptionType : uint8_t {
        ExceptionNone = 0x0,
        ExceptionTrackers = 0x1,
        ExceptionAds = 0x2,
        ExceptionAll = ExceptionTrackers | ExceptionAds
    };

    static constexpr qsizetype MaxEntries = 16;
    struct Entry {
        QUrl origin;
        const void *owner = nullptr;
        qint64 grantedAt = 0;
        int allowedTypes = ExceptionNone;
    };

    static bool eligible(const QUrl &document, bool testHttp = false) {
        return (document.scheme() == QLatin1String("https") || (testHttp && document.scheme() == QLatin1String("http")))
            && !document.host().isEmpty() && document.userInfo().isEmpty();
    }

    // Grants or updates exceptions for the given origin.
    bool grant(const QUrl &document, QObject *owner, int types = ExceptionTrackers, bool testHttp = false) {
        if (!owner || !eligible(document, testHttp) || types == ExceptionNone) return false;
        const QUrl origin = SiteContext::origin(document);
        if (origin.isEmpty()) return false;
        QMutexLocker lock(&m_mutex);
        const QString key = origin.toString();
        auto it = m_entries.find(key);
        if (it != m_entries.end()) {
            it->allowedTypes |= types;
            return true;
        }
        if (m_entries.size() >= MaxEntries) return false;
        m_entries.insert(key, {origin, owner, QDateTime::currentMSecsSinceEpoch(), types});
        QObject::connect(owner, &QObject::destroyed, [this, owner]() {
            releaseOwner(owner);
        });
        return true;
    }

    bool allows(const QUrl &firstParty, int requiredType = ExceptionTrackers) const {
        const QString key = SiteContext::origin(firstParty).toString();
        if (key.isEmpty()) return false;
        QMutexLocker lock(&m_mutex);
        const auto it = m_entries.constFind(key);
        if (it == m_entries.constEnd()) return false;
        return (it->allowedTypes & requiredType) != 0;
    }

    bool allowsTrackers(const QUrl &firstParty) const {
        return allows(firstParty, ExceptionTrackers);
    }

    bool allowsAds(const QUrl &firstParty) const {
        return allows(firstParty, ExceptionAds);
    }

    int allowedTypesFor(const QUrl &firstParty) const {
        const QString key = SiteContext::origin(firstParty).toString();
        if (key.isEmpty()) return ExceptionNone;
        QMutexLocker lock(&m_mutex);
        const auto it = m_entries.constFind(key);
        return (it != m_entries.constEnd()) ? it->allowedTypes : ExceptionNone;
    }

    std::optional<Entry> entryFor(const QUrl &url) const {
        const QString key = SiteContext::origin(url).toString();
        if (key.isEmpty()) return std::nullopt;
        QMutexLocker lock(&m_mutex);
        const auto it = m_entries.constFind(key);
        if (it == m_entries.constEnd()) return std::nullopt;
        return *it;
    }

    // Revokes specific exception types or all types if ExceptionAll
    bool revoke(const QUrl &url, int types = ExceptionAll) {
        const QString key = SiteContext::origin(url).toString();
        if (key.isEmpty()) return false;
        QMutexLocker lock(&m_mutex);
        auto it = m_entries.find(key);
        if (it == m_entries.end()) return false;
        it->allowedTypes &= ~types;
        if (it->allowedTypes == ExceptionNone || types == ExceptionAll) {
            m_entries.erase(it);
        }
        return true;
    }

    // The owner keeps only an exception for the origin it is now on; everything else it owns ends.
    qsizetype retainForOwner(const void *owner, const QUrl &currentUrl) {
        const QString keep = SiteContext::origin(currentUrl).toString();
        QMutexLocker lock(&m_mutex);
        return m_entries.removeIf([&](Entries::iterator item) {
            return item->owner == owner && (keep.isEmpty() || item.key() != keep);
        });
    }

    qsizetype releaseOwner(const void *owner) { return retainForOwner(owner, QUrl()); }
    qsizetype size() const { QMutexLocker lock(&m_mutex); return m_entries.size(); }
    void clear() { QMutexLocker lock(&m_mutex); m_entries.clear(); }

private:
    using Entries = QHash<QString, Entry>;
    mutable QMutex m_mutex;
    Entries m_entries;
};
