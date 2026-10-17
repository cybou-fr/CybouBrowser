#pragma once
#include <QString>
#include <QUrl>
#include <QVector>
#include <QMap>
#include <optional>

struct CloudServiceInfo {
    QString role;                   // "mail", "calendar", "files", "docs", "sheets", "presentations", "meetings"
    QString providerId;             // "proton", "google", "microsoft", "infomaniak", "zoho", "nextcloud", "apple"
    QString name;                   // e.g. "Proton Mail", "Google Drive", etc.
    QUrl url;                       // Verified official HTTPS URL or relative path template (e.g. "/apps/files/")
    QString iconSvgName;            // neutral icon key
    bool requiresConfiguration{false}; // True if self-hosted base URL must be set
    bool requiresSubscription{false};  // True if commercial/tier required
    bool verifiedEndpoint{true};       // True if official endpoint is verified
};

struct CloudProviderInfo {
    QString id;                     // "proton", "google", "microsoft", "infomaniak", "zoho", "nextcloud", "apple"
    QString name;                   // "Proton", "Google Workspace", "Microsoft 365", etc.
    QString description;            // Human-readable summary
    QVector<CloudServiceInfo> services;
};

enum class CloudAppKind {
    WebApp,
    AccountPortal,
    ProductInfo
};

struct CloudAppInfo {
    QString appId;                  // e.g. "proton_mail", "proton_pass", "google_drive"
    QString providerId;             // "proton", "google", "microsoft", etc.
    QString displayName;            // "Proton Pass", "Google Keep", etc.
    QString roleId;                 // Role if mapped to standard role ("mail", "files", etc.) or empty if standalone
    QString iconId;                 // Neutral icon id matching SVG in assets/cloud/ (e.g. "pass", "ai", "wallet")
    QUrl officialUrl;               // Verified canonical HTTPS URL
    CloudAppKind kind{CloudAppKind::WebApp};
    bool enabledByDefault{true};
    bool requiresConfiguration{false};
};

class CloudProviderCatalog {
public:
    static const QVector<QString> &supportedRoles();
    static const QVector<CloudProviderInfo> &allProviders();
    static const QVector<CloudAppInfo> &allApps();
    static QVector<CloudAppInfo> appsForProvider(const QString &providerId);
    static std::optional<CloudProviderInfo> findProvider(const QString &providerId);
    static std::optional<CloudServiceInfo> findService(const QString &providerId, const QString &role);
    static std::optional<CloudAppInfo> findApp(const QString &appId);
    static bool isVerifiedServiceUrl(const QUrl &url, const QString &customNextcloudHost = QString());
    static QString roleDisplayName(const QString &role);
    static bool providerSupportsRole(const QString &providerId, const QString &role);
};
