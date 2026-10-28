#!/usr/bin/env bash
# Builds build/CYBOU-0.4.0-alpha.1-setup.exe from build/stage (cmake --install build --config Release --prefix build/stage).
# Offline only: the installer and its maintenance tool carry no repository address, so nothing updates by itself.
set -euo pipefail
source "$(dirname "$0")/ifw_prepare.sh"
prepare_ifw "$root/build/ifw"
"$ifw/binarycreator.exe" --offline-only -c "$root/build/ifw/config/config.xml" -p "$root/build/ifw/packages" "$root/build/CYBOU-0.4.0-alpha.1-setup.exe"
ls -l "$root/build/CYBOU-0.4.0-alpha.1-setup.exe"
