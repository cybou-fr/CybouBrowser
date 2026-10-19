#!/usr/bin/env bash
# One-command release candidate (alpha): build, stage, sign cybou.exe, SBOM of the signed stage, ZIP, installer,
# sign the installer, SHA256SUMS. Output: build/release/. Signing is self-signed unless the sign script is replaced.
#   bash tools/package/release.sh
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
out="$root/build/release"
stage="$root/build/release-stage"
export SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:-1760918400}"
version=$(grep -oE 'project\(CybouBrowser VERSION [0-9.]+' "$root/CMakeLists.txt" | grep -oE '[0-9.]+$')
pre=$(grep -oE 'CYBOU_PRERELEASE "[^"]+"' "$root/CMakeLists.txt" | cut -d'"' -f2)
name="CYBOU-$version-$pre"
rm -rf "$out" "$stage"; mkdir -p "$out"

echo "== build";   cmake --build "$root/build" --config Release --target cybou > "$out/build.log" 2>&1
echo "== stage";   cmake --install "$root/build" --config Release --prefix "$stage" > "$out/install.log" 2>&1
echo "== sign exe"
powershell -NoProfile -ExecutionPolicy Bypass -File "$root/tools/package/sign_selfsigned.ps1" "$(cygpath -w "$stage/bin/cybou.exe")"
echo "== sbom";    python -I "$root/tools/package/make_sbom.py" "$stage" "$out/$name-sbom.cdx.json"
echo "== zip"
python -I - "$stage" "$out/$name-win64.zip" "$name-win64" <<'PY'
import sys, zipfile
from pathlib import Path
stage, target, top = Path(sys.argv[1]), sys.argv[2], sys.argv[3]
with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as z:
    for p in sorted(stage.rglob("*")):
        if p.is_file():
            z.write(p, f"{top}/{p.relative_to(stage).as_posix()}")
PY
echo "== installer"; CYBOU_STAGE="$stage" bash "$root/tools/package/build_installer.sh" > "$out/installer.log" 2>&1
cp "$root/build/CYBOU-$version-$pre-setup.exe" "$out/$name-setup.exe"
echo "== sign installer"
powershell -NoProfile -ExecutionPolicy Bypass -File "$root/tools/package/sign_selfsigned.ps1" "$(cygpath -w "$out/$name-setup.exe")"
(cd "$out" && sha256sum "$name"-* > SHA256SUMS && cat SHA256SUMS)
