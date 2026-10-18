#pragma once
#include <QObject>
#include <QTimer>
#include <QSettings>
#include <QVariantList>
#include <QDateTime>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QPointer>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QGuiApplication>
#include <QStyleHints>
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
#include <QAccessibilityHints>
#endif
#include <QHostAddress>
#include <optional>
#include <memory>
#include <functional>
#include <QtWebEngineQuick/QQuickWebEngineProfile>
#include <QtWebEngineCore/QWebEngineCookieStore>
#include "controlbar/RequestObserver.h"
#include "security/NavigationGuard.h"
#include "security/StoredUrl.h"
#include "security/BrowserSettings.h"
#include "security/DownloadPolicy.h"
#include "cloud/CloudServiceManager.h"
#include "ui/AddressDisplay.h"

class BrowserStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList bookmarks READ bookmarks NOTIFY changed)
    Q_PROPERTY(QVariantList history READ history NOTIFY changed)
    Q_PROPERTY(QString downloadsPath READ downloadsPath NOTIFY preferencesChanged)
    Q_PROPERTY(bool askDownloadLocation READ askDownloadLocation WRITE setAskDownloadLocation NOTIFY preferencesChanged)
    Q_PROPERTY(QString searchEngine READ searchEngine WRITE setSearchEngine NOTIFY preferencesChanged)
    Q_PROPERTY(bool restoreSession READ restoreSession WRITE setRestoreSession NOTIFY preferencesChanged)
    Q_PROPERTY(bool blockThirdPartyCookies READ blockThirdPartyCookies WRITE setBlockThirdPartyCookies NOTIFY preferencesChanged)
    Q_PROPERTY(bool cacheBusy READ cacheBusy NOTIFY cacheBusyChanged)
    Q_PROPERTY(bool httpsOnly READ httpsOnly WRITE setHttpsOnly NOTIFY preferencesChanged)
    Q_PROPERTY(bool httpsFirst READ httpsFirst WRITE setHttpsFirst NOTIFY preferencesChanged)
    Q_PROPERTY(bool strictWebRtc READ strictWebRtc WRITE setStrictWebRtc NOTIFY preferencesChanged)
    Q_PROPERTY(bool strictWebRtcActive READ strictWebRtcActive NOTIFY preferencesChanged)
    Q_PROPERTY(QString controlBarPosition READ controlBarPosition WRITE setControlBarPosition NOTIFY preferencesChanged)
    Q_PROPERTY(bool reduceMotion READ reduceMotion WRITE setReduceMotion NOTIFY preferencesChanged)
    Q_PROPERTY(bool highContrast READ highContrast NOTIFY preferencesChanged)
    Q_PROPERTY(bool disableGlow READ disableGlow WRITE setDisableGlow NOTIFY preferencesChanged)
    Q_PROPERTY(QString controlBarLanguage READ controlBarLanguage WRITE setControlBarLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString wallpaperType READ wallpaperType WRITE setWallpaperType NOTIFY preferencesChanged)
    Q_PROPERTY(QString customWallpaperPath READ customWallpaperPath WRITE setCustomWallpaperPath NOTIFY preferencesChanged)
    Q_PROPERTY(int glassBlurLevel READ glassBlurLevel WRITE setGlassBlurLevel NOTIFY preferencesChanged)
    Q_PROPERTY(bool blockTrackers READ blockTrackers WRITE setBlockTrackers NOTIFY preferencesChanged)
    Q_PROPERTY(QObject* cloud READ cloud CONSTANT)
public:
    struct TemporaryProfileContext {
        QQuickWebEngineProfile *profile = nullptr;
        QObject *owner = nullptr; // Owns the profile; prototype factories retain their native ownership.
    };
    using TemporaryProfileFactory = std::function<TemporaryProfileContext(QObject *)>;
    explicit BrowserStore(QObject *parent = nullptr, TemporaryProfileFactory factory = {})
        : QObject(parent), m_cloud(this), m_temporaryProfileFactory(std::move(factory)) {
        settings.remove("preferences/httpsOnly"); // Retire the old persistent global HTTP opt-out.
        migrateStoredUrls();
        m_historySaveTimer.setInterval(1000);
        m_historySaveTimer.setSingleShot(true);
        connect(&m_historySaveTimer, &QTimer::timeout, this, &BrowserStore::flushHistory);
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        connect(QGuiApplication::styleHints()->accessibility(), &QAccessibilityHints::contrastPreferenceChanged, this, &BrowserStore::preferencesChanged);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 12, 0)
        connect(QGuiApplication::styleHints()->accessibility(), &QAccessibilityHints::motionPreferenceChanged, this, &BrowserStore::preferencesChanged);
#endif
    }
    ~BrowserStore() override {
        flushHistory();
    }
    // One-time cleanup of URLs stored by earlier versions (fragments, credentials, secret query values).
    void migrateStoredUrls() {
        if (settings.value("privacy/storedUrlsSanitized1", false).toBool()) return;
        for (const char *key : {"history", "bookmarks"}) {
            auto list = settings.value(key).toList();
            bool changed = false;
            for (auto &item : list) {
                auto row = item.toMap();
                const QString url = row.value("url").toString(), clean = StoredUrl::sanitize(url);
                if (clean == url) continue;
                if (row.value("title").toString() == url) row["title"] = clean; // title defaulted to the URL
                row["url"] = clean; item = row; changed = true;
            }
            if (changed) settings.setValue(key, list);
        }
        settings.setValue("privacy/storedUrlsSanitized1", true);
        settings.sync();
    }
    QVariantList bookmarks() const { return settings.value("bookmarks").toList(); }
    QVariantList history() const {
        if (!m_historyCache.has_value()) {
            m_historyCache = settings.value("history").toList();
        }
        return *m_historyCache;
    }
    Q_INVOKABLE void flushHistory() {
        if (m_historyDirty && m_historyCache.has_value()) {
            settings.setValue("history", *m_historyCache);
            settings.sync();
            m_historyDirty = false;
        }
    }
    QString downloadsPath() const { return settings.value("preferences/downloadsPath", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString(); }
    bool askDownloadLocation() const { return settings.value("preferences/askDownloadLocation", false).toBool(); }
    void setAskDownloadLocation(bool value) { settings.setValue("preferences/askDownloadLocation", value); emit preferencesChanged(); }
    Q_INVOKABLE bool setDownloadFolder(const QUrl &url) {
        const QFileInfo folder(url.toLocalFile());
        if (!url.isLocalFile() || !folder.isDir() || !folder.isWritable()) return false;
        settings.setValue("preferences/downloadsPath", folder.absoluteFilePath()); emit preferencesChanged(); return true;
    }
    Q_INVOKABLE QUrl fileUrl(const QString &path) const { return QUrl::fromLocalFile(path); }
    Q_INVOKABLE QString localPath(const QUrl &url) const { return url.isLocalFile() ? url.toLocalFile() : QString(); }
    Q_INVOKABLE QString displayHost(const QUrl &url) const { return AddressDisplay::host(url); }
    Q_INVOKABLE QString fileName(const QUrl &url) const { return QFileInfo(url.toLocalFile()).fileName(); }
    Q_INVOKABLE QString fileDirectory(const QUrl &url) const { return QFileInfo(url.toLocalFile()).absolutePath(); }
    Q_INVOKABLE QString safeFileName(QString title) const {
        title.replace(QRegularExpression("[<>:\"/\\\\|?*\\x00-\\x1f]"), "_");
        title = title.trimmed().left(100);
        while (title.endsWith('.') || title.endsWith(' ')) title.chop(1);
        return title.isEmpty() ? "CYBOU-page" : "CYBOU-" + title;
    }
    Q_INVOKABLE QString nextDownloadPath(const QString &directory, const QString &name) const {
        const QFileInfo file(QFileInfo(name).fileName());
        const auto base = file.completeBaseName().isEmpty() ? "download" : file.completeBaseName();
        const auto suffix = file.suffix().isEmpty() ? QString() : "." + file.suffix();
        QDir dir(directory);
        auto candidate = dir.filePath(base + suffix);
        for (int i = 1; QFileInfo::exists(candidate); ++i) candidate = dir.filePath(base + " (" + QString::number(i) + ")" + suffix);
        return candidate;
    }
    Q_INVOKABLE QString openableFile(const QUrl &url) const {
        const QFileInfo file(url.toLocalFile());
        if (!url.isLocalFile() || !file.isFile() || !QStringList{"html", "htm", "txt", "pdf", "png", "jpg", "jpeg", "gif", "webp", "svg"}.contains(file.suffix().toLower())) return {};
        return QUrl::fromLocalFile(file.canonicalFilePath()).toString();
    }
    Q_INVOKABLE bool canOpenDownloadedFile(const QString &path) const {
        const QFileInfo file(path);
        return file.isFile() && !file.isSymLink() && DownloadPolicy::allowsExternalOpen(file.fileName());
    }
    Q_INVOKABLE bool openDownloadedFile(const QString &path) {
        // Recheck at launch time, even when the UI has already reviewed the file.
        return canOpenDownloadedFile(path) && QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).canonicalFilePath()));
    }
    Q_INVOKABLE void showFileFolder(const QString &path) { QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath())); }
    QString searchEngine() const { return settings.value("preferences/searchEngine", "DuckDuckGo").toString(); }
    bool restoreSession() const { return settings.value("preferences/restoreSession", true).toBool(); }
    bool blockThirdPartyCookies() const { return settings.value("preferences/blockThirdPartyCookies", true).toBool(); }
    bool cacheBusy() const { return m_cacheBusy; }
    bool httpsOnly() const { return !m_testHttp; }
    void allowTestHttpFixture() { m_allowTestHttp = true; }
    void setHttpsOnly(bool enabled) {
        if (!enabled && !m_allowTestHttp) return;
        if (enabled == httpsOnly()) return;
        m_testHttp = !enabled;
        for (auto observer : observers) observer->setHttpsOnly(enabled);
        emit preferencesChanged();
    }
    // Opt-in, off by default. The Chromium switch is process-wide and read before the engine starts,
    // so the saved preference applies after a restart; strictWebRtcActive reports what this process runs with.
    bool strictWebRtc() const { return settings.value("preferences/strictWebRtc", false).toBool(); }
    void setStrictWebRtc(bool enabled) {
        if (enabled == strictWebRtc()) return;
        settings.setValue("preferences/strictWebRtc", enabled);
        emit preferencesChanged();
    }
    bool strictWebRtcActive() const { return m_strictWebRtcActive; }
    void setStrictWebRtcActive(bool active) { m_strictWebRtcActive = active; emit preferencesChanged(); }
    bool httpsFirst() const { return settings.value("preferences/httpsFirst", true).toBool(); }
    void setHttpsFirst(bool enabled) {
        if (enabled == httpsFirst()) return;
        settings.setValue("preferences/httpsFirst", enabled);
        emit preferencesChanged();
    }
    Q_INVOKABLE bool isLocalHost(const QUrl &url) const {
        return NavigationGuard::isLocalHost(url);
    }
    Q_INVOKABLE bool isPrivateNetwork(const QUrl &url) const {
        return NavigationGuard::isPrivateNetwork(url);
    }
    Q_INVOKABLE bool requiresRestrictedBrowsing(const QUrl &url) const {
        return NavigationGuard::requiresRestrictedBrowsing(url);
    }
    Q_INVOKABLE QString restrictedBrowsingReason(const QUrl &url) const {
        return NavigationGuard::restrictedBrowsingReason(url);
    }
    Q_INVOKABLE bool shouldUpgradeToHttps(const QUrl &url) const {
        if (!httpsFirst()) return false;
        if (url.scheme().compare("http", Qt::CaseInsensitive) != 0) return false;
        if (isLocalHost(url)) return false;
        return true;
    }
    Q_INVOKABLE QUrl upgradeToHttps(const QUrl &url) const {
        QUrl upgraded = url;
        upgraded.setScheme("https");
        if (upgraded.port() == 80) upgraded.setPort(-1);
        return upgraded;
    }
    Q_INVOKABLE bool canRequestTemporaryHttp(const QUrl &url) const {
        return url.scheme() == "http" && !url.host().isEmpty() && url.userInfo().isEmpty();
    }
    Q_INVOKABLE QObject *createTemporaryHttpProfile(const QUrl &url) {
        if (!canRequestTemporaryHttp(url)) return nullptr;
        auto context = m_temporaryProfileFactory ? m_temporaryProfileFactory(this) : TemporaryProfileContext{};
        if (!m_temporaryProfileFactory) { context.profile = new QQuickWebEngineProfile(this); context.owner = context.profile; }
        auto profile = context.profile;
        if (!profile || !context.owner) return nullptr;
        if (!profile->isOffTheRecord()) { context.owner->deleteLater(); return nullptr; }
        profile->setPersistentPermissionsPolicy(QQuickWebEngineProfile::PersistentPermissionsPolicy::AskEveryTime);
        configureProfile(profile);
        auto observer = observers.value(profile);
        observer->setHttpsOnly(true);
        if (!observer->allowHttpNavigation(url)) { context.owner->deleteLater(); return nullptr; }
        m_temporaryProfileOwners.insert(profile, context.owner);
        m_temporaryHttpProfiles.insert(profile);
        return profile;
    }
    Q_INVOKABLE bool isTemporaryHttpProfile(QObject *object) const {
        return m_temporaryHttpProfiles.contains(qobject_cast<QQuickWebEngineProfile *>(object));
    }
    Q_INVOKABLE bool canStartTemporaryHttp(QObject *object, const QUrl &url) const {
        const auto profile = qobject_cast<QQuickWebEngineProfile *>(object);
        const auto observer = observers.value(profile);
        return m_temporaryHttpProfiles.contains(profile) && observer && observer->canStartHttpNavigation(url);
    }
    Q_INVOKABLE bool canRequestTemporaryTrackers(const QUrl &url) const {
        const bool fixture = QCoreApplication::arguments().contains("--tracker-test") && isLocalHost(url) && url.scheme() == "http";
        return (url.scheme() == "https" || fixture) && !url.host().isEmpty() && url.userInfo().isEmpty();
    }
    Q_INVOKABLE bool canRequestTemporaryAds(const QUrl &url) const {
        return canRequestTemporaryTrackers(url);
    }
    Q_INVOKABLE bool allowTrackersInProfile(QObject *object, const QUrl &url, QObject *owner) {
        if (!canRequestTemporaryTrackers(url)) return false;
        const bool fixture = QCoreApplication::arguments().contains("--tracker-test") && isLocalHost(url) && url.scheme() == "http";
        const auto profile = qobject_cast<QQuickWebEngineProfile *>(object);
        auto observer = observers.value(profile);
        const bool ok = observer && observer->allowTrackersForOrigin(url, owner, fixture);
        if (ok) emit trackerExceptionsChanged();
        return ok;
    }
    Q_INVOKABLE bool allowAdsInProfile(QObject *object, const QUrl &url, QObject *owner) {
        if (!canRequestTemporaryAds(url)) return false;
        const bool fixture = QCoreApplication::arguments().contains("--tracker-test") && isLocalHost(url) && url.scheme() == "http";
        const auto profile = qobject_cast<QQuickWebEngineProfile *>(object);
        auto observer = observers.value(profile);
        const bool ok = observer && observer->allowAdsForOrigin(url, owner, fixture);
        if (ok) emit trackerExceptionsChanged();
        return ok;
    }
    Q_INVOKABLE bool isTrackerAllowedInProfile(QObject *object, const QUrl &url) const {
        const auto profile = qobject_cast<QQuickWebEngineProfile *>(object);
        auto observer = observers.value(profile);
        return observer && observer->isTrackerAllowed(url);
    }
    Q_INVOKABLE bool isAdAllowedInProfile(QObject *object, const QUrl &url) const {
        const auto profile = qobject_cast<QQuickWebEngineProfile *>(object);
        auto observer = observers.value(profile);
        return observer && observer->isAdAllowed(url);
    }
    Q_INVOKABLE bool revokeTrackersForOrigin(QObject *object, const QUrl &url) {
        const auto profile = qobject_cast<QQuickWebEngineProfile *>(object);
        auto observer = observers.value(profile);
        const bool ok = observer && observer->revokeTrackersForOrigin(url);
        if (ok) emit trackerExceptionsChanged();
        return ok;
    }
    Q_INVOKABLE bool revokeAdsForOrigin(QObject *object, const QUrl &url) {
        const auto profile = qobject_cast<QQuickWebEngineProfile *>(object);
        auto observer = observers.value(profile);
        const bool ok = observer && observer->revokeAdsForOrigin(url);
        if (ok) emit trackerExceptionsChanged();
        return ok;
    }
    Q_INVOKABLE void retainTrackersForOwner(QObject *object, const QObject *owner, const QUrl &currentUrl) {
        const auto profile = qobject_cast<QQuickWebEngineProfile *>(object);
        if (auto observer = observers.value(profile)) {
            observer->retainTrackersForOwner(owner, currentUrl);
            emit trackerExceptionsChanged();
        }
    }
    Q_INVOKABLE void releaseTrackerOwner(QObject *object, const QObject *owner) {
        const auto profile = qobject_cast<QQuickWebEngineProfile *>(object);
        if (auto observer = observers.value(profile)) {
            observer->releaseTrackerOwner(owner);
            emit trackerExceptionsChanged();
        }
    }
    Q_INVOKABLE void releaseTemporaryProfile(QObject *object) {
        auto profile = qobject_cast<QQuickWebEngineProfile *>(object);
        const bool http = m_temporaryHttpProfiles.remove(profile);
        if (!http) return;
        if (auto observer = observers.value(profile)) { observer->revokeHttpNavigation(); }
        if (auto owner = m_temporaryProfileOwners.take(profile)) owner->deleteLater();
    }
    bool blockTrackers() const { return settings.value("preferences/blockTrackers", true).toBool(); }
    void setBlockTrackers(bool enabled) {
        if (enabled == blockTrackers()) return;
        settings.setValue("preferences/blockTrackers", enabled);
        for (auto observer : observers) observer->setBlockTrackers(enabled);
        emit preferencesChanged();
    }
    Q_INVOKABLE int totalBlockedTrackers() const {
        int total = 0;
        for (auto observer : observers) total += observer->blockedCount();
        return total;
    }
    Q_INVOKABLE int totalBlockedTrackersOnly() const {
        int total = 0;
        for (auto observer : observers) total += observer->blockedTrackers();
        return total;
    }
    Q_INVOKABLE int totalBlockedAds() const {
        int total = 0;
        for (auto observer : observers) total += observer->blockedAds();
        return total;
    }
    Q_INVOKABLE QString navigationBlockReason(const QUrl &url, QObject *profile = nullptr, bool mainFrame = true) const {
        if (NetworkPolicy::hasWebCredentials(url))
            return tr("Les adresses contenant un identifiant ou un mot de passe sont bloquÃ©es.");
        if (NetworkPolicy::blocks(url, httpsOnly())
            && !(isTemporaryHttpProfile(profile) && url.scheme() == "http" && (!mainFrame || canStartTemporaryHttp(profile, url))))
            return tr("Mode HTTPS strict : cette connexion non chiffrÃ©e est bloquÃ©e. Utilisez une adresse HTTPS.");
        return {};
    }
    QString controlBarPosition() const { return settings.value("ui/controlBar/position", "right").toString(); }
    void setControlBarPosition(const QString &value) {
        if (!QStringList{"top", "right", "bottom", "left"}.contains(value) || value == controlBarPosition()) return;
        settings.setValue("ui/controlBar/position", value); settings.sync(); emit preferencesChanged();
    }
    bool reduceMotion() const {
#if QT_VERSION >= QT_VERSION_CHECK(6, 12, 0)
        if (QGuiApplication::styleHints()->accessibility()->motionPreference() == Qt::MotionPreference::ReducedMotion) return true;
#endif
        return settings.value("ui/controlBar/reduceMotion", false).toBool();
    }
    bool highContrast() const {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        return QGuiApplication::styleHints()->accessibility()->contrastPreference() == Qt::ContrastPreference::HighContrast;
#else
        return false;
#endif
    }
    void setReduceMotion(bool value) { settings.setValue("ui/controlBar/reduceMotion", value); emit preferencesChanged(); }
    bool disableGlow() const { return settings.value("ui/controlBar/disableGlow", false).toBool(); }
    void setDisableGlow(bool value) { settings.setValue("ui/controlBar/disableGlow", value); emit preferencesChanged(); }
    QString controlBarLanguage() const { return settings.value("ui/controlBar/language", "fr").toString(); }
    void setControlBarLanguage(const QString &language) {
        if ((language != "fr" && language != "ru") || language == controlBarLanguage()) return;
        settings.setValue("ui/controlBar/language", language); emit languageChanged(); emit preferencesChanged();
    }
    QString wallpaperType() const { return settings.value("preferences/wallpaperType", "alpine").toString(); }
    void setWallpaperType(const QString &value) {
        if (!QStringList{"alpine", "dark", "custom"}.contains(value) || value == wallpaperType()) return;
        settings.setValue("preferences/wallpaperType", value); emit preferencesChanged();
    }
    QString customWallpaperPath() const { return settings.value("preferences/customWallpaperPath", "").toString(); }
    void setCustomWallpaperPath(const QString &path) {
        if (path == customWallpaperPath()) return;
        settings.setValue("preferences/customWallpaperPath", path); emit preferencesChanged();
    }
    Q_INVOKABLE bool setCustomWallpaper(const QUrl &url) {
        const QFileInfo file(url.toLocalFile());
        if (!url.isLocalFile() || !file.isFile() || !QStringList{"jpg", "jpeg", "png", "webp", "bmp"}.contains(file.suffix().toLower())) return false;
        settings.setValue("preferences/customWallpaperPath", file.canonicalFilePath());
        settings.setValue("preferences/wallpaperType", "custom");
        emit preferencesChanged();
        return true;
    }
    int glassBlurLevel() const { return settings.value("preferences/glassBlurLevel", 2).toInt(); }
    void setGlassBlurLevel(int level) {
        if (level < 0 || level > 2 || level == glassBlurLevel()) return;
        settings.setValue("preferences/glassBlurLevel", level); emit preferencesChanged();
    }
    RequestObserver *observerFor(QQuickWebEngineProfile *profile) const { return observers.value(profile); }
    void setSearchEngine(const QString &value) {
        if (!QStringList{"DuckDuckGo", "Google", "Bing", "Brave"}.contains(value) || value == searchEngine()) return;
        settings.setValue("preferences/searchEngine", value); emit preferencesChanged();
    }
    void setRestoreSession(bool value) {
        if (restoreSession() == value) return;
        settings.setValue("preferences/restoreSession", value); emit preferencesChanged();
    }
    void setBlockThirdPartyCookies(bool value) {
        if (blockThirdPartyCookies() == value) return;
        settings.setValue("preferences/blockThirdPartyCookies", value);
        for (auto profile : profiles) if (profile) applyCookieFilter(profile);
        emit preferencesChanged();
    }
    Q_INVOKABLE QString searchUrl(const QString &query) const {
        const QString prefix = searchEngine() == "Google" ? "https://www.google.com/search?q="
            : searchEngine() == "Bing" ? "https://www.bing.com/search?q="
            : searchEngine() == "Brave" ? "https://search.brave.com/search?q=" : "https://duckduckgo.com/?q=";
        return prefix + QString::fromLatin1(QUrl::toPercentEncoding(query));
    }
    Q_INVOKABLE QString origin(const QString &address) const {
        QUrl url(address);
        if ((url.scheme() == "https" && url.port() == 443) || (url.scheme() == "http" && url.port() == 80)) url.setPort(-1);
        return url.adjusted(QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment | QUrl::RemoveUserInfo).toString();
    }
    Q_INVOKABLE void configureProfile(QObject *object) {
        auto profile = qobject_cast<QQuickWebEngineProfile *>(object);
        profiles.removeIf([](const auto &item) { return item.isNull(); });
        if (!profile || profiles.contains(profile)) return;
        profiles.append(profile); applyCookieFilter(profile);
        auto observer = new RequestObserver(profile);
        observer->setHttpsOnly(httpsOnly());
        observer->setBlockTrackers(blockTrackers());
        observers.insert(profile, observer);
        profile->setUrlRequestInterceptor(observer);
        emit preferencesChanged();
        connect(profile, &QObject::destroyed, this, [this, profile] { observers.remove(profile); m_temporaryHttpProfiles.remove(profile); m_temporaryProfileOwners.remove(profile); });
    }
    Q_INVOKABLE bool clearCookies(QObject *object) {
        auto profile = qobject_cast<QQuickWebEngineProfile *>(object);
        if (!profile) return false;
        profile->cookieStore()->deleteAllCookies(); return true;
    }
    Q_INVOKABLE bool clearCache(QObject *object) {
        auto profile = qobject_cast<QQuickWebEngineProfile *>(object);
        if (!profile || m_cacheBusy) return false;
        m_cacheBusy = true; emit cacheBusyChanged();
        m_cacheTarget = profile;
        auto connection = std::make_shared<QMetaObject::Connection>();
        *connection = connect(profile, &QQuickWebEngineProfile::clearHttpCacheCompleted, this, [this, connection] {
            disconnect(*connection); m_cacheTarget = nullptr; m_cacheBusy = false; emit cacheBusyChanged();
        });
        connect(profile, &QObject::destroyed, this, [this, profile] {
            if (m_cacheTarget == profile) { m_cacheTarget = nullptr; m_cacheBusy = false; emit cacheBusyChanged(); }
        });
        profile->clearHttpCache(); return true;
    }
    Q_INVOKABLE QString permissionName(int type) const {
        switch (static_cast<QWebEnginePermission::PermissionType>(type)) {
        case QWebEnginePermission::PermissionType::MediaAudioCapture: return "Microphone";
        case QWebEnginePermission::PermissionType::MediaVideoCapture: return "CamÃ©ra";
        case QWebEnginePermission::PermissionType::MediaAudioVideoCapture: return "CamÃ©ra et microphone";
        case QWebEnginePermission::PermissionType::Geolocation: return "Localisation";
        case QWebEnginePermission::PermissionType::Notifications: return "Notifications";
        case QWebEnginePermission::PermissionType::MouseLock: return "Verrouillage du pointeur";
        case QWebEnginePermission::PermissionType::ClipboardReadWrite: return "Lecture du presse-papiers";
        default: return "Permission du site";
        }
    }
    Q_INVOKABLE void newPrivateWindow() { emit privateWindowRequested(); }
    Q_INVOKABLE QVariantList suggestions(const QString &query, bool includeHistory) const {
        const auto needle = query.trimmed();
        if (needle.isEmpty()) return {};
        QVariantList result;
        QSet<QString> seen;
        auto appendMatches = [&](const QVariantList &items, const QString &kind, const QString &kindId) {
            for (const auto &item : items) {
                auto row = item.toMap();
                const auto url = row.value("url").toString();
                const auto title = row.value("title").toString();
                if (!isWebUrl(url) || seen.contains(url) || (!url.contains(needle, Qt::CaseInsensitive) && !title.contains(needle, Qt::CaseInsensitive))) continue;
                row["kind"] = kind;
                row["kindId"] = kindId;
                result.append(row); seen.insert(url);
                if (result.size() >= 8) break;
            }
        };
        appendMatches(bookmarks(), "Favori", "bookmark");
        if (includeHistory && result.size() < 8) appendMatches(history(), "Historique", "history");
        return result;
    }
    Q_INVOKABLE bool isBookmarked(const QString &rawUrl) const {
        const QString url = StoredUrl::sanitize(rawUrl);
        for (const auto &item : bookmarks()) if (item.toMap().value("url").toString() == url) return true;
        return false;
    }
    Q_INVOKABLE void toggleBookmark(const QString &rawUrl, const QString &rawTitle) {
        if (!isWebUrl(rawUrl)) return;
        const QString url = StoredUrl::sanitize(rawUrl);
        const QString title = rawTitle == rawUrl ? url : rawTitle;
        auto list = bookmarks();
        for (int i = 0; i < list.size(); ++i) {
            if (list[i].toMap().value("url").toString() == url) {
                list.removeAt(i); settings.setValue("bookmarks", list); emit changed(); return;
            }
        }
        list.prepend(QVariantMap{{"url", url}, {"title", title.isEmpty() ? url : title}});
        settings.setValue("bookmarks", list); emit changed();
    }
    Q_INVOKABLE void recordVisit(const QString &rawUrl, const QString &rawTitle) {
        if (!isWebUrl(rawUrl)) return;
        const QString url = StoredUrl::sanitize(rawUrl);
        const QString title = rawTitle == rawUrl ? url : rawTitle;
        auto list = history();
        if (!list.isEmpty() && list.first().toMap().value("url").toString() == url) list.removeFirst();
        list.prepend(QVariantMap{{"url", url}, {"title", title.isEmpty() ? url : title},
                                 {"time", QDateTime::currentDateTime().toString(Qt::ISODate)}});
        while (list.size() > 1000) list.removeLast();
        m_historyCache = list;
        m_historyDirty = true;
        if (!m_historySaveTimer.isActive()) m_historySaveTimer.start();
        emit changed();
    }
    Q_INVOKABLE void clearHistory() {
        m_historySaveTimer.stop();
        m_historyDirty = false;
        m_historyCache = QVariantList();
        settings.remove("history");
        emit changed();
    }
    Q_INVOKABLE QVariantList session() const { return settings.value("session").toList(); }
    Q_INVOKABLE void saveSession(const QVariantList &urls) { settings.setValue("session", urls); settings.sync(); }
    Q_INVOKABLE void openDownloads() { QDesktopServices::openUrl(QUrl::fromLocalFile(downloadsPath())); }
    Q_INVOKABLE CloudServiceManager *cloud() { return &m_cloud; }
signals:
    void temporaryTransportExpired(QObject *profile);
    void changed();
    void preferencesChanged();
    void cacheBusyChanged();
    void privateWindowRequested();
    void languageChanged();
    void trackerExceptionsChanged();
private:
    void applyCookieFilter(QQuickWebEngineProfile *profile) {
        const bool block = blockThirdPartyCookies();
        profile->cookieStore()->setCookieFilter([block](const QWebEngineCookieStore::FilterRequest &request) {
            return !block || !request.thirdParty;
        });
    }
    CloudServiceManager m_cloud;
    TemporaryProfileFactory m_temporaryProfileFactory;
    QHash<QQuickWebEngineProfile *, QPointer<QObject>> m_temporaryProfileOwners;
    QList<QPointer<QQuickWebEngineProfile>> profiles;
    QHash<QQuickWebEngineProfile *, RequestObserver *> observers;
    bool m_cacheBusy = false;
    QObject *m_cacheTarget = nullptr;
    static bool isWebUrl(const QString &url) {
        const auto scheme = QUrl(url).scheme(); return scheme == "http" || scheme == "https";
    }
    bool m_allowTestHttp = false;
    bool m_testHttp = false;
    bool m_strictWebRtcActive = false;
    QSet<QQuickWebEngineProfile *> m_temporaryHttpProfiles;
    mutable BrowserSettings settings;
    mutable std::optional<QVariantList> m_historyCache;
    mutable bool m_historyDirty = false;
    QTimer m_historySaveTimer;
};
