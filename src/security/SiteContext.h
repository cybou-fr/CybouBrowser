#pragma once
#include <QObject>
#include <QPointer>
#include <QUrl>
#include <QVariant>

struct SiteContext {
    QPointer<QObject> view;
    QPointer<QObject> profile;
    QString tabUid;
    QUrl topOrigin;
    QUrl requestingOrigin;
    quint64 navigationEpoch = 0;

    static QUrl origin(const QUrl &url) {
        if ((url.scheme() != "https" && url.scheme() != "http") || url.host().isEmpty()) return {};
        QUrl result;
        result.setScheme(url.scheme().toLower());
        result.setHost(url.host().toLower());
        const int port = url.port();
        if (port != -1 && !(port == 443 && result.scheme() == "https")
            && !(port == 80 && result.scheme() == "http")) result.setPort(port);
        return result;
    }
    bool matches(QObject *activeView, QObject *activeProfile) const {
        return view && profile && view == activeView && profile == activeProfile
            && view->property("profile").value<QObject *>() == profile
            && view->property("tabUid").toString() == tabUid
            && view->property("navigationEpoch").toULongLong() == navigationEpoch
            && origin(view->property("url").toUrl()) == topOrigin;
    }
};
