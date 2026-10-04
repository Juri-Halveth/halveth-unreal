#!/usr/bin/env bash
# MIT. Native Portal Garden controlled from Git Bash.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ENGINE="${GARDEN_ENGINE:-/c/Program Files/Epic Games/UE_5.8}"
PYTHON="${GARDEN_PYTHON:-python}"
MODE=${1:-play}
[[ $# -le 1 ]] || { echo 'GARDEN.sh [play|build|prepare|check|visual|status|sky]' >&2; exit 2; }
EDITOR="$ENGINE/Engine/Binaries/Win64/UnrealEditor.exe"
CMD="$ENGINE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe"
PROJECT="$ROOT/HALVETHRealms.uproject"
[[ -f "$EDITOR" ]] || { echo 'Unreal Engine 5.8 fehlt. GARDEN_ENGINE setzt den Installationspfad.' >&2; exit 3; }
export MSYS2_ARG_CONV_EXCL='*'
mkdir -p "$ROOT/Saved"
NATIVE_PROJECT=$(cygpath -w "$PROJECT")
build() {
  "$PYTHON" "$(cygpath -w "$ROOT/scripts/garden_assets.py")" bootstrap
  local digest
  digest=$(cd "$ROOT" && { find Source Config -type f -print0 | sort -z | xargs -0 sha256sum; sha256sum HALVETHRealms.uproject; } | sha256sum | cut -d ' ' -f 1)
  if [[ ! -f "$ROOT/Binaries/Win64/UnrealEditor-HALVETHRealms.dll" || ! -f "$ROOT/Saved/GARDEN-build.sha256" || "$(cat "$ROOT/Saved/GARDEN-build.sha256")" != "$digest" ]]; then
    printf '@echo off\r\ncall "%s" HALVETHRealmsEditor Win64 Development "-Project=%s" -WaitMutex -MaxParallelActions=2 -NoUBA\r\nexit /b %%errorlevel%%\r\n' "$(cygpath -w "$ENGINE/Engine/Build/BatchFiles/Build.bat")" "$NATIVE_PROJECT" > "$ROOT/Saved/GARDEN-build.cmd"
    (cd "$ROOT" && cmd.exe /d /c 'Saved\GARDEN-build.cmd')
    printf '%s\n' "$digest" > "$ROOT/Saved/GARDEN-build.sha256"
  fi
}
prepare() {
  build
  if ! "$PYTHON" "$(cygpath -w "$ROOT/scripts/garden_assets.py")" verify >/dev/null; then
    if ! "$PYTHON" "$(cygpath -w "$ROOT/scripts/garden_assets.py")" geometry-current; then
      "$CMD" "$NATIVE_PROJECT" -run=HALVETHPrepare -SurfaceOnly -unattended -nop4 -nosound "-abslog=$(cygpath -w "$ROOT/Saved/GARDEN-surface.log")"
      "$PYTHON" "$(cygpath -w "$ROOT/scripts/garden_assets.py")" archive-generated
      "$CMD" "$NATIVE_PROJECT" -run=GardenPrepare -unattended -nop4 -nosound "-abslog=$(cygpath -w "$ROOT/Saved/GARDEN-prepare.log")"
    fi
    if ! "$PYTHON" "$(cygpath -w "$ROOT/scripts/garden_assets.py")" characters-current; then
      "$CMD" "$NATIVE_PROJECT" -run=GardenCharacters -unattended -nop4 -nosound "-abslog=$(cygpath -w "$ROOT/Saved/GARDEN-characters.log")"
    fi
    "$PYTHON" "$(cygpath -w "$ROOT/scripts/garden_assets.py")" bind
  fi
}
case "$MODE" in
  build) build ;;
  prepare) prepare ;;
  status) "$PYTHON" "$(cygpath -w "$ROOT/scripts/garden_assets.py")" verify ;;
  sky) "$PYTHON" "$(cygpath -w "$ROOT/scripts/garden_sky.py")" sync ;;
  play)
    if ! "$PYTHON" "$(cygpath -w "$ROOT/scripts/garden_sky.py")" sync; then echo 'NASA-Abruf fehlgeschlagen; datierter lokaler Himmelsverlauf wird weiter genutzt.' >&2; fi
    prepare
    exec "$EDITOR" "$NATIVE_PROJECT" -game -HalvethSeed=2026092302 -HalvethQuality=2 -windowed -borderless -ResX=2560 -ResY=1440 -NoSplash "-abslog=$(cygpath -w "$ROOT/Saved/GARDEN-play.log")" ;;
  check)
    prepare
    "$CMD" "$NATIVE_PROJECT" -game -HalvethSmokeTest -NullRHI -unattended -nosound "-abslog=$(cygpath -w "$ROOT/Saved/GARDEN-check.log")"
    "$PYTHON" "$(cygpath -w "$ROOT/scripts/garden_assets.py")" qa ;;
  visual)
    prepare
    "$EDITOR" "$NATIVE_PROJECT" -game -HalvethVisual -windowed -ResX=1920 -ResY=1080 -ForceRes -NoSplash -nosound "-abslog=$(cygpath -w "$ROOT/Saved/GARDEN-visual.log")" ;;
  *) echo 'GARDEN.sh [play|build|prepare|check|visual|status|sky]' >&2; exit 2 ;;
esac
