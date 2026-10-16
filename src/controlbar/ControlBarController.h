#pragma once
#include <QObject>
#include <QPointer>
#include <QSettings>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include <QtWebEngineQuick/QQuickWebEngineProfile>
#include <QtWebEngineCore/QWebEnginePermission>
#include "CapabilityModel.h"

class BrowserStore;
class ControlBarController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject *profile READ profile WRITE setProfile NOTIFY changed)
    Q_PROPERTY(QObject *store READ store WRITE setStore NOTIFY changed)
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY changed)
    Q_PROPERTY(QString tabId READ tabId WRITE setTabId NOTIFY changed)
    Q_PROPERTY(bool javascriptEnabled MEMBER javascriptEnabled NOTIFY contextChanged)
    Q_PROPERTY(bool storageEnabled MEMBER storageEnabled NOTIFY contextChanged)
    Q_PROPERTY(QStringList grantedKeys MEMBER grantedKeys NOTIFY contextChanged)
    Q_PROPERTY(bool reloadRequired MEMBER reloadRequired NOTIFY contextChanged)
    Q_PROPERTY(QAbstractItemModel *model READ model CONSTANT)
    Q_PROPERTY(int revision READ revision NOTIFY changed)
public:
    enum class PolicyState { Ask, Allow, Deny, Unavailable, ReloadRequired };
    Q_ENUM(PolicyState)
    enum class ObservationState { None, Requested, BlockedAttempt, RecentVerified };
    Q_ENUM(ObservationState)
    explicit ControlBarController(QObject *parent = nullptr);
    QObject *profile() const { return m_profile; }
    void setProfile(QObject *profile);
    QObject *store() const;
    void setStore(QObject *store);
    QUrl url() const { return m_url; }
    void setUrl(const QUrl &url) { if (url == m_url) return; m_url = url; refresh(); }
    QString tabId() const { return m_tabId; }
    void setTabId(const QString &id) { if (id == m_tabId) return; m_tabId = id; refresh(); }
    QAbstractItemModel *model() { return &m_model; }
    int revision() const { return m_revision; }
    bool javascriptEnabled = true;
    bool storageEnabled = true;
    QStringList grantedKeys;
    bool reloadRequired = false;
    Q_INVOKABLE QVariantMap capability(const QString &key) const { return m_model.row(key); }
    Q_INVOKABLE bool settingFor(const QUrl &url, const QString &key, const QString &tab = {}) const;
    Q_INVOKABLE int popupMode(const QUrl &url, const QString &tab = {}) const;
    Q_INVOKABLE bool setBehavior(const QString &key, int mode);
    Q_INVOKABLE bool setPermissionPolicy(const QString &key, int mode);
    Q_INVOKABLE bool shouldDeny(const QWebEnginePermission &permission) const;
    Q_INVOKABLE void notePermission(const QWebEnginePermission &permission, const QString &tab, const QUrl &top, bool blocked);
    Q_INVOKABLE void finishPermission(const QWebEnginePermission &permission, const QString &tab, bool allowed);
    Q_INVOKABLE void abandonPending(const QString &tab);
    Q_INVOKABLE void clearPermissionPolicies();
    Q_INVOKABLE void expireContext(const QString &tab);
    Q_INVOKABLE void inheritContext(const QString &fromTab, const QString &toTab, const QUrl &source, const QUrl &target) {
        const auto subject = origin(source);
        if (subject.isEmpty() || subject != origin(target) || fromTab == toTab) return;
        const auto suffix = QString::fromLatin1(subject.toUtf8().toBase64(QByteArray::Base64UrlEncoding)) + "/";
        const auto prefix = fromTab + "/" + suffix;
        const auto snapshot = m_privatePolicies;
        for (auto it = snapshot.cbegin(); it != snapshot.cend(); ++it)
            if (it.key().startsWith(prefix)) m_privatePolicies[toTab + "/" + suffix + it.key().mid(prefix.size())] = it.value();
    }
    Q_INVOKABLE void notePopup(const QString &tab, const QUrl &url, bool blocked);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE QVariantList networkSample() const;
    static QString origin(const QUrl &url);
signals:
    void changed();
    void contextChanged();
    void policyChanged(const QString &origin, const QString &key);
private:
    struct Observation { QString key, tab, top, requester; qint64 time; bool pending = false, blocked = false; };
    QVariant policyValue(const QUrl &url, const QString &key, const QVariant &fallback, const QString &tab = {}) const;
    bool writePolicy(const QUrl &url, const QString &key, const QVariant &value);
    static QStringList keysFor(QWebEnginePermission::PermissionType type);
    static QWebEnginePermission::PermissionType typeFor(const QString &key);
    QPointer<QQuickWebEngineProfile> m_profile;
    QPointer<BrowserStore> m_store;
    QUrl m_url;
    QString m_tabId;
    CapabilityModel m_model;
    QSettings m_settings;
    QHash<QString, QVariant> m_privatePolicies;
    QList<Observation> m_observations;
    QTimer m_timer;
    int m_revision = 0;
};
