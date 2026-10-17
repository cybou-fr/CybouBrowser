#pragma once
#include <QObject>
#include <QString>
#include <QMap>
#include <QVariantList>
#include <QVariantMap>
#include <QSettings>
#include "CloudProviderCatalog.h"

class CloudServiceManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString defaultProvider READ defaultProvider WRITE setDefaultProvider NOTIFY configurationChanged)
    Q_PROPERTY(bool isEnabled READ isEnabled NOTIFY configurationChanged)
    Q_PROPERTY(QString nextcloudBaseUrl READ nextcloudBaseUrl WRITE setNextcloudBaseUrl NOTIFY nextcloudBaseUrlChanged)
    Q_PROPERTY(QVariantList availableProviders READ availableProviders CONSTANT)
    Q_PROPERTY(QVariantList resolvedServices READ resolvedServices NOTIFY configurationChanged)
    Q_PROPERTY(QVariantList resolvedApps READ resolvedApps NOTIFY configurationChanged)
    Q_PROPERTY(QVariantMap roleOverrides READ roleOverrides NOTIFY configurationChanged)
public:
    explicit CloudServiceManager(QObject *parent = nullptr);

    // Configuration state
    QString defaultProvider() const;
    void setDefaultProvider(const QString &providerId);
    bool isEnabled() const;

    QString nextcloudBaseUrl() const;
    void setNextcloudBaseUrl(const QString &url);

    // Role resolution
    // override values can be:
    // - "inherit" : follows defaultProvider
    // - "disabled" : explicitly disabled
    // - "<providerId>" : explicit provider override (e.g. "google", "proton")
    Q_INVOKABLE QString roleOverride(const QString &role) const;
    Q_INVOKABLE void setRoleOverride(const QString &role, const QString &choice);
    Q_INVOKABLE void resetRoleOverride(const QString &role);
    Q_INVOKABLE void resetAllOverrides();

    // Returns providers eligible for a specific role (for QML dropdowns)
    Q_INVOKABLE QVariantList providersForRole(const QString &role) const;

    // Resolves a single role into effective service info (empty map if disabled/none/unavailable)
    Q_INVOKABLE QVariantMap resolveRole(const QString &role) const;

    // Resolves URL for a specific role or app ID
    Q_INVOKABLE QUrl resolveUrl(const QString &role) const;
    Q_INVOKABLE QUrl resolveAppUrl(const QString &appId) const;

    // Fast-path for NewTab mail button
    Q_INVOKABLE QUrl mailUrl() const;
    Q_INVOKABLE bool isMailAvailable() const;

    // URL validation helper for self-hosted instances
    static bool sanitizeNextcloudUrl(const QString &input, QString *sanitizedOut = nullptr);

    // QML convenience properties
    QVariantList availableProviders() const;
    QVariantList resolvedServices() const;
    QVariantList resolvedApps() const;
    QVariantMap roleOverrides() const;

signals:
    void configurationChanged();
    void nextcloudBaseUrlChanged();

private:
    void loadSettings();
    void saveSettings();

    QString m_defaultProvider{QStringLiteral("proton")};
    QString m_nextcloudBaseUrl;
    QMap<QString, QString> m_roleOverrides;
};
