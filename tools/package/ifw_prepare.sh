# Sourced by build_installer.sh. Prepares $work with config, packages and the payload.
#   CYBOU_STAGE        staged folder (default build/stage)
#   CYBOU_PKG_VERSION  package version (default 0.4.0-alpha.1)
# There is no online update: installers carry no repository address (see docs/PACKAGING.md, Updates).
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ifw=$(ls -d /c/Qt/Tools/QtInstallerFramework/*/bin | sort -V | tail -1)
pkgver="${CYBOU_PKG_VERSION:-0.4.0-alpha.1}"
prepare_ifw() {
  work="$1"
  rm -rf "$work"; mkdir -p "$work"
  cp -r "$root/installer/config" "$root/installer/packages" "$work/"
  local data="$work/packages/org.cybou.browser/data"
  mkdir -p "$data"
  cp -r "${CYBOU_STAGE:-$root/build/stage}/." "$data/"
  sed -i "s#../../assets/cybou.png#$root/assets/cybou.png#; s#../../assets/cybou#$root/assets/cybou#" "$work/config/config.xml"
  sed -i "s#<Version>0.4.0-alpha.1</Version>#<Version>$pkgver</Version>#" "$work/packages/org.cybou.browser/meta/package.xml"
  sed -i "s#<Version>0.4.0-alpha.1</Version>#<Version>$pkgver</Version>#" "$work/config/config.xml"  # installer version names the install folder
}
