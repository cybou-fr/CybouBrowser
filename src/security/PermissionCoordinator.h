#pragma once
#include "PermissionLease.h"
#include <QSet>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include <QtWebEngineCore/QWebEnginePermission>
#include <optional>

class PermissionCoordinator : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool hasPending READ hasPending NOTIFY changed)
    Q_PROPERTY(QWebEnginePermission pendingPermission READ pendingPermission NOTIFY changed)
    Q_PROPERTY(QObject *pendingView READ pendingView NOTIFY changed)
    Q_PROPERTY(quint64 pendingRequestId READ pendingRequestId NOTIFY changed)
    Q_PROPERTY(int revision READ revision NOTIFY changed)
public:
    explicit PermissionCoordinator(QObject *parent = nullptr);
    ~PermissionCoordinator() override;
    bool hasPending() const { return m_pending.has_value(); }
    QWebEnginePermission pendingPermission() const { return m_pending ? m_pending->permission : QWebEnginePermission(); }
    QObject *pendingView() const { return m_pending ? m_pending->context.view.data() : nullptr; }
    quint64 pendingRequestId() const { return m_pending ? m_pending->id : 0; }
    int revision() const { return m_revision; }
    static QStringList keysFor(QWebEnginePermission::PermissionType type);
    Q_INVOKABLE bool submit(const QWebEnginePermission &permission, QObject *view, QObject *profile,
                            bool foreground, bool policyDenied);
    Q_INVOKABLE bool resolve(bool allow, QObject *activeView, QObject *activeProfile, quint64 requestId);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void forgetView(QObject *view);
    Q_INVOKABLE void documentLoaded(QObject *view, quint64 epoch);
    Q_INVOKABLE void revokeAll();
    Q_INVOKABLE QStringList grantedKeys(QObject *view, quint64 epoch) const;
    Q_INVOKABLE qint64 remainingLeaseMs(QObject *view, quint64 epoch) const;
signals:
    void changed();
    void requestObserved(const QWebEnginePermission &permission, const QString &tab, const QUrl &top, bool blocked);
    void decisionFinished(const QWebEnginePermission &permission, const QString &tab, bool allowed);
    void revokeDocument(QObject *view);
private:
    struct Pending {
        quint64 id;
        QWebEnginePermission permission;
        SiteContext context;
        PermissionDeadline deadline;
    };
    void watch(QObject *object);
    void expire();
    void schedule();
    void notifyChanged();
    std::optional<Pending> m_pending;
    QList<PermissionLease> m_leases;
    QSet<QObject *> m_watched;
    QTimer m_timer;
    quint64 m_nextRequestId = 0;
    int m_revision = 0;
};
