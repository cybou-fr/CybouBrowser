#!/usr/bin/env bash
# Builds cybou.exe twice from clean and compares the SHA-256. Needs the same source folder and toolchain both times.
# SOURCE_DATE_EPOCH fixes the resource timestamps that rcc embeds; /Brepro (CMakeLists.txt) fixes the PE/PDB stamps.
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
export SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:-1760918400}"
for i in 1 2; do
  cmake --build "$root/build" --config Release --target cybou --clean-first > "$root/build/repro-$i.log" 2>&1
  h[$i]=$(sha256sum "$root/build/Release/cybou.exe" | cut -c1-64)
  echo "build $i: ${h[$i]}"
done
[ "${h[1]}" = "${h[2]}" ] && echo "REPRODUCIBLE: identical" || { echo "NOT REPRODUCIBLE: hashes differ"; exit 1; }
