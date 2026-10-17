#include "CloudServiceManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace {
constexpr int kSchemaVersion = 1;
const QString kSettingsKey = QStringLiteral("cloud/configuration");
}

CloudServiceManager::CloudServiceManager(QObject *parent)
    : QObject(parent) {
    loadSettings();
}

QString CloudServiceManager::defaultProvider() const {
    return m_defaultProvider;
}

void CloudServiceManager::setDefaultProvider(const QString &providerId) {
    if (m_defaultProvider == providerId) return;

    // Validation: only allow "none" or valid provider IDs from catalog
    if (providerId != QStringLiteral("none") && !CloudProviderCatalog::findProvider(providerId).has_value()) {
        return; // Reject invalid provider ID
    }

    m_defaultProvider = providerId;
    saveSettings();
    emit configurationChanged();
}

bool CloudServiceManager::isEnabled() const {
    return m_defaultProvider != QStringLiteral("none");
}

QString CloudServiceManager::nextcloudBaseUrl() const {
    return m_nextcloudBaseUrl;
}

void CloudServiceManager::setNextcloudBaseUrl(const QString &url) {
    QString sanitized;
    if (url.trimmed().isEmpty()) {
        if (m_nextcloudBaseUrl.isEmpty()) return;
        m_nextcloudBaseUrl.clear();
        saveSettings();
        emit nextcloudBaseUrlChanged();
        emit configurationChanged();
        return;
    }

    if (!sanitizeNextcloudUrl(url, &sanitized)) {
        return; // Reject invalid or non-HTTPS URL
    }

    if (m_nextcloudBaseUrl == sanitized) return;
    m_nextcloudBaseUrl = sanitized;
    saveSettings();
    emit nextcloudBaseUrlChanged();
    emit configurationChanged();
}

bool CloudServiceManager::sanitizeNextcloudUrl(const QString &input, QString *sanitizedOut) {
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) return false;

    QUrl parsed(trimmed);
    if (!parsed.isValid()) return false;
    if (parsed.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0) return false;
    if (parsed.host().isEmpty()) return false;
    if (!parsed.userName().isEmpty() || !parsed.password().isEmpty()) return false;
    if (parsed.hasQuery() || parsed.hasFragment()) return false;

    // Clean up trailing slash
    QString clean = parsed.toString(QUrl::StripTrailingSlash);
    while (clean.endsWith(QLatin1Char('/'))) {
        clean.chop(1);
    }
    if (sanitizedOut) {
        *sanitizedOut = clean;
    }
    return true;
}

QString CloudServiceManager::roleOverride(const QString &role) const {
    return m_roleOverrides.value(role, QStringLiteral("inherit"));
}

void CloudServiceManager::setRoleOverride(const QString &role, const QString &choice) {
    if (!CloudProviderCatalog::supportedRoles().contains(role)) {
        return; // Reject invalid role
    }

    // Validation: choice can be "inherit", "disabled", or a provider that actually supports this role
    if (choice != QStringLiteral("inherit") && choice != QStringLiteral("disabled")) {
        if (!CloudProviderCatalog::providerSupportsRole(choice, role)) {
            return; // Reject choice if provider does not offer this service
        }
    }

    if (m_roleOverrides.value(role) == choice) return;
    if (choice == QStringLiteral("inherit")) {
        m_roleOverrides.remove(role);
    } else {
        m_roleOverrides.insert(role, choice);
    }
    saveSettings();
    emit configurationChanged();
}

void CloudServiceManager::resetRoleOverride(const QString &role) {
    if (!m_roleOverrides.contains(role)) return;
    m_roleOverrides.remove(role);
    saveSettings();
    emit configurationChanged();
}

void CloudServiceManager::resetAllOverrides() {
    if (m_roleOverrides.isEmpty()) return;
    m_roleOverrides.clear();
    saveSettings();
    emit configurationChanged();
}

QVariantList CloudServiceManager::providersForRole(const QString &role) const {
    QVariantList list;

    // 1. Inherit option
    QVariantMap inheritOpt;
    inheritOpt.insert(QStringLiteral("text"), QStringLiteral("Comme suite principale (par défaut)"));
    inheritOpt.insert(QStringLiteral("value"), QStringLiteral("inherit"));
    list.append(inheritOpt);

    // 2. Only providers that actually provide this role
    for (const auto &p : CloudProviderCatalog::allProviders()) {
        if (CloudProviderCatalog::providerSupportsRole(p.id, role)) {
            QVariantMap opt;
            opt.insert(QStringLiteral("text"), p.name);
            opt.insert(QStringLiteral("value"), p.id);
            list.append(opt);
        }
    }

    // 3. Explicit disabled option
    QVariantMap disabledOpt;
    disabledOpt.insert(QStringLiteral("text"), QStringLiteral("Désactivé pour ce service"));
    disabledOpt.insert(QStringLiteral("value"), QStringLiteral("disabled"));
    list.append(disabledOpt);

    return list;
}

QVariantMap CloudServiceManager::resolveRole(const QString &role) const {
    if (!isEnabled()) return {};

    const QString choice = roleOverride(role);
    if (choice == QStringLiteral("disabled")) return {};

    const QString providerId = (choice == QStringLiteral("inherit")) ? m_defaultProvider : choice;
    if (providerId == QStringLiteral("none")) return {};

    const auto serviceOpt = CloudProviderCatalog::findService(providerId, role);
    if (!serviceOpt) return {};

    const auto &s = *serviceOpt;
    const auto providerOpt = CloudProviderCatalog::findProvider(providerId);
    const QString providerName = providerOpt ? providerOpt->name : providerId;

    QVariantMap map;
    map.insert(QStringLiteral("role"), s.role);
    map.insert(QStringLiteral("roleName"), CloudProviderCatalog::roleDisplayName(s.role));
    map.insert(QStringLiteral("name"), s.name);
    map.insert(QStringLiteral("providerId"), providerId);
    map.insert(QStringLiteral("providerName"), providerName);
    map.insert(QStringLiteral("icon"), s.iconSvgName);

    if (providerId == QStringLiteral("nextcloud")) {
        if (m_nextcloudBaseUrl.isEmpty()) {
            map.insert(QStringLiteral("requiresConfiguration"), true);
            map.insert(QStringLiteral("url"), QString());
        } else {
            map.insert(QStringLiteral("requiresConfiguration"), false);
            map.insert(QStringLiteral("url"), m_nextcloudBaseUrl + s.url.toString());
        }
    } else {
        map.insert(QStringLiteral("requiresConfiguration"), false);
        map.insert(QStringLiteral("url"), s.url.toString());
    }

    return map;
}

QUrl CloudServiceManager::resolveUrl(const QString &role) const {
    const auto map = resolveRole(role);
    if (map.isEmpty()) return {};
    return QUrl(map.value(QStringLiteral("url")).toString());
}

QUrl CloudServiceManager::mailUrl() const {
    return resolveUrl(QStringLiteral("mail"));
}

bool CloudServiceManager::isMailAvailable() const {
    return !mailUrl().isEmpty();
}

QVariantList CloudServiceManager::availableProviders() const {
    QVariantList list;
    // Option to disable / none
    QVariantMap noneOpt;
    noneOpt.insert(QStringLiteral("id"), QStringLiteral("none"));
    noneOpt.insert(QStringLiteral("name"), QStringLiteral("Désactivé"));
    noneOpt.insert(QStringLiteral("description"), QStringLiteral("Aucune suite облачной интеграции"));
    list.append(noneOpt);

    for (const auto &p : CloudProviderCatalog::allProviders()) {
        QVariantMap map;
        map.insert(QStringLiteral("id"), p.id);
        map.insert(QStringLiteral("name"), p.name);
        map.insert(QStringLiteral("description"), p.description);
        list.append(map);
    }
    return list;
}

QVariantList CloudServiceManager::resolvedServices() const {
    QVariantList list;
    if (!isEnabled()) return list;

    for (const auto &role : CloudProviderCatalog::supportedRoles()) {
        const auto map = resolveRole(role);
        if (!map.isEmpty()) {
            list.append(map);
        }
    }
    return list;
}

QVariantMap CloudServiceManager::roleOverrides() const {
    QVariantMap map;
    for (auto it = m_roleOverrides.begin(); it != m_roleOverrides.end(); ++it) {
        map.insert(it.key(), it.value());
    }
    return map;
}

void CloudServiceManager::loadSettings() {
    QSettings settings;
    const QString rawJson = settings.value(kSettingsKey).toString();
    if (rawJson.isEmpty()) {
        // Defaults to proton
        m_defaultProvider = QStringLiteral("proton");
        m_nextcloudBaseUrl.clear();
        m_roleOverrides.clear();
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(rawJson.toUtf8());
    if (!doc.isObject()) {
        // Corrupted settings fallback safely to proton
        m_defaultProvider = QStringLiteral("proton");
        m_nextcloudBaseUrl.clear();
        m_roleOverrides.clear();
        return;
    }

    const QJsonObject obj = doc.object();
    const int version = obj.value(QStringLiteral("schemaVersion")).toInt(kSchemaVersion);
    if (version > kSchemaVersion) {
        // Unknown future version: fallback safely
        m_defaultProvider = QStringLiteral("proton");
        m_nextcloudBaseUrl.clear();
        m_roleOverrides.clear();
        return;
    }

    // Validate default provider
    const QString loadedDefault = obj.value(QStringLiteral("defaultProvider")).toString(QStringLiteral("proton"));
    if (loadedDefault == QStringLiteral("none") || CloudProviderCatalog::findProvider(loadedDefault).has_value()) {
        m_defaultProvider = loadedDefault;
    } else {
        m_defaultProvider = QStringLiteral("proton");
    }

    // Validate Nextcloud URL
    const QString loadedNcUrl = obj.value(QStringLiteral("nextcloudBaseUrl")).toString();
    QString sanitizedNc;
    if (sanitizeNextcloudUrl(loadedNcUrl, &sanitizedNc)) {
        m_nextcloudBaseUrl = sanitizedNc;
    } else {
        m_nextcloudBaseUrl.clear();
    }

    // Validate role overrides
    m_roleOverrides.clear();
    const QJsonObject overrides = obj.value(QStringLiteral("roleOverrides")).toObject();
    for (auto it = overrides.begin(); it != overrides.end(); ++it) {
        const QString role = it.key();
        const QString choice = it.value().toString();
        if (!CloudProviderCatalog::supportedRoles().contains(role)) continue;
        if (choice == QStringLiteral("disabled") || choice == QStringLiteral("inherit") || CloudProviderCatalog::providerSupportsRole(choice, role)) {
            if (choice != QStringLiteral("inherit")) {
                m_roleOverrides.insert(role, choice);
            }
        }
    }
}

void CloudServiceManager::saveSettings() {
    QJsonObject obj;
    obj.insert(QStringLiteral("schemaVersion"), kSchemaVersion);
    obj.insert(QStringLiteral("defaultProvider"), m_defaultProvider);
    obj.insert(QStringLiteral("nextcloudBaseUrl"), m_nextcloudBaseUrl);

    QJsonObject overrides;
    for (auto it = m_roleOverrides.begin(); it != m_roleOverrides.end(); ++it) {
        overrides.insert(it.key(), it.value());
    }
    obj.insert(QStringLiteral("roleOverrides"), overrides);

    QSettings settings;
    settings.setValue(kSettingsKey, QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
}
