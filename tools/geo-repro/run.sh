#!/usr/bin/env bash
# Usage: run.sh [processes=30] [cycles=50] [mode=delete|stop-then-delete] [plugin-dir-to-prepend]
# Runs geo_repro many times and reports how many processes ended abnormally.
set -u
processes=${1:-30}; cycles=${2:-50}; mode=${3:-delete}; plugins=${4:-}
here="$(cd "$(dirname "$0")" && pwd)"
exe="$here/../../build/geo-repro/Release/geo_repro.exe"
export QT_FORCE_STDERR_LOGGING=1
export PATH="/c/Qt/6.12.0/msvc2022_64/bin:$PATH"
if [ -n "$plugins" ]; then export QT_PLUGIN_PATH="$plugins"; fi
dumps_before=$(ls "$LOCALAPPDATA/CrashDumps" 2>/dev/null | grep -c geo_repro)
crashed=0; ok=0; created=0
for i in $(seq 1 "$processes"); do
  out=$("$exe" "$cycles" "$mode" 2>&1); rc=$?
  if [ $rc -eq 0 ]; then ok=$((ok+1)); c=$(echo "$out" | grep -o "created [0-9]*" | head -1 | awk '{print $2}'); created=$((created+${c:-0})); else crashed=$((crashed+1)); fi
done
dumps_after=$(ls "$LOCALAPPDATA/CrashDumps" 2>/dev/null | grep -c geo_repro)
echo "GEO_REPRO_RESULT mode=$mode processes=$processes cycles=$cycles clean=$ok abnormal=$crashed sourcesCreatedInCleanRuns=$created newDumps=$((dumps_after-dumps_before))"
