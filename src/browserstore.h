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
#include <memory>
#include <QtWebEngineQuick/QQuickWebEngineProfile>
#include <QtWebEngineCore/QWebEngineCookieStore>

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
public:
    explicit BrowserStore(QObject *parent = nullptr) : QObject(parent) {}
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
    Q_INVOKABLE bool openDownloadedFile(const QString &path) {
        return QFileInfo(path).isFile() && QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
    Q_INVOKABLE void showFileFolder(const QString &path) { QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath())); }
    QString searchEngine() const { return settings.value("preferences/searchEngine", "DuckDuckGo").toString(); }
    bool restoreSession() const { return settings.value("preferences/restoreSession", true).toBool(); }
    bool blockThirdPartyCookies() const { return settings.value("preferences/blockThirdPartyCookies", true).toBool(); }
    bool cacheBusy() const { return m_cacheBusy; }
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
        const QUrl url(address);
        return url.adjusted(QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment | QUrl::RemoveUserInfo).toString();
    }
    Q_INVOKABLE void configureProfile(QObject *object) {
        auto profile = qobject_cast<QQuickWebEngineProfile *>(object);
        profiles.removeIf([](const auto &item) { return item.isNull(); });
        if (!profile || profiles.contains(profile)) return;
        profiles.append(profile); applyCookieFilter(profile);
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
private:
    void applyCookieFilter(QQuickWebEngineProfile *profile) {
        const bool block = blockThirdPartyCookies();
        profile->cookieStore()->setCookieFilter([block](const QWebEngineCookieStore::FilterRequest &request) {
            return !block || !request.thirdParty;
        });
    }
    QList<QPointer<QQuickWebEngineProfile>> profiles;
    bool m_cacheBusy = false;
    QObject *m_cacheTarget = nullptr;
    static bool isWebUrl(const QString &url) {
        const auto scheme = QUrl(url).scheme(); return scheme == "http" || scheme == "https";
    }
    mutable QSettings settings;
};
