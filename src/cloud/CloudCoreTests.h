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
    if (providers.size() != 7) { // proton, google, microsoft, infomaniak, zoho, nextcloud, apple
        qCritical() << "CLOUD_CORE_TEST FAIL: providers count mismatch" << providers.size();
        return false;
    }

    // Verify Proton services (per ADR 0007: Proton has no standalone Presentations app)
    if (CloudProviderCatalog::providerSupportsRole("proton", "presentations")) {
        qCritical() << "CLOUD_CORE_TEST FAIL: proton should not offer fictitious presentations app";
        return false;
    }
    if (!CloudProviderCatalog::providerSupportsRole("proton", "mail") ||
        !CloudProviderCatalog::providerSupportsRole("proton", "calendar") ||
        !CloudProviderCatalog::providerSupportsRole("proton", "files") ||
        !CloudProviderCatalog::providerSupportsRole("proton", "docs") ||
        !CloudProviderCatalog::providerSupportsRole("proton", "sheets") ||
        !CloudProviderCatalog::providerSupportsRole("proton", "meetings")) {
        qCritical() << "CLOUD_CORE_TEST FAIL: missing real proton services";
        return false;
    }

    // Verify Infomaniak & Zoho presence
    if (!CloudProviderCatalog::findProvider("infomaniak").has_value() ||
        !CloudProviderCatalog::findProvider("zoho").has_value()) {
        qCritical() << "CLOUD_CORE_TEST FAIL: infomaniak or zoho missing from catalog";
        return false;
    }

    // Verify verified service URLs (strict prefix & HTTPS)
    if (!CloudProviderCatalog::isVerifiedServiceUrl(QUrl("https://mail.proton.me/"))) return false;
    if (!CloudProviderCatalog::isVerifiedServiceUrl(QUrl("https://docs.google.com/document/"))) return false;
    if (CloudProviderCatalog::isVerifiedServiceUrl(QUrl("https://evil-phishing.com/"))) return false;
    if (CloudProviderCatalog::isVerifiedServiceUrl(QUrl("http://mail.google.com/"))) return false; // HTTP not allowed
    if (CloudProviderCatalog::isVerifiedServiceUrl(QUrl("https://try.nextcloud.com/"))) return false; // demo not allowed

    // 2. Nextcloud URL sanitization tests
    {
        QString clean;
        if (!CloudServiceManager::sanitizeNextcloudUrl("https://mycloud.example.org", &clean) || clean != "https://mycloud.example.org") return false;
        if (!CloudServiceManager::sanitizeNextcloudUrl("  https://mycloud.example.org/  ", &clean) || clean != "https://mycloud.example.org") return false;
        // Non-https must be rejected
        if (CloudServiceManager::sanitizeNextcloudUrl("http://mycloud.example.org")) return false;
        // Credentials must be rejected
        if (CloudServiceManager::sanitizeNextcloudUrl("https://admin:pass@mycloud.example.org")) return false;
        // Query / fragment must be rejected
        if (CloudServiceManager::sanitizeNextcloudUrl("https://mycloud.example.org/path?query=1")) return false;
        // Empty / whitespace
        if (CloudServiceManager::sanitizeNextcloudUrl("   ")) return false;
    }

    // 3. CloudServiceManager state, validation, Nextcloud resolution & resilience
    {
        // Clean test environment
        QSettings settings;
        const auto backup = settings.value("cloud/configuration");
        settings.remove("cloud/configuration");

        CloudServiceManager mgr;
        // Default is proton
        if (mgr.defaultProvider() != "proton" || !mgr.isEnabled()) return false;
        if (mgr.mailUrl() != QUrl("https://mail.proton.me/")) return false;
        if (!mgr.isMailAvailable()) return false;

        // Change default to google
        mgr.setDefaultProvider("google");
        if (mgr.defaultProvider() != "google") return false;
        if (mgr.mailUrl() != QUrl("https://mail.google.com/")) return false;
        if (mgr.resolveUrl("meetings") != QUrl("https://meet.google.com/")) return false;

        // Test invalid default provider rejected
        mgr.setDefaultProvider("nonexistent_hacker_suite");
        if (mgr.defaultProvider() != "google") return false; // remains google

        // Test invalid role override rejected
        mgr.setRoleOverride("invalid_role", "proton");
        if (mgr.roleOverride("invalid_role") != "inherit") return false;
        // Apple does not have meetings web app -> setting apple for meetings must be rejected!
        mgr.setRoleOverride("meetings", "apple");
        if (mgr.roleOverride("meetings") != "inherit") return false;

        // Override single role (mail -> proton, files -> microsoft)
        mgr.setRoleOverride("mail", "proton");
        mgr.setRoleOverride("files", "microsoft");
        if (mgr.mailUrl() != QUrl("https://mail.proton.me/")) return false;
        if (mgr.resolveUrl("files") != QUrl("https://onedrive.live.com/")) return false;
        // Other roles still inherit google
        if (mgr.resolveUrl("calendar") != QUrl("https://calendar.google.com/")) return false;

        // Disable mail role -> isMailAvailable() must become false and mailUrl() empty!
        mgr.setRoleOverride("mail", "disabled");
        if (mgr.isMailAvailable() || mgr.mailUrl().isValid()) return false;

        // Reset file override before Nextcloud test
        mgr.resetRoleOverride("files");
        if (mgr.roleOverride("files") != "inherit") return false;

        // Nextcloud tests (requiresConfiguration when base URL empty)
        mgr.setDefaultProvider("nextcloud");
        if (mgr.defaultProvider() != "nextcloud") return false;
        auto ncFiles = mgr.resolveRole("files");
        if (!ncFiles.value("requiresConfiguration").toBool()) return false;
        if (!ncFiles.value("url").toString().isEmpty()) return false;

        // Set valid Nextcloud URL
        mgr.setNextcloudBaseUrl("https://hub.example.org");
        if (mgr.nextcloudBaseUrl() != "https://hub.example.org") return false;
        ncFiles = mgr.resolveRole("files");
        if (ncFiles.value("requiresConfiguration").toBool()) return false;
        if (ncFiles.value("url").toString() != "https://hub.example.org/apps/files/") return false;

        // Setting invalid nextcloud url must be rejected and preserve existing
        mgr.setNextcloudBaseUrl("http://insecure-site.org");
        if (mgr.nextcloudBaseUrl() != "https://hub.example.org") return false;

        // Reset single override
        mgr.resetRoleOverride("mail");
        if (mgr.roleOverride("mail") != "inherit") return false;

        // Reset all overrides
        mgr.resetAllOverrides();
        if (mgr.roleOverride("files") != "inherit") return false;

        // Disable all by setting defaultProvider to "none"
        mgr.setDefaultProvider("none");
        if (mgr.isEnabled()) return false;
        if (!mgr.resolvedServices().isEmpty()) return false;
        if (mgr.mailUrl().isValid() || mgr.isMailAvailable()) return false;

        // 4. Corrupted settings recovery
        settings.setValue("cloud/configuration", QStringLiteral("{ corrupt: json }"));
        CloudServiceManager corruptMgr;
        if (corruptMgr.defaultProvider() != "proton" || !corruptMgr.isEnabled()) return false;

        // Restore settings
        settings.setValue("cloud/configuration", backup);
    }

    qInfo() << "CLOUD_CORE_TEST PASS: catalog completeness, verified HTTPS URLs, role overrides, inherit & disabled semantics";
    return true;
}
