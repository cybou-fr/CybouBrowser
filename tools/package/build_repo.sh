#!/usr/bin/env bash
# Builds an update repository for one package version: build_repo.sh <output-dir>
#   CYBOU_PKG_VERSION=0.4.0-alpha.1 CYBOU_STAGE=build/stage bash tools/package/build_repo.sh build/update-repo
# Publish the folder's contents at the URL that installers were built with (CYBOU_UPDATE_URL).
set -euo pipefail
source "$(dirname "$0")/ifw_prepare.sh"
out="${1:?output directory}"
prepare_ifw "$root/build/ifw-repo"
rm -rf "$out"
"$ifw/repogen.exe" -p "$root/build/ifw-repo/packages" "$out"
echo "repository for $pkgver in $out"
