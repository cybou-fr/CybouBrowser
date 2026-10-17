#pragma once
#include <QString>
#include <QUrl>
#include <QVector>
#include <QMap>
#include <optional>

struct CloudServiceInfo {
    QString role;           // "mail", "calendar", "files", "docs", "sheets", "presentations", "meetings"
    QString name;           // e.g. "Proton Mail", "Google Drive", etc.
    QUrl url;               // Verified official HTTPS URL
    QString iconSvgName;    // neutral icon key or path
};

struct CloudProviderInfo {
    QString id;             // "proton", "google", "microsoft", "nextcloud", "apple"
    QString name;           // "Proton", "Google Workspace", "Microsoft 365", etc.
    QString description;    // Human-readable summary
    QVector<CloudServiceInfo> services;
};

class CloudProviderCatalog {
public:
    static const QVector<QString> &supportedRoles();
    static const QVector<CloudProviderInfo> &allProviders();
    static std::optional<CloudProviderInfo> findProvider(const QString &providerId);
    static std::optional<CloudServiceInfo> findService(const QString &providerId, const QString &role);
    static bool isVerifiedServiceUrl(const QUrl &url);
    static QString roleDisplayName(const QString &role);
};
