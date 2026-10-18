cmake_minimum_required(VERSION 3.24)
project(CybouBrowser VERSION 0.3.0 LANGUAGES CXX)
# Product source version: 0.3.0-alpha.1; QML module URI version below is unrelated.
set(CYBOU_PRERELEASE "alpha.1")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.9 REQUIRED COMPONENTS Quick QuickControls2 WebEngineQuick WebEngineCore Network LinguistTools)
find_package(Qt6 6.9 OPTIONAL_COMPONENTS Test)
qt_standard_project_setup(REQUIRES 6.9)

qt_add_library(cybou_core STATIC
    src/browserstore.h
    src/ui/AddressDisplay.h
    src/security/NetworkPolicy.h
    src/security/DownloadPolicy.h
    src/security/RuleEngine.h
    src/security/TrackerBlocker.h
    src/security/TrackerExceptionRegistry.h
)

qt_add_resources(cybou_core "brand" PREFIX "/"
    FILES assets/cybou.png assets/cybou-alpine-dawn-1920x1080.jpg assets/speaker.svg assets/speaker-muted.svg)

file(GLOB CONTROL_BAR_ICONS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "assets/controlbar/*.svg")
qt_add_resources(cybou_core "controlbar_icons" PREFIX "/" FILES ${CONTROL_BAR_ICONS})

file(GLOB CLOUD_ICONS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "assets/cloud/*.svg")
qt_add_resources(cybou_core "cloud_icons" PREFIX "/" FILES ${CLOUD_ICONS})

set_source_files_properties(qml/CybouTheme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(cybou_core URI Cybou VERSION 1.0
    SOURCES
        src/security/PermissionCoordinator.h
        src/security/PermissionCoordinator.cpp
        src/security/SiteContext.h
        src/security/PermissionLease.h
        src/security/NavigationGuard.h
        src/security/EffectivePermissionPolicy.h
        src/controlbar/ControlBarController.h
        src/controlbar/ControlBarController.cpp
        src/controlbar/CapabilityModel.h
        src/controlbar/RequestObserver.h
        src/activity/ActivityEvent.h
        src/activity/ActivityAggregator.h
        src/activity/ActivityAggregator.cpp
        src/cloud/CloudProviderCatalog.h
        src/cloud/CloudProviderCatalog.cpp
        src/cloud/CloudServiceManager.h
        src/cloud/CloudServiceManager.cpp
    QML_FILES
        qml/CybouTheme.qml
        qml/NewTabPage.qml
        qml/Main.qml
        qml/NavButton.qml
        qml/GlassToolTip.qml
        qml/SettingsPanel.qml
        qml/CloudSettingsSection.qml
        qml/FrostedGlassSurface.qml
        qml/CloudAppTile.qml
        qml/CloudAppLauncher.qml
        qml/AdaptiveAddress.qml
        qml/ControlBarInline.qml
        qml/ControlBarOverflow.qml
        qml/ControlBarIcon.qml
        qml/CapabilityPopover.qml
        qml/ControlBarHarness.qml
        qml/TransportHarness.qml
        qml/TrackerHarness.qml
        qml/GlassButton.qml
        qml/GlassDialogBackground.qml
        qml/GlassMenuItem.qml
        qml/GlassMenuSeparator.qml
        qml/LibraryDrawer.qml
        qml/BrowserTab.qml
        qml/DownloadOpenDialog.qml
        qml/PolicyBanner.qml
        qml/FindBar.qml
)

target_include_directories(cybou_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/controlbar"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/security"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/activity"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cloud"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui"
)

qt_add_translations(cybou_core TS_FILES translations/cybou_ru.ts RESOURCE_PREFIX "/translations")

target_link_libraries(cybou_core PUBLIC
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::WebEngineQuick
    Qt6::WebEngineCore
    Qt6::Network
)

# Production browser executable (no Qt6::Test linked)
qt_add_executable(cybou src/main.cpp)
if(WIN32)
    target_sources(cybou PRIVATE assets/cybou.rc)
endif()
target_link_libraries(cybou PRIVATE cybou_core cybou_coreplugin)
target_compile_definitions(cybou PRIVATE "CYBOU_PRODUCT_VERSION=\"${PROJECT_VERSION}-${CYBOU_PRERELEASE}\"")
set_target_properties(cybou PROPERTIES WIN32_EXECUTABLE TRUE MACOSX_BUNDLE TRUE)
install(TARGETS cybou BUNDLE DESTINATION . RUNTIME DESTINATION bin)

# Test runner executable
if(TARGET Qt6::Test)
    qt_add_executable(cybou_tests tests/test_main.cpp src/security/SecurityTests.h src/security/ExternalProtocolTests.h)
    target_link_libraries(cybou_tests PRIVATE cybou_core cybou_coreplugin Qt6::Test)
    target_compile_definitions(cybou_tests PRIVATE "CYBOU_PRODUCT_VERSION=\"${PROJECT_VERSION}-${CYBOU_PRERELEASE}\"")
endif()
