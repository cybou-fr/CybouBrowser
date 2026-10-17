#pragma once
#include "PermissionLease.h"
#include <QSet>
#include <QList>
#include <QPointer>
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
    Q_PROPERTY(QObject *activeView READ activeView WRITE setActiveView NOTIFY activeViewChanged)
public:
    explicit PermissionCoordinator(QObject *parent = nullptr);
    ~PermissionCoordinator() override;
    bool hasPending() const { return m_pending.has_value(); }
    QWebEnginePermission pendingPermission() const { return m_pending ? m_pending->permission : QWebEnginePermission(); }
    QObject *pendingView() const { return m_pending ? m_pending->context.view.data() : nullptr; }
    quint64 pendingRequestId() const { return m_pending ? m_pending->id : 0; }
    int revision() const { return m_revision; }
    QObject *activeView() const { return m_activeView.data(); }
    void setActiveView(QObject *view);
    qsizetype pendingCount() const { return (m_pending ? 1 : 0) + m_queue.size(); }
    qsizetype queuedCount() const { return m_queue.size(); }
    static QStringList keysFor(QWebEnginePermission::PermissionType type);
    Q_INVOKABLE bool submit(const QWebEnginePermission &permission, QObject *view, QObject *profile,
                            bool foreground, bool policyDenied);
    Q_INVOKABLE bool resolve(bool allow, QObject *activeView, QObject *activeProfile, quint64 requestId);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void forgetView(QObject *view);
    Q_INVOKABLE void documentLoaded(QObject *view, quint64 epoch);
    Q_INVOKABLE void revokeAll();
    Q_INVOKABLE QStringList grantedKeys(QObject *view, quint64 epoch) const;
    Q_INVOKABLE bool hasPendingFor(const QString &tab) const;
signals:
    void changed();
    void activeViewChanged();
    void requestObserved(const QWebEnginePermission &permission, const QString &tab, const QUrl &top, bool blocked);
    void decisionFinished(const QWebEnginePermission &permission, const QString &tab, bool allowed);
    void revokeDocument(QObject *view);
private:
    struct Pending {
        quint64 id = 0;
        QWebEnginePermission permission;
        SiteContext context;
        PermissionDeadline deadline;
    };
    void watch(QObject *object);
    void expire();
    void schedule();
    void notifyChanged();
    void promoteNext();
    std::optional<Pending> m_pending;
    QList<Pending> m_queue;
    QPointer<QObject> m_activeView;
    QList<PermissionLease> m_leases;
    QSet<QObject *> m_watched;
    QTimer m_timer;
    quint64 m_nextRequestId = 0;
    int m_revision = 0;
};
