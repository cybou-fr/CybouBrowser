cmake_minimum_required(VERSION 3.24)
project(QtWinRtPositioningFix LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Directory holding the (patched) Qt 6.12.0 WinRT position plugin sources; build.sh prepares it.
set(PLUGIN_SRC "" CACHE PATH "Patched qtpositioning winrt plugin sources")
if(NOT EXISTS "${PLUGIN_SRC}/qgeopositioninfosource_winrt.cpp")
    message(FATAL_ERROR "PLUGIN_SRC must point at the patched plugin sources (run build.sh)")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core CorePrivate Positioning)

# Same plugin identity as the stock one (file qtposition_winrt.dll, key "winrt"); it is picked up by placing
# its directory first on QT_PLUGIN_PATH (or in the application's plugin search path), see README.md.
qt_add_plugin(qtposition_winrt SHARED PLUGIN_TYPE position CLASS_NAME QGeoPositionInfoSourceFactoryWinRT)
target_sources(qtposition_winrt PRIVATE
    "${PLUGIN_SRC}/qgeopositioninfosource_winrt.cpp" "${PLUGIN_SRC}/qgeopositioninfosource_winrt_p.h"
    "${PLUGIN_SRC}/qgeopositioninfosourcefactory_winrt.cpp" "${PLUGIN_SRC}/qgeopositioninfosourcefactory_winrt.h")
target_include_directories(qtposition_winrt PRIVATE "${PLUGIN_SRC}")
target_link_libraries(qtposition_winrt PRIVATE Qt6::Core Qt6::CorePrivate Qt6::Positioning runtimeobject)
set_target_properties(qtposition_winrt PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/plugins/position"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/plugins/position"
    OUTPUT_NAME qtposition_winrt)
