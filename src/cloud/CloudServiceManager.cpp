#include "CloudServiceManager.h"
#include <QJsonDocument>
#include <QJsonObject>

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
    m_defaultProvider = providerId;
    saveSettings();
    emit configurationChanged();
}

bool CloudServiceManager::isEnabled() const {
    return m_defaultProvider != QStringLiteral("none");
}

QString CloudServiceManager::roleOverride(const QString &role) const {
    return m_roleOverrides.value(role, QStringLiteral("inherit"));
}

void CloudServiceManager::setRoleOverride(const QString &role, const QString &choice) {
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

QVariantMap CloudServiceManager::resolveRole(const QString &role) const {
    if (!isEnabled()) return {};

    const QString choice = roleOverride(role);
    if (choice == QStringLiteral("disabled")) return {};

    const QString providerId = (choice == QStringLiteral("inherit")) ? m_defaultProvider : choice;
    const auto serviceOpt = CloudProviderCatalog::findService(providerId, role);
    if (!serviceOpt) return {};

    const auto &s = *serviceOpt;
    QVariantMap map;
    map.insert(QStringLiteral("role"), s.role);
    map.insert(QStringLiteral("roleName"), CloudProviderCatalog::roleDisplayName(s.role));
    map.insert(QStringLiteral("name"), s.name);
    map.insert(QStringLiteral("url"), s.url.toString());
    map.insert(QStringLiteral("providerId"), providerId);
    map.insert(QStringLiteral("icon"), s.iconSvgName);
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
        m_roleOverrides.clear();
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(rawJson.toUtf8());
    if (!doc.isObject()) return;

    const QJsonObject obj = doc.object();
    m_defaultProvider = obj.value(QStringLiteral("defaultProvider")).toString(QStringLiteral("proton"));

    m_roleOverrides.clear();
    const QJsonObject overrides = obj.value(QStringLiteral("roleOverrides")).toObject();
    for (auto it = overrides.begin(); it != overrides.end(); ++it) {
        m_roleOverrides.insert(it.key(), it.value().toString());
    }
}

void CloudServiceManager::saveSettings() {
    QJsonObject obj;
    obj.insert(QStringLiteral("schemaVersion"), kSchemaVersion);
    obj.insert(QStringLiteral("defaultProvider"), m_defaultProvider);

    QJsonObject overrides;
    for (auto it = m_roleOverrides.begin(); it != m_roleOverrides.end(); ++it) {
        overrides.insert(it.key(), it.value());
    }
    obj.insert(QStringLiteral("roleOverrides"), overrides);

    QSettings settings;
    settings.setValue(kSettingsKey, QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
}
