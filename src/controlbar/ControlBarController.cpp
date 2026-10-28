#include "ControlBarController.h"
#include "../browserstore.h"
#include "../security/EffectivePermissionPolicy.h"
#include "../security/OsMediaAccess.h"
#include <QDateTime>

using Type = QWebEnginePermission::PermissionType;
using State = QWebEnginePermission::State;

ControlBarController::ControlBarController(QObject *parent) : QObject(parent), m_model(this) {
    connect(this, &ControlBarController::contextChanged, this, &ControlBarController::refresh);
    connect(&m_timer, &QTimer::timeout, this, &ControlBarController::refresh);
    m_timer.setSingleShot(true);
    m_timer.setInterval(60001);
    m_settings.remove("controlPolicies");
    refresh();
}
QObject *ControlBarController::store() const { return m_store.data(); }
void ControlBarController::setStore(QObject *object) {
    if (m_store) disconnect(m_store, nullptr, this, nullptr);
    m_store = qobject_cast<BrowserStore *>(object);
    if (m_store) connect(m_store, &BrowserStore::preferencesChanged, this, [this] {
        if (auto observer = m_profile ? m_store->observerFor(m_profile) : nullptr)
            connect(observer, &RequestObserver::activityAvailable, this, &ControlBarController::refresh, Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
        refresh();
    });
    refresh();
}
void ControlBarController::setProfile(QObject *object) {
    if (m_profile) { if (auto observer = m_store ? m_store->observerFor(m_profile) : nullptr) disconnect(observer, nullptr, this, nullptr); }
    m_profile = qobject_cast<QQuickWebEngineProfile *>(object);
    if (auto observer = m_store && m_profile ? m_store->observerFor(m_profile) : nullptr)
        connect(observer, &RequestObserver::activityAvailable, this, &ControlBarController::refresh, Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    refresh();
}
QString ControlBarController::origin(const QUrl &url) {
    if ((url.scheme() != "https" && url.scheme() != "http") || url.host().isEmpty()) return {};
    QUrl value; value.setScheme(url.scheme().toLower()); value.setHost(url.host().toLower());
    const int port = url.port();
    if (port != -1 && !(port == 443 && value.scheme() == "https") && !(port == 80 && value.scheme() == "http")) value.setPort(port);
    return value.toString();
}
QVariant ControlBarController::policyValue(const QUrl &url, const QString &key, const QVariant &fallback, const QString &tab) const {
    const auto subject = origin(url);
    if (subject.isEmpty() || !m_profile) return fallback;
    const auto storageKey = QString::fromLatin1(subject.toUtf8().toBase64(QByteArray::Base64UrlEncoding)) + "/" + key;
    return m_privatePolicies.value((tab.isEmpty() ? m_tabId : tab) + "/" + storageKey, fallback);
}
bool ControlBarController::writePolicy(const QUrl &url, const QString &key, const QVariant &value) {
    const auto subject = origin(url);
    if (subject.isEmpty() || !m_profile) return false;
    const auto storageKey = QString::fromLatin1(subject.toUtf8().toBase64(QByteArray::Base64UrlEncoding)) + "/" + key;
    m_privatePolicies[m_tabId + "/" + storageKey] = value;
    emit policyChanged(subject, key); refresh(); return true;
}
bool ControlBarController::settingFor(const QUrl &url, const QString &key, const QString &tab) const { return policyValue(url, key, true, tab).toBool(); }
int ControlBarController::popupMode(const QUrl &url, const QString &tab) const { return policyValue(url, "popups", 0, tab).toInt(); }
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
bool ControlBarController::setPermissionPolicy(const QString &key, int mode, const QUrl &targetUrl) {
    // Never manufacture a grant through queryPermission: only a live request can grant.
    if (typeFor(key) == Type::Unsupported || (mode != 0 && mode != 2)) return false;
    const auto effectiveTarget = (targetUrl.isValid() && !targetUrl.isEmpty()) ? targetUrl : m_url;
    return writePolicy(effectiveTarget, key, mode);
}

bool ControlBarController::shouldDenyOrigin(const QString &key, const QUrl &requester, const QUrl &topUrl, const QString &tab) const {
    const auto effectiveTop = topUrl.isEmpty() ? m_url : topUrl;
    const auto effectiveTab = tab.isEmpty() ? m_tabId : tab;
    const int topMode = policyValue(effectiveTop, key, 0, effectiveTab).toInt();
    const int reqMode = policyValue(requester, key, 0, effectiveTab).toInt();
    const auto eval = EffectivePermissionPolicy::evaluate(key, effectiveTop, requester, topMode, reqMode);
    return eval.decision == PermissionDecision::Deny;
}
bool ControlBarController::shouldDeny(const QWebEnginePermission &permission, const QUrl &topUrl, const QString &tab) const {
    const auto effectiveTop = topUrl.isEmpty() ? m_url : topUrl;
    const auto effectiveTab = tab.isEmpty() ? m_tabId : tab;
    const auto keys = keysFor(permission.permissionType());
    const auto eval = EffectivePermissionPolicy::evaluateCompound(keys, effectiveTop, permission.origin(),
        [this, &effectiveTab](const QString &k, const QUrl &originUrl) {
            return policyValue(originUrl, k, 0, effectiveTab).toInt();
        });
    return eval.decision == PermissionDecision::Deny;
}
void ControlBarController::notePermission(const QWebEnginePermission &permission, const QString &tab, const QUrl &top, bool blocked, quint64 requestId) {
    for (const auto &key : keysFor(permission.permissionType())) {
        m_aggregator.recordPermission(key, tab, origin(top), origin(permission.origin()), !blocked, blocked, requestId);
    }
    refresh();
}
void ControlBarController::finishPermission(const QWebEnginePermission &permission, const QString &tab, bool allowed, quint64 requestId) {
    for (const auto &key : keysFor(permission.permissionType())) {
        m_aggregator.finishPermission(key, tab, origin(permission.origin()), allowed, requestId);
    }
    refresh();
}
void ControlBarController::noteBlockedRequest(const QString &key, const QString &tab, const QUrl &url, const QUrl &requester) {
    static const QStringList permissionKeys{"camera", "mic", "screen", "location", "notifications", "clipboard"};
    if (!permissionKeys.contains(key)) return;
    m_aggregator.recordPermission(key, tab, origin(url), origin(requester.isEmpty() ? url : requester), false, true);
    refresh();
}
void ControlBarController::notePopup(const QString &tab, const QUrl &url, bool blocked) {
    m_aggregator.recordPopup(tab, origin(url), blocked);
    refresh();
}
void ControlBarController::abandonPending(const QString &tab) {
    m_aggregator.abandonPending(tab);
    refresh();
}
void ControlBarController::expireContext(const QString &tab) {
    for (const auto &key : m_privatePolicies.keys()) if (key.startsWith(tab + "/")) m_privatePolicies.remove(key);
    m_aggregator.expireTab(tab);
    refresh();
}
void ControlBarController::clearPermissionPolicies() {
    m_privatePolicies.clear();
    m_settings.remove("controlPolicies"); // Migrate obsolete persistent origin privileges.
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
    m_aggregator.pruneExpired(60000);
    const auto sample = networkSample();
    QVariantMap networkCounts{{"transport",0}, {"ads",0}, {"trackers",0}, {"exceptions",0}, {"other",0}};
    for (const auto &item : sample) {
        const auto event = item.toMap();
        const auto decision = event.value("decision").toString();
        const QString key = (decision == "tracker-exception" || decision == "ad-exception") ? "exceptions"
            : event.value("blocked").toBool() ? event.value("category").toString() : QString();
        if (networkCounts.contains(key)) networkCounts[key] = networkCounts[key].toInt() + 1;
    }
    for (int i = 0; i < keys.size(); ++i) {
        const auto key = keys[i];
        PolicyState state = PolicyState::Unavailable;
        QString scope = tr("Cet onglet · session temporaire");
        QString reason = valid ? tr("Utilisation réelle non observable. Aucun indicateur d’enregistrement.") : tr("Ouvrez un site HTTP ou HTTPS pour gérer ses permissions.");
        bool canAct = valid, persistent = false, explicitDeny = false;
        if (valid && i < 6 && key != "screen") {
            state = grantedKeys.contains(key) ? PolicyState::Allow : PolicyState::Deny;
            explicitDeny = policyValue(m_url, key, 0).toInt() == 2;
            persistent = false;
        }
        if ((key == "camera" || key == "mic") && valid && !OsMediaAccess::allowed(key))
            reason = key == "camera" ? tr("Caméra désactivée dans les paramètres de confidentialité de Windows. Les demandes sont refusées sans autorisation.")
                                     : tr("Microphone désactivé dans les paramètres de confidentialité de Windows. Les demandes sont refusées sans autorisation.");
        if (key == "screen") { canAct = false; reason = tr("Sélecteur de capture non intégré. Les demandes sont refusées, sans prétendre observer un partage."); }
        if (key == "notifications" && valid) reason = tr("Permission du site uniquement. Présentation des notifications sur le bureau non intégrée.");
        if (key == "clipboard" && valid) reason = tr("Autorisation Qt de lecture/écriture. Tous les accès au presse-papiers ne sont pas observables.");
        if (key == "javascript" || key == "storage") {
            const bool actual = key == "javascript" ? javascriptEnabled : storageEnabled;
            state = valid ? (reloadRequired ? PolicyState::ReloadRequired : actual ? PolicyState::Allow : PolicyState::Deny) : PolicyState::Unavailable;
            explicitDeny = valid && !actual;
            reason = key == "javascript" ? tr("Politique par origine, appliquée à la navigation. Recharger pour retirer les effets des scripts déjà exécutés.") : tr("Contrôle de localStorage uniquement. Aucun comptage ni effacement complet d’IndexedDB ou des service workers.");
            scope = tr("Ce site · paramètres de la vue");
        }
        if (key == "popups" && valid) {
            const auto mode = popupMode(m_url);
            state = mode == 1 ? PolicyState::Allow : PolicyState::Deny;
            explicitDeny = mode == 2;
            reason = mode == 0 ? tr("Seuls les liens ouverts par un geste explicite deviennent des onglets.") : tr("Politique des nouvelles fenêtres de ce site. Aucun comptage des tentatives supprimées en interne par Chromium.");
        }
        if (key == "cookies") {
            state = m_profile ? PolicyState::Ask : PolicyState::Unavailable; canAct = bool(m_store);
            scope = tr("Tous les profils ouverts");
            reason = m_store && m_store->blockThirdPartyCookies() ? tr("Cookies tiers bloqués par le filtre du profil. Cookies internes autorisés. Pas de comptage exhaustif.") : tr("Cookies tiers autorisés dans ce profil. Pas de comptage exhaustif.");
        }
        const auto summary = m_aggregator.summarize(key, m_tabId, subject, 60000);
        bool pending = summary.pending;
        bool recent = summary.recent;
        bool blocked = summary.blocked;
        QString requester = summary.requester;
        QVariantList eventRows = summary.events;
        if (key == "network") {
            state = m_profile ? PolicyState::Ask : PolicyState::Unavailable; canAct = false;
            scope = tr("Profil entier · observation seulement");
            if (m_store && m_store->isTemporaryHttpProfile(m_profile)) scope = tr("Profil temporaire réservé à cet onglet");
            reason = tr("Échantillon des décisions du filtre, pas une preuve de chargement réussi. Origines et types uniquement, sans contenu ni chemin d’adresse. Dans un profil partagé, ces événements ne sont pas attribuables à un onglet.");
            eventRows.clear();
            QHash<QString,int> categoryRows;
            for (int j = sample.size() - 1; j >= 0; --j) {
                const auto event = sample[j].toMap();
                const auto category = event.value("category").toString();
                if (categoryRows.value(category) < 8) { eventRows.append(event); ++categoryRows[category]; }
                blocked |= event.value("blocked").toBool();
            }
            recent = !eventRows.isEmpty();
        }
        rows.append(QVariantMap{{"key", key}, {"title", titles[i]}, {"policy", int(state)}, {"explicitDeny", explicitDeny}, {"scope", scope}, {"reason", reason}, {"origin", subject},
                                {"persistent", persistent}, {"canAct", canAct}, {"pending", pending}, {"recent", recent}, {"blocked", blocked}, {"requester", requester}, {"events", eventRows},
                                {"sampleCount", key == "network" ? sample.size() : eventRows.size()}, {"networkCounts", key == "network" ? networkCounts : QVariantMap()}});
    }
    if ((!m_aggregator.isEmpty() || !sample.isEmpty()) && !m_timer.isActive()) m_timer.start();
    if (m_model.update(rows)) { ++m_revision; emit changed(); }
}
