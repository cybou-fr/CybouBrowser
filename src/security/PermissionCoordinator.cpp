#include "PermissionCoordinator.h"
#include <QVariant>
#include <QDebug>
#include <QCoreApplication>
#include <QtWebEngineQuick/QQuickWebEngineProfile>
#include <limits>

using Type = QWebEnginePermission::PermissionType;
namespace {
constexpr qint64 RequestTimeoutMs = 15000;
constexpr qsizetype MaxLeases = 64;
constexpr qsizetype MaxQueuedRequests = 16;
bool transientProfile(QObject *object) {
    const auto profile = qobject_cast<QQuickWebEngineProfile *>(object);
    return profile && profile->persistentPermissionsPolicy() == QQuickWebEngineProfile::PersistentPermissionsPolicy::AskEveryTime;
}
}

PermissionCoordinator::PermissionCoordinator(QObject *parent) : QObject(parent) {
    m_timer.setSingleShot(true);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &PermissionCoordinator::expire);
}

PermissionCoordinator::~PermissionCoordinator() {
    if (m_pending && m_pending->permission.isValid()) m_pending->permission.deny();
    for (auto &req : m_queue) {
        if (req.permission.isValid()) req.permission.deny();
    }
    m_queue.clear();
}

QStringList PermissionCoordinator::keysFor(Type type) {
    switch (type) {
    case Type::MediaVideoCapture: return {"camera"};
    case Type::MediaAudioCapture: return {"mic"};
    case Type::MediaAudioVideoCapture: return {"camera", "mic"};
    case Type::Geolocation: return {"location"};
    case Type::Notifications: return {"notifications"};
    case Type::ClipboardReadWrite: return {"clipboard"};
    default: return {};
    }
}

void PermissionCoordinator::notifyChanged() {
    ++m_revision;
    emit changed();
}

void PermissionCoordinator::setActiveView(QObject *view) {
    if (m_activeView == view) return;
    m_activeView = view;
    emit activeViewChanged();
    if (!view) return;
    if (!m_pending || m_pending->context.view != view) {
        for (int i = 0; i < m_queue.size(); ++i) {
            if (m_queue[i].context.view == view) {
                auto target = m_queue.takeAt(i);
                if (m_pending) {
                    m_queue.prepend(*m_pending);
                    m_pending.reset();
                }
                m_pending = target;
                notifyChanged();
                schedule();
                break;
            }
        }
    }
}

void PermissionCoordinator::promoteNext() {
    if (m_pending) return;
    while (!m_queue.isEmpty()) {
        int pick = 0;
        if (m_activeView) {
            for (int i = 0; i < m_queue.size(); ++i) {
                if (m_queue[i].context.view == m_activeView) {
                    pick = i;
                    break;
                }
            }
        }
        auto candidate = m_queue.takeAt(pick);
        if (candidate.deadline.expired()
            || !candidate.context.view || !candidate.context.profile
            || candidate.context.view->property("navigationEpoch").toULongLong() != candidate.context.navigationEpoch) {
            if (candidate.permission.isValid()) candidate.permission.deny();
            emit decisionFinished(candidate.permission, candidate.context.tabUid, false, candidate.id);
            continue;
        }
        m_pending = candidate;
        break;
    }
}

void PermissionCoordinator::cancelMatching(const std::function<bool(const Pending &)> &predicate) {
    // 1. Purge matching items from m_queue first so promoteNext will never see them
    for (int i = m_queue.size() - 1; i >= 0; --i) {
        if (predicate(m_queue[i])) {
            auto req = m_queue.takeAt(i);
            if (req.permission.isValid()) req.permission.deny();
            emit decisionFinished(req.permission, req.context.tabUid, false, req.id);
        }
    }
    // 2. Purge active m_pending if matching
    bool pendingPurged = false;
    if (m_pending && predicate(*m_pending)) {
        auto req = *m_pending;
        m_pending.reset();
        if (req.permission.isValid()) req.permission.deny();
        emit decisionFinished(req.permission, req.context.tabUid, false, req.id);
        pendingPurged = true;
    }
    // 3. Promote only if m_pending was purged
    if (pendingPurged) {
        promoteNext();
    }
    notifyChanged();
    schedule();
}

void PermissionCoordinator::watch(QObject *object) {
    if (!object || m_watched.contains(object)) return;
    m_watched.insert(object);
    connect(object, &QObject::destroyed, this, [this, object] {
        m_watched.remove(object);
        if (m_activeView == object) m_activeView = nullptr;
        cancelMatching([](const Pending &p) {
            return !p.context.view || !p.context.profile;
        });
        m_leases.removeIf([](const PermissionLease &lease) { return !lease.context.view || !lease.context.profile; });
        notifyChanged(); schedule();
    });
}

bool PermissionCoordinator::submit(const QWebEnginePermission &permission, QObject *view, QObject *profile,
                                   bool foreground, bool policyDenied) {
    expire(); // Check pending request deadline even if the GUI timer was delayed.
    if (!permission.isValid()) return false;
    SiteContext context{view, profile, view ? view->property("tabUid").toString() : QString(),
                        view ? SiteContext::origin(view->property("url").toUrl()) : QUrl(),
                        SiteContext::origin(permission.origin()),
                        view ? view->property("navigationEpoch").toULongLong() : 0};
    const qsizetype totalPending = (m_pending ? 1 : 0) + m_queue.size();
    const bool blocked = policyDenied || totalPending >= MaxQueuedRequests
        || !view || !transientProfile(profile)
        || view->property("profile").value<QObject *>() != profile
        || context.tabUid.isEmpty() || context.topOrigin.isEmpty() || context.requestingOrigin.isEmpty()
        || keysFor(permission.permissionType()).isEmpty() || m_leases.size() >= MaxLeases;
    const quint64 newReqId = ++m_nextRequestId;
    emit requestObserved(permission, context.tabUid, context.topOrigin, blocked, newReqId);
    if (blocked) { permission.deny(); return false; }
    watch(view); watch(profile);
    const Pending newReq{newReqId, permission, context, PermissionDeadline(RequestTimeoutMs)};
    if (foreground) {
        m_activeView = view;
        if (!m_pending) {
            m_pending = newReq;
        } else if (m_pending->context.view != view) {
            m_queue.prepend(*m_pending);
            m_pending = newReq;
        } else {
            m_queue.append(newReq);
        }
    } else {
        if (!m_pending && (!m_activeView || m_activeView == view)) {
            m_pending = newReq;
        } else {
            m_queue.append(newReq);
        }
    }
    notifyChanged(); schedule(); return true;
}

bool PermissionCoordinator::resolve(bool allow, QObject *activeView, QObject *activeProfile, quint64 requestId) {
    Pending target;
    bool found = false;
    bool wasPending = false;
    if (m_pending && m_pending->id == requestId) {
        target = *m_pending;
        m_pending.reset();
        found = true;
        wasPending = true;
    } else {
        for (int i = 0; i < m_queue.size(); ++i) {
            if (m_queue[i].id == requestId) {
                target = m_queue.takeAt(i);
                found = true;
                break;
            }
        }
    }
    if (!found) return false;

    const bool granted = allow && !target.deadline.expired() && target.permission.isValid()
        && target.context.matches(activeView, activeProfile)
        && transientProfile(activeProfile)
        && SiteContext::origin(target.permission.origin()) == target.context.requestingOrigin
        && m_leases.size() < MaxLeases;
    if (granted) {
        m_leases.append({target.id, target.context, keysFor(target.permission.permissionType())});
        target.permission.grant();
    } else if (target.permission.isValid()) {
        target.permission.deny();
    }
    emit decisionFinished(target.permission, target.context.tabUid, granted, target.id);
    if (wasPending) {
        promoteNext();
    }
    notifyChanged(); schedule(); return granted;
}

void PermissionCoordinator::cancel() {
    if (m_pending) resolve(false, nullptr, nullptr, m_pending->id);
}

void PermissionCoordinator::forgetView(QObject *view) {
    if (m_activeView == view) m_activeView = nullptr;
    cancelMatching([view](const Pending &p) {
        return p.context.view == view || !p.context.view;
    });
    m_leases.removeIf([view](const PermissionLease &lease) { return lease.context.view == view; });
    notifyChanged(); schedule();
}

void PermissionCoordinator::documentLoaded(QObject *view, quint64 epoch) {
    cancelMatching([view, epoch](const Pending &p) {
        return p.context.view == view && p.context.navigationEpoch != epoch;
    });
    const auto removed = m_leases.removeIf([=](const PermissionLease &lease) {
        return lease.context.view == view && lease.context.navigationEpoch != epoch;
    });
    if (removed) { notifyChanged(); schedule(); }
}

QStringList PermissionCoordinator::grantedKeys(QObject *view, quint64 epoch) const {
    QStringList result;
    for (const auto &lease : m_leases) if (lease.context.view == view && lease.context.navigationEpoch == epoch
                                         && lease.context.profile) result.append(lease.keys);
    result.removeDuplicates(); return result;
}

bool PermissionCoordinator::hasPendingFor(const QString &tab) const {
    if (m_pending && m_pending->context.tabUid == tab) return true;
    for (const auto &item : m_queue) {
        if (item.context.tabUid == tab) return true;
    }
    return false;
}

void PermissionCoordinator::expire() {
    cancelMatching([](const Pending &p) {
        return p.deadline.expired() || (!p.permission.isValid() && p.id == 0);
    });
}

void PermissionCoordinator::revokeAll() {
    cancelMatching([](const Pending &) { return true; });
    QList<QPointer<QObject>> views;
    for (const auto &lease : m_leases) if (lease.context.view && !views.contains(lease.context.view)) views.append(lease.context.view);
    m_leases.clear(); notifyChanged(); schedule();
    for (const auto &view : views) if (view) emit revokeDocument(view);
}

void PermissionCoordinator::schedule() {
    qint64 minRemaining = -1;
    if (m_pending) {
        minRemaining = m_pending->deadline.remainingTime();
    }
    for (const auto &item : m_queue) {
        const qint64 r = item.deadline.remainingTime();
        if (minRemaining < 0 || r < minRemaining) minRemaining = r;
    }
    if (minRemaining < 0) {
        m_timer.stop();
    } else {
        m_timer.start(int(std::clamp<qint64>(minRemaining, 1, std::numeric_limits<int>::max())));
    }
}

bool PermissionCoordinator::enqueueTestRequest(quint64 id, QObject *view, QObject *profile, const QString &tabUid,
                                              const QUrl &topOrigin, quint64 epoch, qint64 timeoutMs) {
    if (!view || !profile) return false;
    watch(view); watch(profile);
    SiteContext context{view, profile, tabUid, SiteContext::origin(topOrigin), SiteContext::origin(topOrigin), epoch};
    Pending req{id, QWebEnginePermission(), context, PermissionDeadline(timeoutMs)};
    if (m_activeView && m_activeView == view && !m_pending) {
        m_pending = req;
    } else if (!m_pending) {
        m_pending = req;
    } else if (m_activeView && m_activeView == view && m_pending->context.view != view) {
        m_queue.prepend(*m_pending);
        m_pending = req;
    } else {
        m_queue.append(req);
    }
    notifyChanged();
    schedule();
    return true;
}
