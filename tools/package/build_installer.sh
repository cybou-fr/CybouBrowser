#!/usr/bin/env bash
# Builds build/CYBOU-0.3.0-alpha.1-setup.exe from build/stage (cmake --install build --config Release --prefix build/stage).
# With CYBOU_UPDATE_URL set, the installed maintenance tool can update from that repository (see build_repo.sh).
set -euo pipefail
source "$(dirname "$0")/ifw_prepare.sh"
prepare_ifw "$root/build/ifw"
# --offline-only drops the repository address, so a build meant to update itself uses the default (hybrid) mode: payload embedded, repository kept.
mode="--offline-only"; [ -n "${CYBOU_UPDATE_URL:-}" ] && mode="" || true
"$ifw/binarycreator.exe" $mode -c "$root/build/ifw/config/config.xml" -p "$root/build/ifw/packages" "$root/build/CYBOU-0.3.0-alpha.1-setup.exe"
ls -l "$root/build/CYBOU-0.3.0-alpha.1-setup.exe"
