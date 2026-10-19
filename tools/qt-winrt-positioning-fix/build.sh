#!/usr/bin/env bash
# Builds the patched Qt 6.12.0 WinRT position plugin.
#   build.sh [QT_ROOT=C:/Qt/6.12.0] [OUT=<repo>/build/qt-winrt-fix]
# 1. copies the stock sources from the Qt installation's Src directory,
# 2. refuses to continue unless they match ORIGINAL.sha256 (the unmodified 6.12.0 files),
# 3. applies the patch, 4. builds a standalone plugin.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
qt_root="${1:-C:/Qt/6.12.0}"
out="${2:-$here/../../build/qt-winrt-fix}"
stock="$qt_root/Src/qtpositioning/src/plugins/position/winrt"
src="$out/src"
rm -rf "$src"; mkdir -p "$src"
for f in qgeopositioninfosource_winrt.cpp qgeopositioninfosource_winrt_p.h qgeopositioninfosourcefactory_winrt.cpp qgeopositioninfosourcefactory_winrt.h plugin.json; do cp "$stock/$f" "$src/$f"; done
( cd "$src" && sha256sum --check --quiet "$here/ORIGINAL.sha256" ) || { echo "stock sources differ from the pinned Qt 6.12.0 files; not patching" >&2; exit 2; }
( cd "$src" && git apply --ignore-whitespace -p1 "$here"/0001-*.patch )
cmake -S "$here" -B "$out" -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="$qt_root/msvc2022_64" -DPLUGIN_SRC="$src"
cmake --build "$out" --config Release
sha256sum "$out/plugins/position/qtposition_winrt.dll"
