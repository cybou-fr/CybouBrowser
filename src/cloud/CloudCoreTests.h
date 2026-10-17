#pragma once
#include "CloudProviderCatalog.h"
#include "CloudServiceManager.h"
#include <QDebug>
#include <QSettings>

inline bool runCloudCoreTests() {
    // 1. Catalog completeness and consistency
    const auto &roles = CloudProviderCatalog::supportedRoles();
    if (roles.size() != 7) {
        qCritical() << "CLOUD_CORE_TEST FAIL: unexpected roles count" << roles.size();
        return false;
    }

    const auto &providers = CloudProviderCatalog::allProviders();
    if (providers.size() < 4) {
        qCritical() << "CLOUD_CORE_TEST FAIL: providers count too low" << providers.size();
        return false;
    }

    for (const auto &p : providers) {
        if (p.id.isEmpty() || p.name.isEmpty()) return false;
        for (const auto &s : p.services) {
            if (!s.url.isValid() || s.url.scheme() != QStringLiteral("https")) {
                qCritical() << "CLOUD_CORE_TEST FAIL: unverified or non-https URL" << p.id << s.role << s.url;
                return false;
            }
            if (!CloudProviderCatalog::isVerifiedServiceUrl(s.url)) {
                qCritical() << "CLOUD_CORE_TEST FAIL: isVerifiedServiceUrl failed" << s.url;
                return false;
            }
        }
    }

    // Unverified / arbitrary URL must return false
    if (CloudProviderCatalog::isVerifiedServiceUrl(QUrl("https://evil-phishing.com/"))) return false;
    if (CloudProviderCatalog::isVerifiedServiceUrl(QUrl("http://mail.google.com/"))) return false; // HTTP not allowed

    // 2. CloudServiceManager resolution logic in memory
    {
        // Clean test environment
        QSettings settings;
        const auto backup = settings.value("cloud/configuration");
        settings.remove("cloud/configuration");

        CloudServiceManager mgr;
        // Default is proton
        if (mgr.defaultProvider() != "proton" || !mgr.isEnabled()) return false;
        if (mgr.mailUrl() != QUrl("https://mail.proton.me/")) return false;

        // Change default to google
        mgr.setDefaultProvider("google");
        if (mgr.defaultProvider() != "google") return false;
        if (mgr.mailUrl() != QUrl("https://mail.google.com/")) return false;
        if (mgr.resolveUrl("meetings") != QUrl("https://meet.google.com/")) return false;

        // Override single role (mail -> proton, files -> microsoft)
        mgr.setRoleOverride("mail", "proton");
        mgr.setRoleOverride("files", "microsoft");
        if (mgr.mailUrl() != QUrl("https://mail.proton.me/")) return false;
        if (mgr.resolveUrl("files") != QUrl("https://onedrive.live.com/")) return false;
        // Other roles still inherit google
        if (mgr.resolveUrl("calendar") != QUrl("https://calendar.google.com/")) return false;

        // Disable presentations role
        mgr.setRoleOverride("presentations", "disabled");
        if (!mgr.resolveRole("presentations").isEmpty() || mgr.resolveUrl("presentations").isValid()) return false;

        // Reset single override
        mgr.resetRoleOverride("mail");
        if (mgr.roleOverride("mail") != "inherit") return false;
        if (mgr.mailUrl() != QUrl("https://mail.google.com/")) return false; // back to google

        // Reset all overrides
        mgr.resetAllOverrides();
        if (mgr.roleOverride("files") != "inherit" || mgr.roleOverride("presentations") != "inherit") return false;
        if (mgr.resolveUrl("files") != QUrl("https://drive.google.com/")) return false;

        // Disable all by setting defaultProvider to "none"
        mgr.setDefaultProvider("none");
        if (mgr.isEnabled()) return false;
        if (!mgr.resolvedServices().isEmpty()) return false;
        if (mgr.mailUrl().isValid()) return false;

        // Restore settings
        settings.setValue("cloud/configuration", backup);
    }

    qInfo() << "CLOUD_CORE_TEST PASS: catalog completeness, verified HTTPS URLs, role overrides, inherit & disabled semantics";
    return true;
}
