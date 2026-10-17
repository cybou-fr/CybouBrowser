#include "CloudProviderCatalog.h"

const QVector<QString> &CloudProviderCatalog::supportedRoles() {
    static const QVector<QString> roles = {
        QStringLiteral("mail"),
        QStringLiteral("calendar"),
        QStringLiteral("files"),
        QStringLiteral("docs"),
        QStringLiteral("sheets"),
        QStringLiteral("presentations"),
        QStringLiteral("meetings")
    };
    return roles;
}

const QVector<CloudProviderInfo> &CloudProviderCatalog::allProviders() {
    static const QVector<CloudProviderInfo> providers = {
        {
            QStringLiteral("proton"),
            QStringLiteral("Proton"),
            QStringLiteral("Confidentialité par défaut, chiffrement de bout en bout."),
            {
                { QStringLiteral("mail"), QStringLiteral("Proton Mail"), QUrl(QStringLiteral("https://mail.proton.me/")), QStringLiteral("mail") },
                { QStringLiteral("calendar"), QStringLiteral("Proton Calendar"), QUrl(QStringLiteral("https://calendar.proton.me/")), QStringLiteral("calendar") },
                { QStringLiteral("files"), QStringLiteral("Proton Drive"), QUrl(QStringLiteral("https://drive.proton.me/")), QStringLiteral("files") },
                { QStringLiteral("docs"), QStringLiteral("Proton Docs"), QUrl(QStringLiteral("https://docs.proton.me/")), QStringLiteral("docs") },
                { QStringLiteral("sheets"), QStringLiteral("Proton Sheets"), QUrl(QStringLiteral("https://drive.proton.me/")), QStringLiteral("sheets") },
                { QStringLiteral("presentations"), QStringLiteral("Proton Presentations"), QUrl(QStringLiteral("https://drive.proton.me/")), QStringLiteral("presentations") },
                { QStringLiteral("meetings"), QStringLiteral("Proton Meet"), QUrl(QStringLiteral("https://meet.proton.me/")), QStringLiteral("meetings") }
            }
        },
        {
            QStringLiteral("google"),
            QStringLiteral("Google"),
            QStringLiteral("Suite collaborative Google Workspace."),
            {
                { QStringLiteral("mail"), QStringLiteral("Gmail"), QUrl(QStringLiteral("https://mail.google.com/")), QStringLiteral("mail") },
                { QStringLiteral("calendar"), QStringLiteral("Google Agenda"), QUrl(QStringLiteral("https://calendar.google.com/")), QStringLiteral("calendar") },
                { QStringLiteral("files"), QStringLiteral("Google Drive"), QUrl(QStringLiteral("https://drive.google.com/")), QStringLiteral("files") },
                { QStringLiteral("docs"), QStringLiteral("Google Docs"), QUrl(QStringLiteral("https://docs.google.com/document/")), QStringLiteral("docs") },
                { QStringLiteral("sheets"), QStringLiteral("Google Sheets"), QUrl(QStringLiteral("https://docs.google.com/spreadsheets/")), QStringLiteral("sheets") },
                { QStringLiteral("presentations"), QStringLiteral("Google Slides"), QUrl(QStringLiteral("https://docs.google.com/presentation/")), QStringLiteral("presentations") },
                { QStringLiteral("meetings"), QStringLiteral("Google Meet"), QUrl(QStringLiteral("https://meet.google.com/")), QStringLiteral("meetings") }
            }
        },
        {
            QStringLiteral("microsoft"),
            QStringLiteral("Microsoft 365"),
            QStringLiteral("Productivité bureautique et cloud Microsoft 365."),
            {
                { QStringLiteral("mail"), QStringLiteral("Outlook"), QUrl(QStringLiteral("https://outlook.live.com/mail/")), QStringLiteral("mail") },
                { QStringLiteral("calendar"), QStringLiteral("Outlook Calendrier"), QUrl(QStringLiteral("https://outlook.live.com/calendar/")), QStringLiteral("calendar") },
                { QStringLiteral("files"), QStringLiteral("OneDrive"), QUrl(QStringLiteral("https://onedrive.live.com/")), QStringLiteral("files") },
                { QStringLiteral("docs"), QStringLiteral("Word pour le web"), QUrl(QStringLiteral("https://www.office.com/launch/word")), QStringLiteral("docs") },
                { QStringLiteral("sheets"), QStringLiteral("Excel pour le web"), QUrl(QStringLiteral("https://www.office.com/launch/excel")), QStringLiteral("sheets") },
                { QStringLiteral("presentations"), QStringLiteral("PowerPoint pour le web"), QUrl(QStringLiteral("https://www.office.com/launch/powerpoint")), QStringLiteral("presentations") },
                { QStringLiteral("meetings"), QStringLiteral("Microsoft Teams"), QUrl(QStringLiteral("https://teams.live.com/")), QStringLiteral("meetings") }
            }
        },
        {
            QStringLiteral("nextcloud"),
            QStringLiteral("Nextcloud"),
            QStringLiteral("Productivité décentralisée et auto-hébergée."),
            {
                { QStringLiteral("mail"), QStringLiteral("Nextcloud Mail"), QUrl(QStringLiteral("https://try.nextcloud.com/apps/mail/")), QStringLiteral("mail") },
                { QStringLiteral("calendar"), QStringLiteral("Nextcloud Calendar"), QUrl(QStringLiteral("https://try.nextcloud.com/apps/calendar/")), QStringLiteral("calendar") },
                { QStringLiteral("files"), QStringLiteral("Nextcloud Files"), QUrl(QStringLiteral("https://try.nextcloud.com/apps/files/")), QStringLiteral("files") },
                { QStringLiteral("docs"), QStringLiteral("Nextcloud Text"), QUrl(QStringLiteral("https://try.nextcloud.com/apps/files/")), QStringLiteral("docs") },
                { QStringLiteral("sheets"), QStringLiteral("Nextcloud Tables"), QUrl(QStringLiteral("https://try.nextcloud.com/apps/tables/")), QStringLiteral("sheets") },
                { QStringLiteral("presentations"), QStringLiteral("Nextcloud Office"), QUrl(QStringLiteral("https://try.nextcloud.com/apps/files/")), QStringLiteral("presentations") },
                { QStringLiteral("meetings"), QStringLiteral("Nextcloud Talk"), QUrl(QStringLiteral("https://try.nextcloud.com/apps/spreed/")), QStringLiteral("meetings") }
            }
        },
        {
            QStringLiteral("apple"),
            QStringLiteral("Apple iCloud"),
            QStringLiteral("Écosystème et services web iCloud."),
            {
                { QStringLiteral("mail"), QStringLiteral("iCloud Mail"), QUrl(QStringLiteral("https://www.icloud.com/mail/")), QStringLiteral("mail") },
                { QStringLiteral("calendar"), QStringLiteral("iCloud Calendrier"), QUrl(QStringLiteral("https://www.icloud.com/calendar/")), QStringLiteral("calendar") },
                { QStringLiteral("files"), QStringLiteral("iCloud Drive"), QUrl(QStringLiteral("https://www.icloud.com/iclouddrive/")), QStringLiteral("files") },
                { QStringLiteral("docs"), QStringLiteral("Pages pour iCloud"), QUrl(QStringLiteral("https://www.icloud.com/pages/")), QStringLiteral("docs") },
                { QStringLiteral("sheets"), QStringLiteral("Numbers pour iCloud"), QUrl(QStringLiteral("https://www.icloud.com/numbers/")), QStringLiteral("sheets") },
                { QStringLiteral("presentations"), QStringLiteral("Keynote pour iCloud"), QUrl(QStringLiteral("https://www.icloud.com/keynote/")), QStringLiteral("presentations") }
                // Note: meetings is not provided on web by Apple iCloud, role is omitted
            }
        }
    };
    return providers;
}

std::optional<CloudProviderInfo> CloudProviderCatalog::findProvider(const QString &providerId) {
    const auto &providers = allProviders();
    for (const auto &p : providers) {
        if (p.id == providerId) return p;
    }
    return std::nullopt;
}

std::optional<CloudServiceInfo> CloudProviderCatalog::findService(const QString &providerId, const QString &role) {
    const auto provider = findProvider(providerId);
    if (!provider) return std::nullopt;
    for (const auto &s : provider->services) {
        if (s.role == role) return s;
    }
    return std::nullopt;
}

bool CloudProviderCatalog::isVerifiedServiceUrl(const QUrl &url) {
    if (!url.isValid() || url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0) {
        return false;
    }
    const auto &providers = allProviders();
    for (const auto &p : providers) {
        for (const auto &s : p.services) {
            if (s.url.host().compare(url.host(), Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
    }
    return false;
}

QString CloudProviderCatalog::roleDisplayName(const QString &role) {
    if (role == QStringLiteral("mail")) return QStringLiteral("Courrier");
    if (role == QStringLiteral("calendar")) return QStringLiteral("Calendrier");
    if (role == QStringLiteral("files")) return QStringLiteral("Fichiers");
    if (role == QStringLiteral("docs")) return QStringLiteral("Documents");
    if (role == QStringLiteral("sheets")) return QStringLiteral("Tableurs");
    if (role == QStringLiteral("presentations")) return QStringLiteral("Présentations");
    if (role == QStringLiteral("meetings")) return QStringLiteral("Réunions vidéo");
    return role;
}
