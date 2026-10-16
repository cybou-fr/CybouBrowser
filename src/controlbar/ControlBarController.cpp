#include "ControlBarController.h"
#include "../browserstore.h"
#include <QDateTime>

using Type = QWebEnginePermission::PermissionType;
using State = QWebEnginePermission::State;

ControlBarController::ControlBarController(QObject *parent) : QObject(parent), m_model(this) {
    connect(this, &ControlBarController::contextChanged, this, &ControlBarController::refresh);
    connect(&m_timer, &QTimer::timeout, this, &ControlBarController::refresh);
    m_timer.start(250);
    refresh();
}
QObject *ControlBarController::store() const { return m_store.data(); }
void ControlBarController::setStore(QObject *object) {
    if (m_store) disconnect(m_store, nullptr, this, nullptr);
    m_store = qobject_cast<BrowserStore *>(object);
    if (m_store) connect(m_store, &BrowserStore::preferencesChanged, this, &ControlBarController::refresh);
    refresh();
}
void ControlBarController::setProfile(QObject *object) { m_profile = qobject_cast<QQuickWebEngineProfile *>(object); refresh(); }
QString ControlBarController::origin(const QUrl &url) {
    if (url.scheme() != "https" && url.scheme() != "http") return {};
    QUrl value; value.setScheme(url.scheme().toLower()); value.setHost(url.host().toLower());
    const int port = url.port();
    if (port != -1 && !(port == 443 && value.scheme() == "https") && !(port == 80 && value.scheme() == "http")) value.setPort(port);
    return value.toString();
}
QVariant ControlBarController::policyValue(const QUrl &url, const QString &key, const QVariant &fallback) const {
    const auto subject = origin(url);
    if (subject.isEmpty() || !m_profile) return fallback;
    const auto storageKey = QString::fromLatin1(subject.toUtf8().toBase64(QByteArray::Base64UrlEncoding)) + "/" + key;
    if (m_profile->isOffTheRecord()) return m_privatePolicies.value(storageKey, fallback);
    return m_settings.value("controlPolicies/" + storageKey, fallback);
}
bool ControlBarController::writePolicy(const QUrl &url, const QString &key, const QVariant &value) {
    const auto subject = origin(url);
    if (subject.isEmpty() || !m_profile) return false;
    const auto storageKey = QString::fromLatin1(subject.toUtf8().toBase64(QByteArray::Base64UrlEncoding)) + "/" + key;
    if (m_profile->isOffTheRecord()) m_privatePolicies[storageKey] = value;
    else {
        const auto fullKey = "controlPolicies/" + storageKey;
        const bool existed = m_settings.contains(fullKey);
        const auto previous = m_settings.value(fullKey);
        m_settings.setValue(fullKey, value); m_settings.sync();
        if (m_settings.status() != QSettings::NoError) {
            if (existed) m_settings.setValue(fullKey, previous); else m_settings.remove(fullKey);
            return false;
        }
    }
    emit policyChanged(subject, key); refresh(); return true;
}
bool ControlBarController::settingFor(const QUrl &url, const QString &key) const { return policyValue(url, key, true).toBool(); }
int ControlBarController::popupMode(const QUrl &url) const { return policyValue(url, "popups", 0).toInt(); }
bool ControlBarController::setBehavior(const QString &key, int mode) {
    if (key == "popups" && mode >= 0 && mode <= 2) return writePolicy(m_url, key, mode);
    if ((key == "javascript" || key == "storage") && (mode == 1 || mode == 2)) return writePolicy(m_url, key, mode == 1);
    return false;
}
Type ControlBarController::typeFor(const QString &key) {
    if (key == "camera") return Type::MediaVideoCapture;
    if (key == "mic") return Type::MediaAudioCapture;
    if (key == "location") return Type::Geolocation;
    if (key == "notifications") return Type::Notifications;
    if (key == "clipboard") return Type::ClipboardReadWrite;
    return Type::Unsupported;
}
QStringList ControlBarController::keysFor(Type type) {
    switch (type) {
    case Type::MediaVideoCapture: return {"camera"};
    case Type::MediaAudioCapture: return {"mic"};
    case Type::MediaAudioVideoCapture: return {"camera", "mic"};
    case Type::Geolocation: return {"location"};
    case Type::Notifications: return {"notifications"};
    case Type::ClipboardReadWrite: return {"clipboard"};
    case Type::DesktopVideoCapture: case Type::DesktopAudioVideoCapture: return {"screen"};
    default: return {};
    }
}
bool ControlBarController::setPermissionPolicy(const QString &key, int mode) {
    const auto type = typeFor(key);
    if (!m_profile || origin(m_url).isEmpty() || type == Type::Unsupported || mode < 0 || mode > 2) return false;
    if (!QWebEnginePermission::isPersistent(type)) return mode != 1 && writePolicy(m_url, key, mode);
    auto permission = m_profile->queryPermission(QUrl(origin(m_url)), type);
    if (!permission.isValid()) return false;
    if (mode == 0) permission.reset(); else if (mode == 1) permission.grant(); else permission.deny();
    const auto state = m_profile->queryPermission(QUrl(origin(m_url)), type).state();
    refresh(); return state == (mode == 0 ? State::Ask : mode == 1 ? State::Granted : State::Denied);
}
bool ControlBarController::shouldDeny(const QWebEnginePermission &permission) const {
    for (const auto &key : keysFor(permission.permissionType()))
        if (key == "screen" || policyValue(permission.origin(), key, 0).toInt() == 2) return true;
    return false;
}
void ControlBarController::notePermission(const QWebEnginePermission &permission, const QString &tab, const QUrl &top, bool blocked) {
    for (const auto &key : keysFor(permission.permissionType())) {
        m_observations.append({key, tab, origin(top), origin(permission.origin()), QDateTime::currentMSecsSinceEpoch(), !blocked, blocked});
        if (m_observations.size() > 256) m_observations.removeFirst();
    }
    refresh();
}
void ControlBarController::finishPermission(const QWebEnginePermission &permission, const QString &tab, bool allowed) {
    for (auto &event : m_observations) if (event.pending && event.tab == tab && event.requester == origin(permission.origin()) && keysFor(permission.permissionType()).contains(event.key)) {
        event.pending = false; event.blocked = !allowed; event.time = QDateTime::currentMSecsSinceEpoch();
    }
    refresh();
}
void ControlBarController::notePopup(const QString &tab, const QUrl &url, bool blocked) {
    m_observations.append({"popups", tab, origin(url), origin(url), QDateTime::currentMSecsSinceEpoch(), false, blocked});
    if (m_observations.size() > 256) m_observations.removeFirst();
    refresh();
}
void ControlBarController::abandonPending(const QString &tab) {
    for (auto &event : m_observations) if (event.tab == tab) event.pending = false;
    refresh();
}
void ControlBarController::clearPermissionPolicies() {
    if (!m_profile) return;
    if (m_profile->isOffTheRecord()) {
        for (const auto &key : m_privatePolicies.keys()) if (key.endsWith("/camera") || key.endsWith("/mic")) m_privatePolicies.remove(key);
    } else {
        m_settings.beginGroup("controlPolicies");
        for (const auto &key : m_settings.allKeys()) if (key.endsWith("/camera") || key.endsWith("/mic")) m_settings.remove(key);
        m_settings.endGroup(); m_settings.sync();
    }
    refresh();
}
QVariantList ControlBarController::networkSample() const {
    auto observer = m_store && m_profile ? m_store->observerFor(m_profile) : nullptr;
    return observer ? observer->sample() : QVariantList();
}
void ControlBarController::refresh() {
    const QStringList keys{"camera", "mic", "screen", "location", "notifications", "clipboard", "javascript", "popups", "cookies", "storage", "network"};
    const QStringList titles{tr("Caméra"), tr("Microphone"), tr("Partage d’écran"), tr("Géolocalisation"), tr("Notifications"), tr("Presse-papiers"), tr("JavaScript"), tr("Fenêtres surgissantes"), tr("Cookies"), tr("Stockage du site"), tr("Réseau")};
    QVariantList rows;
    const auto subject = origin(m_url);
    const bool valid = m_profile && !subject.isEmpty();
    const auto now = QDateTime::currentMSecsSinceEpoch();
    const auto sample = networkSample();
    for (int i = 0; i < keys.size(); ++i) {
        const auto key = keys[i];
        PolicyState state = PolicyState::Unavailable;
        QString scope = tr("Ce site · profil");
        QString reason = valid ? tr("Utilisation réelle non observable. Aucun indicateur d’enregistrement.") : tr("Ouvrez un site HTTP ou HTTPS pour gérer ses permissions.");
        bool canAct = valid, persistent = false;
        if (valid && i < 6 && key != "screen") {
            const auto type = typeFor(key); persistent = QWebEnginePermission::isPersistent(type);
            if (persistent) {
                const auto native = m_profile->queryPermission(QUrl(subject), type).state();
                state = native == State::Granted ? PolicyState::Allow : native == State::Denied ? PolicyState::Deny : PolicyState::Ask;
            } else state = policyValue(m_url, key, 0).toInt() == 2 ? PolicyState::Deny : PolicyState::Ask;
        }
        if (key == "screen") { canAct = false; reason = tr("Sélecteur de capture non intégré. Les demandes sont refusées, sans prétendre observer un partage."); }
        if (key == "notifications" && valid) reason = tr("Permission du site uniquement. Présentation des notifications sur le bureau non intégrée.");
        if (key == "clipboard" && valid) reason = tr("Autorisation Qt de lecture/écriture. Tous les accès au presse-papiers ne sont pas observables.");
        if (key == "javascript" || key == "storage") {
            const bool actual = key == "javascript" ? javascriptEnabled : storageEnabled;
            state = valid ? (reloadRequired ? PolicyState::ReloadRequired : actual ? PolicyState::Allow : PolicyState::Deny) : PolicyState::Unavailable;
            reason = key == "javascript" ? tr("Politique par origine, appliquée à la navigation. Recharger pour retirer les effets des scripts déjà exécutés.") : tr("Contrôle de localStorage uniquement. Aucun comptage ni effacement complet d’IndexedDB ou des service workers.");
            scope = tr("Ce site · paramètres de la vue");
        }
        if (key == "popups" && valid) {
            const auto mode = popupMode(m_url);
            state = mode == 2 ? PolicyState::Deny : mode == 1 ? PolicyState::Allow : PolicyState::Ask;
            reason = mode == 0 ? tr("Seuls les liens ouverts par un geste explicite deviennent des onglets.") : tr("Politique des nouvelles fenêtres de ce site. Aucun comptage des tentatives supprimées en interne par Chromium.");
        }
        if (key == "cookies") {
            state = m_profile ? PolicyState::Ask : PolicyState::Unavailable; canAct = bool(m_store);
            scope = tr("Tous les profils ouverts");
            reason = m_store && m_store->blockThirdPartyCookies() ? tr("Cookies tiers bloqués par le filtre du profil. Cookies internes autorisés. Pas de comptage exhaustif.") : tr("Cookies tiers autorisés dans ce profil. Pas de comptage exhaustif.");
        }
        QVariantList eventRows;
        bool pending = false, recent = false, blocked = false;
        QString requester;
        for (auto it = m_observations.crbegin(); it != m_observations.crend() && eventRows.size() < 8; ++it) {
            if (it->key != key || it->tab != m_tabId || it->top != subject || (!it->pending && now - it->time > 60000)) continue;
            eventRows.append(QVariantMap{{"origin", it->requester}, {"time", it->time}, {"blocked", it->blocked}});
            pending |= it->pending; recent = true; blocked |= it->blocked;
            if (requester.isEmpty()) requester = it->requester;
        }
        if (key == "network") {
            state = m_profile ? PolicyState::Ask : PolicyState::Unavailable; canAct = false;
            scope = tr("Profil entier · observation seulement");
            reason = tr("Échantillon borné de requêtes HTTP(S) : origine, type et heure. Aucun contenu, secret, volume ni attribution certaine à cet onglet.");
            eventRows.clear();
            for (int j = sample.size() - 1; j >= 0 && eventRows.size() < 12; --j) eventRows.append(sample[j]);
            recent = !eventRows.isEmpty();
        }
        rows.append(QVariantMap{{"key", key}, {"title", titles[i]}, {"policy", int(state)}, {"scope", scope}, {"reason", reason}, {"origin", subject},
                                {"persistent", persistent}, {"canAct", canAct}, {"pending", pending}, {"recent", recent}, {"blocked", blocked}, {"requester", requester}, {"events", eventRows},
                                {"sampleCount", key == "network" ? sample.size() : eventRows.size()}});
    }
    if (m_model.update(rows)) { ++m_revision; emit changed(); }
}
