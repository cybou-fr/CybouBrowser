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
    Q_PROPERTY(QVariantList availableProviders READ availableProviders CONSTANT)
    Q_PROPERTY(QVariantList resolvedServices READ resolvedServices NOTIFY configurationChanged)
    Q_PROPERTY(QVariantMap roleOverrides READ roleOverrides NOTIFY configurationChanged)
public:
    explicit CloudServiceManager(QObject *parent = nullptr);

    // Configuration state
    QString defaultProvider() const;
    void setDefaultProvider(const QString &providerId);
    bool isEnabled() const;

    // Role resolution
    // override values can be:
    // - "inherit" : follows defaultProvider
    // - "disabled" : explicitly disabled
    // - "<providerId>" : explicit provider override (e.g. "google", "proton")
    Q_INVOKABLE QString roleOverride(const QString &role) const;
    Q_INVOKABLE void setRoleOverride(const QString &role, const QString &choice);
    Q_INVOKABLE void resetRoleOverride(const QString &role);
    Q_INVOKABLE void resetAllOverrides();

    // Resolves a single role into effective service info (empty map if disabled/none/unavailable)
    Q_INVOKABLE QVariantMap resolveRole(const QString &role) const;

    // Resolves URL for a specific role
    Q_INVOKABLE QUrl resolveUrl(const QString &role) const;

    // Fast-path for NewTab mail button
    Q_INVOKABLE QUrl mailUrl() const;

    // QML convenience properties
    QVariantList availableProviders() const;
    QVariantList resolvedServices() const;
    QVariantMap roleOverrides() const;

signals:
    void configurationChanged();

private:
    void loadSettings();
    void saveSettings();

    QString m_defaultProvider{QStringLiteral("proton")};
    QMap<QString, QString> m_roleOverrides;
};
