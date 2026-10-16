#include "PermissionCoordinator.h"
#include <QVariant>
#include <QDebug>
#include <QCoreApplication>
#include <QtWebEngineQuick/QQuickWebEngineProfile>
#include <limits>

using Type = QWebEnginePermission::PermissionType;
namespace {
constexpr qint64 RequestTimeoutMs = 15000;
constexpr qint64 LeaseDurationMs = 60000;
constexpr qsizetype MaxLeases = 64;
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
void PermissionCoordinator::notifyChanged() { ++m_revision; emit changed(); }
void PermissionCoordinator::watch(QObject *object) {
    if (!object || m_watched.contains(object)) return;
    m_watched.insert(object);
    connect(object, &QObject::destroyed, this, [this, object] {
        m_watched.remove(object);
        if (m_pending && (!m_pending->context.view || !m_pending->context.profile)) cancel();
        m_leases.removeIf([](const PermissionLease &lease) { return !lease.context.view || !lease.context.profile; });
        notifyChanged(); schedule();
    });
}
bool PermissionCoordinator::submit(const QWebEnginePermission &permission, QObject *view, QObject *profile,
                                   bool foreground, bool policyDenied) {
    expire(); // Check absolute deadlines even if the GUI timer was delayed.
    if (!permission.isValid()) return false;
    SiteContext context{view, profile, view ? view->property("tabUid").toString() : QString(),
                        view ? SiteContext::origin(view->property("url").toUrl()) : QUrl(),
                        SiteContext::origin(permission.origin()),
                        view ? view->property("navigationEpoch").toULongLong() : 0};
    const bool blocked = !foreground || policyDenied || m_pending || !view || !transientProfile(profile)
        || view->property("profile").value<QObject *>() != profile
        || context.tabUid.isEmpty() || context.topOrigin.isEmpty() || context.requestingOrigin.isEmpty()
        || keysFor(permission.permissionType()).isEmpty() || m_leases.size() >= MaxLeases;
    emit requestObserved(permission, context.tabUid, context.topOrigin, blocked);
    if (blocked) { permission.deny(); return false; }
    watch(view); watch(profile);
    m_pending = Pending{++m_nextRequestId, permission, context, PermissionDeadline(RequestTimeoutMs)};
    notifyChanged(); schedule(); return true;
}
bool PermissionCoordinator::resolve(bool allow, QObject *activeView, QObject *activeProfile, quint64 requestId) {
    if (!m_pending || m_pending->id != requestId) return false;
    const auto pending = *m_pending;
    m_pending.reset();
    const bool granted = allow && !pending.deadline.expired() && pending.permission.isValid()
        && pending.context.matches(activeView, activeProfile)
        && transientProfile(activeProfile)
        && SiteContext::origin(pending.permission.origin()) == pending.context.requestingOrigin
        && m_leases.size() < MaxLeases;
    if (granted) {
        // One independent deadline per concrete request; later grants never renew earlier ones.
        m_leases.append({pending.id, pending.context, keysFor(pending.permission.permissionType()), PermissionDeadline(LeaseDurationMs)});
        pending.permission.grant();
    } else if (pending.permission.isValid()) pending.permission.deny();
    emit decisionFinished(pending.permission, pending.context.tabUid, granted);
    notifyChanged(); schedule(); return granted;
}
void PermissionCoordinator::cancel() { if (m_pending) resolve(false, nullptr, nullptr, m_pending->id); }
void PermissionCoordinator::forgetView(QObject *view) {
    if (m_pending && m_pending->context.view == view) cancel();
    m_leases.removeIf([view](const PermissionLease &lease) { return lease.context.view == view; });
    notifyChanged(); schedule();
}
void PermissionCoordinator::documentLoaded(QObject *view, quint64 epoch) {
    // A confirmed replacement document retires the previous document's leases.
    const auto removed = m_leases.removeIf([=](const PermissionLease &lease) {
        return lease.context.view == view && lease.context.navigationEpoch != epoch;
    });
    if (removed) { notifyChanged(); schedule(); }
}
QStringList PermissionCoordinator::grantedKeys(QObject *view, quint64 epoch) const {
    QStringList result;
    for (const auto &lease : m_leases) if (lease.context.view == view && lease.context.navigationEpoch == epoch
                                         && lease.context.profile && !lease.deadline.expired()) result.append(lease.keys);
    result.removeDuplicates(); return result;
}
qint64 PermissionCoordinator::remainingLeaseMs(QObject *view, quint64 epoch) const {
    qint64 remaining = std::numeric_limits<qint64>::max();
    for (const auto &lease : m_leases) if (lease.context.view == view && lease.context.navigationEpoch == epoch)
        remaining = std::min(remaining, lease.deadline.remainingTime());
    return remaining == std::numeric_limits<qint64>::max() ? 0 : remaining;
}
void PermissionCoordinator::expire() {
    if (m_pending && (m_pending->deadline.expired() || !m_pending->permission.isValid())) cancel();
    QList<QPointer<QObject>> expiredViews;
    for (const auto &lease : m_leases) if (lease.deadline.expired() && lease.context.view
                                          && !expiredViews.contains(lease.context.view)) expiredViews.append(lease.context.view);
    for (const auto &view : expiredViews) {
        qInfo() << "PERMISSION_DOCUMENT_EXPIRED";
        forgetView(view);
        if (view) emit revokeDocument(view);
    }
    schedule();
}
void PermissionCoordinator::revokeAll() {
    cancel();
    QList<QPointer<QObject>> views;
    for (const auto &lease : m_leases) if (lease.context.view && !views.contains(lease.context.view)) views.append(lease.context.view);
    m_leases.clear(); notifyChanged(); schedule();
    for (const auto &view : views) if (view) emit revokeDocument(view);
}
void PermissionCoordinator::schedule() {
    qint64 remaining = m_pending ? m_pending->deadline.remainingTime() : std::numeric_limits<qint64>::max();
    for (const auto &lease : m_leases) remaining = std::min(remaining, lease.deadline.remainingTime());
    if (QCoreApplication::arguments().contains("--controlbar-test")) qInfo() << "PERMISSION_DEADLINE" << remaining << "leases" << m_leases.size();
    if (remaining == std::numeric_limits<qint64>::max()) m_timer.stop();
    else m_timer.start(int(std::clamp<qint64>(remaining, 1, std::numeric_limits<int>::max())));
}
