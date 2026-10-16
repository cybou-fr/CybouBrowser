#pragma once
#include <QObject>
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
#include <memory>
#include <QtWebEngineQuick/QQuickWebEngineProfile>
#include <QtWebEngineCore/QWebEngineCookieStore>
#include "controlbar/RequestObserver.h"
#include "security/DownloadPolicy.h"

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
    Q_PROPERTY(QString controlBarPosition READ controlBarPosition WRITE setControlBarPosition NOTIFY preferencesChanged)
    Q_PROPERTY(bool reduceMotion READ reduceMotion WRITE setReduceMotion NOTIFY preferencesChanged)
    Q_PROPERTY(bool highContrast READ highContrast NOTIFY preferencesChanged)
    Q_PROPERTY(bool disableGlow READ disableGlow WRITE setDisableGlow NOTIFY preferencesChanged)
    Q_PROPERTY(QString controlBarLanguage READ controlBarLanguage WRITE setControlBarLanguage NOTIFY languageChanged)
public:
    explicit BrowserStore(QObject *parent = nullptr) : QObject(parent) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        connect(QGuiApplication::styleHints()->accessibility(), &QAccessibilityHints::contrastPreferenceChanged, this, &BrowserStore::preferencesChanged);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 12, 0)
        connect(QGuiApplication::styleHints()->accessibility(), &QAccessibilityHints::motionPreferenceChanged, this, &BrowserStore::preferencesChanged);
#endif
    }
    QVariantList bookmarks() const { return settings.value("bookmarks").toList(); }
    QVariantList history() const { return settings.value("history").toList(); }
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
    bool httpsOnly() const { return settings.value("preferences/httpsOnly", false).toBool(); }
    void setHttpsOnly(bool enabled) {
        if (enabled == httpsOnly()) return;
        settings.setValue("preferences/httpsOnly", enabled);
        for (auto observer : observers) observer->setHttpsOnly(enabled);
        emit preferencesChanged();
    }
    Q_INVOKABLE QString navigationBlockReason(const QUrl &url) const {
        if (NetworkPolicy::hasWebCredentials(url))
            return tr("Les adresses contenant un identifiant ou un mot de passe sont bloquées.");
        if (NetworkPolicy::blocks(url, httpsOnly()))
            return tr("Mode HTTPS strict : cette connexion non chiffrée est bloquée. Utilisez une adresse HTTPS.");
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
        observers.insert(profile, observer);
        profile->setUrlRequestInterceptor(observer);
        connect(profile, &QObject::destroyed, this, [this, profile] { observers.remove(profile); });
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
        case QWebEnginePermission::PermissionType::MediaVideoCapture: return "Caméra";
        case QWebEnginePermission::PermissionType::MediaAudioVideoCapture: return "Caméra et microphone";
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
        auto appendMatches = [&](const QVariantList &items, const QString &kind) {
            for (const auto &item : items) {
                auto row = item.toMap();
                const auto url = row.value("url").toString();
                const auto title = row.value("title").toString();
                if (!isWebUrl(url) || seen.contains(url) || (!url.contains(needle, Qt::CaseInsensitive) && !title.contains(needle, Qt::CaseInsensitive))) continue;
                row["kind"] = kind; result.append(row); seen.insert(url);
                if (result.size() >= 8) break;
            }
        };
        appendMatches(bookmarks(), "Favori");
        if (includeHistory && result.size() < 8) appendMatches(history(), "Historique");
        return result;
    }
    Q_INVOKABLE bool isBookmarked(const QString &url) const {
        for (const auto &item : bookmarks()) if (item.toMap().value("url").toString() == url) return true;
        return false;
    }
    Q_INVOKABLE void toggleBookmark(const QString &url, const QString &title) {
        if (!isWebUrl(url)) return;
        auto list = bookmarks();
        for (int i = 0; i < list.size(); ++i) {
            if (list[i].toMap().value("url").toString() == url) {
                list.removeAt(i); settings.setValue("bookmarks", list); emit changed(); return;
            }
        }
        list.prepend(QVariantMap{{"url", url}, {"title", title.isEmpty() ? url : title}});
        settings.setValue("bookmarks", list); emit changed();
    }
    Q_INVOKABLE void recordVisit(const QString &url, const QString &title) {
        if (!isWebUrl(url)) return;
        auto list = history();
        if (!list.isEmpty() && list.first().toMap().value("url").toString() == url) list.removeFirst();
        list.prepend(QVariantMap{{"url", url}, {"title", title.isEmpty() ? url : title},
                                 {"time", QDateTime::currentDateTime().toString(Qt::ISODate)}});
        while (list.size() > 1000) list.removeLast();
        settings.setValue("history", list); emit changed();
    }
    Q_INVOKABLE void clearHistory() { settings.remove("history"); emit changed(); }
    Q_INVOKABLE QVariantList session() const { return settings.value("session").toList(); }
    Q_INVOKABLE void saveSession(const QVariantList &urls) { settings.setValue("session", urls); settings.sync(); }
    Q_INVOKABLE void openDownloads() { QDesktopServices::openUrl(QUrl::fromLocalFile(downloadsPath())); }
signals:
    void changed();
    void preferencesChanged();
    void cacheBusyChanged();
    void privateWindowRequested();
    void languageChanged();
private:
    void applyCookieFilter(QQuickWebEngineProfile *profile) {
        const bool block = blockThirdPartyCookies();
        profile->cookieStore()->setCookieFilter([block](const QWebEngineCookieStore::FilterRequest &request) {
            return !block || !request.thirdParty;
        });
    }
    QList<QPointer<QQuickWebEngineProfile>> profiles;
    QHash<QQuickWebEngineProfile *, RequestObserver *> observers;
    bool m_cacheBusy = false;
    QObject *m_cacheTarget = nullptr;
    static bool isWebUrl(const QString &url) {
        const auto scheme = QUrl(url).scheme(); return scheme == "http" || scheme == "https";
    }
    mutable QSettings settings;
};
