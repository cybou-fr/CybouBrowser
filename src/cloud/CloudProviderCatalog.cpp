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
                { QStringLiteral("mail"), QStringLiteral("proton"), QStringLiteral("Proton Mail"), QUrl(QStringLiteral("https://mail.proton.me/")), QStringLiteral("mail"), false, false, true },
                { QStringLiteral("calendar"), QStringLiteral("proton"), QStringLiteral("Proton Calendar"), QUrl(QStringLiteral("https://calendar.proton.me/")), QStringLiteral("calendar"), false, false, true },
                { QStringLiteral("files"), QStringLiteral("proton"), QStringLiteral("Proton Drive"), QUrl(QStringLiteral("https://drive.proton.me/")), QStringLiteral("files"), false, false, true },
                { QStringLiteral("docs"), QStringLiteral("proton"), QStringLiteral("Proton Docs"), QUrl(QStringLiteral("https://docs.proton.me/")), QStringLiteral("docs"), false, false, true },
                { QStringLiteral("sheets"), QStringLiteral("proton"), QStringLiteral("Proton Sheets"), QUrl(QStringLiteral("https://drive.proton.me/")), QStringLiteral("sheets"), false, false, true },
                // Note: Proton does not have a standalone Presentations app; role omitted per ADR 0007.
                { QStringLiteral("meetings"), QStringLiteral("proton"), QStringLiteral("Proton Meet"), QUrl(QStringLiteral("https://meet.proton.me/")), QStringLiteral("meetings"), false, false, true }
            }
        },
        {
            QStringLiteral("google"),
            QStringLiteral("Google"),
            QStringLiteral("Suite collaborative Google Workspace."),
            {
                { QStringLiteral("mail"), QStringLiteral("google"), QStringLiteral("Gmail"), QUrl(QStringLiteral("https://mail.google.com/")), QStringLiteral("mail"), false, false, true },
                { QStringLiteral("calendar"), QStringLiteral("google"), QStringLiteral("Google Agenda"), QUrl(QStringLiteral("https://calendar.google.com/")), QStringLiteral("calendar"), false, false, true },
                { QStringLiteral("files"), QStringLiteral("google"), QStringLiteral("Google Drive"), QUrl(QStringLiteral("https://drive.google.com/")), QStringLiteral("files"), false, false, true },
                { QStringLiteral("docs"), QStringLiteral("google"), QStringLiteral("Google Docs"), QUrl(QStringLiteral("https://docs.google.com/document/")), QStringLiteral("docs"), false, false, true },
                { QStringLiteral("sheets"), QStringLiteral("google"), QStringLiteral("Google Sheets"), QUrl(QStringLiteral("https://docs.google.com/spreadsheets/")), QStringLiteral("sheets"), false, false, true },
                { QStringLiteral("presentations"), QStringLiteral("google"), QStringLiteral("Google Slides"), QUrl(QStringLiteral("https://docs.google.com/presentation/")), QStringLiteral("presentations"), false, false, true },
                { QStringLiteral("meetings"), QStringLiteral("google"), QStringLiteral("Google Meet"), QUrl(QStringLiteral("https://meet.google.com/")), QStringLiteral("meetings"), false, false, true }
            }
        },
        {
            QStringLiteral("microsoft"),
            QStringLiteral("Microsoft 365"),
            QStringLiteral("Productivité bureautique et cloud Microsoft 365."),
            {
                { QStringLiteral("mail"), QStringLiteral("microsoft"), QStringLiteral("Outlook"), QUrl(QStringLiteral("https://outlook.live.com/mail/")), QStringLiteral("mail"), false, false, true },
                { QStringLiteral("calendar"), QStringLiteral("microsoft"), QStringLiteral("Outlook Calendrier"), QUrl(QStringLiteral("https://outlook.live.com/calendar/")), QStringLiteral("calendar"), false, false, true },
                { QStringLiteral("files"), QStringLiteral("microsoft"), QStringLiteral("OneDrive"), QUrl(QStringLiteral("https://onedrive.live.com/")), QStringLiteral("files"), false, false, true },
                { QStringLiteral("docs"), QStringLiteral("microsoft"), QStringLiteral("Word pour le web"), QUrl(QStringLiteral("https://www.office.com/launch/word")), QStringLiteral("docs"), false, false, true },
                { QStringLiteral("sheets"), QStringLiteral("microsoft"), QStringLiteral("Excel pour le web"), QUrl(QStringLiteral("https://www.office.com/launch/excel")), QStringLiteral("sheets"), false, false, true },
                { QStringLiteral("presentations"), QStringLiteral("microsoft"), QStringLiteral("PowerPoint pour le web"), QUrl(QStringLiteral("https://www.office.com/launch/powerpoint")), QStringLiteral("presentations"), false, false, true },
                { QStringLiteral("meetings"), QStringLiteral("microsoft"), QStringLiteral("Microsoft Teams"), QUrl(QStringLiteral("https://teams.live.com/")), QStringLiteral("meetings"), false, false, true }
            }
        },
        {
            QStringLiteral("infomaniak"),
            QStringLiteral("Infomaniak kSuite"),
            QStringLiteral("Suite bureautique souveraine et cloud hébergé en Suisse."),
            {
                { QStringLiteral("mail"), QStringLiteral("infomaniak"), QStringLiteral("Infomaniak Mail"), QUrl(QStringLiteral("https://mail.infomaniak.com/")), QStringLiteral("mail"), false, false, true },
                { QStringLiteral("calendar"), QStringLiteral("infomaniak"), QStringLiteral("Infomaniak Calendrier"), QUrl(QStringLiteral("https://calendar.infomaniak.com/")), QStringLiteral("calendar"), false, false, true },
                { QStringLiteral("files"), QStringLiteral("infomaniak"), QStringLiteral("kDrive"), QUrl(QStringLiteral("https://kdrive.infomaniak.com/")), QStringLiteral("files"), false, false, true },
                { QStringLiteral("docs"), QStringLiteral("infomaniak"), QStringLiteral("kDrive Documents"), QUrl(QStringLiteral("https://kdrive.infomaniak.com/")), QStringLiteral("docs"), false, false, true },
                { QStringLiteral("meetings"), QStringLiteral("infomaniak"), QStringLiteral("kMeet"), QUrl(QStringLiteral("https://kmeet.infomaniak.com/")), QStringLiteral("meetings"), false, false, true }
            }
        },
        {
            QStringLiteral("zoho"),
            QStringLiteral("Zoho Workplace"),
            QStringLiteral("Suite collaborative et applications d'entreprise intégrées."),
            {
                { QStringLiteral("mail"), QStringLiteral("zoho"), QStringLiteral("Zoho Mail"), QUrl(QStringLiteral("https://mail.zoho.com/")), QStringLiteral("mail"), false, false, true },
                { QStringLiteral("calendar"), QStringLiteral("zoho"), QStringLiteral("Zoho Calendar"), QUrl(QStringLiteral("https://calendar.zoho.com/")), QStringLiteral("calendar"), false, false, true },
                { QStringLiteral("files"), QStringLiteral("zoho"), QStringLiteral("Zoho WorkDrive"), QUrl(QStringLiteral("https://workdrive.zoho.com/")), QStringLiteral("files"), false, false, true },
                { QStringLiteral("docs"), QStringLiteral("zoho"), QStringLiteral("Zoho Writer"), QUrl(QStringLiteral("https://writer.zoho.com/")), QStringLiteral("docs"), false, false, true },
                { QStringLiteral("sheets"), QStringLiteral("zoho"), QStringLiteral("Zoho Sheet"), QUrl(QStringLiteral("https://sheet.zoho.com/")), QStringLiteral("sheets"), false, false, true },
                { QStringLiteral("presentations"), QStringLiteral("zoho"), QStringLiteral("Zoho Show"), QUrl(QStringLiteral("https://show.zoho.com/")), QStringLiteral("presentations"), false, false, true },
                { QStringLiteral("meetings"), QStringLiteral("zoho"), QStringLiteral("Zoho Meeting"), QUrl(QStringLiteral("https://meeting.zoho.com/")), QStringLiteral("meetings"), false, false, true }
            }
        },
        {
            QStringLiteral("nextcloud"),
            QStringLiteral("Nextcloud Hub"),
            QStringLiteral("Productivité décentralisée sur votre serveur privé."),
            {
                // Self-hosted: URLs are relative app paths; requiresConfiguration is true until user sets nextcloudBaseUrl.
                { QStringLiteral("files"), QStringLiteral("nextcloud"), QStringLiteral("Nextcloud Files"), QUrl(QStringLiteral("/apps/files/")), QStringLiteral("files"), true, false, true },
                { QStringLiteral("mail"), QStringLiteral("nextcloud"), QStringLiteral("Nextcloud Mail"), QUrl(QStringLiteral("/apps/mail/")), QStringLiteral("mail"), true, false, true },
                { QStringLiteral("calendar"), QStringLiteral("nextcloud"), QStringLiteral("Nextcloud Calendar"), QUrl(QStringLiteral("/apps/calendar/")), QStringLiteral("calendar"), true, false, true },
                { QStringLiteral("docs"), QStringLiteral("nextcloud"), QStringLiteral("Nextcloud Text / Office"), QUrl(QStringLiteral("/apps/files/")), QStringLiteral("docs"), true, false, true },
                { QStringLiteral("meetings"), QStringLiteral("nextcloud"), QStringLiteral("Nextcloud Talk"), QUrl(QStringLiteral("/apps/spreed/")), QStringLiteral("meetings"), true, false, true }
            }
        },
        {
            QStringLiteral("apple"),
            QStringLiteral("Apple iCloud"),
            QStringLiteral("Écosystème et services web iCloud."),
            {
                { QStringLiteral("mail"), QStringLiteral("apple"), QStringLiteral("iCloud Mail"), QUrl(QStringLiteral("https://www.icloud.com/mail/")), QStringLiteral("mail"), false, false, true },
                { QStringLiteral("calendar"), QStringLiteral("apple"), QStringLiteral("iCloud Calendrier"), QUrl(QStringLiteral("https://www.icloud.com/calendar/")), QStringLiteral("calendar"), false, false, true },
                { QStringLiteral("files"), QStringLiteral("apple"), QStringLiteral("iCloud Drive"), QUrl(QStringLiteral("https://www.icloud.com/iclouddrive/")), QStringLiteral("files"), false, false, true },
                { QStringLiteral("docs"), QStringLiteral("apple"), QStringLiteral("Pages pour iCloud"), QUrl(QStringLiteral("https://www.icloud.com/pages/")), QStringLiteral("docs"), false, false, true },
                { QStringLiteral("sheets"), QStringLiteral("apple"), QStringLiteral("Numbers pour iCloud"), QUrl(QStringLiteral("https://www.icloud.com/numbers/")), QStringLiteral("sheets"), false, false, true },
                { QStringLiteral("presentations"), QStringLiteral("apple"), QStringLiteral("Keynote pour iCloud"), QUrl(QStringLiteral("https://www.icloud.com/keynote/")), QStringLiteral("presentations"), false, false, true }
                // Meetings is not provided on web by Apple iCloud.
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

bool CloudProviderCatalog::providerSupportsRole(const QString &providerId, const QString &role) {
    return findService(providerId, role).has_value();
}

bool CloudProviderCatalog::isVerifiedServiceUrl(const QUrl &url, const QString &customNextcloudHost) {
    if (!url.isValid() || url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0) {
        return false;
    }
    // Check against custom user-configured Nextcloud host
    if (!customNextcloudHost.isEmpty() && url.host().compare(customNextcloudHost, Qt::CaseInsensitive) == 0) {
        return true;
    }

    const auto &providers = allProviders();
    for (const auto &p : providers) {
        if (p.id == QStringLiteral("nextcloud")) continue; // Nextcloud is self-hosted only
        for (const auto &s : p.services) {
            if (s.url.host().compare(url.host(), Qt::CaseInsensitive) == 0) {
                // If service defines a specific path, verify that url starts with that path
                const QString servicePath = s.url.path();
                if (servicePath.isEmpty() || servicePath == QStringLiteral("/") || url.path().startsWith(servicePath)) {
                    return true;
                }
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
