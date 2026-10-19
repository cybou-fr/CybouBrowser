#!/usr/bin/env bash
# Builds build/CYBOU-<version>-setup.exe from build/stage (cmake --install build --config Release --prefix build/stage).
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
ifw=$(ls -d /c/Qt/Tools/QtInstallerFramework/*/bin | sort -V | tail -1)
work="$root/build/ifw"
rm -rf "$work"; mkdir -p "$work"
cp -r "$root/installer/config" "$root/installer/packages" "$work/"
data="$work/packages/org.cybou.browser/data"
mkdir -p "$data"
cp -r "${CYBOU_STAGE:-$root/build/stage}/." "$data/"
sed -i "s#../../assets/cybou#$root/assets/cybou#; s#../../assets/cybou.png#$root/assets/cybou.png#" "$work/config/config.xml"
"$ifw/binarycreator.exe" --offline-only -c "$work/config/config.xml" -p "$work/packages" "$root/build/CYBOU-0.3.0-alpha.1-setup.exe"
ls -l "$root/build/CYBOU-0.3.0-alpha.1-setup.exe"
