#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ENGINE="${VOID_ENGINE:-/c/Program Files/Epic Games/UE_5.8}"
PROJECT="$ROOT/HALVETHVoid.uproject"
MODE="${1:-play}"
SOUL="${2-0}"
[[ -f "$ENGINE/Engine/Binaries/Win64/UnrealEditor.exe" ]] || { echo 'Unreal Engine 5.8 fehlt. VOID_ENGINE kann den Installationspfad setzen.' >&2; exit 3; }
mkdir -p "$ROOT/Saved"
EDITOR="$ENGINE/Engine/Binaries/Win64/UnrealEditor.exe"
CMD="$ENGINE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe"
WIN_PROJECT="$(cygpath -w "$PROJECT")"
ASSET_TOOL="$(cygpath -w "$ROOT/scripts/assets.py")"
SOUL_TOOL="$(cygpath -w "$ROOT/scripts/soulseed.py")"
export MSYS2_ARG_CONV_EXCL='*'
if [[ "$SOUL" == --soul-file ]]; then
    [[ $# == 3 ]] || { echo 'VOID.sh MODE --soul-file UTF8-DATEI' >&2; exit 2; }
    SOUL_ID="$(python "$SOUL_TOOL" "--file=$(cygpath -w "$3")")"
elif [[ "$SOUL" == --text ]]; then
    [[ $# == 3 ]] || { echo 'VOID.sh MODE --text SOULSEED' >&2; exit 2; }
    SOUL_ID="$(python "$SOUL_TOOL" "--text=$3")"
else
    SOUL_ID="$(python "$SOUL_TOOL" "--text=$SOUL")"
fi
[[ "$SOUL_ID" =~ ^[0-9a-f]{64}$ ]] || { echo 'Soulseed-Aufloesung fehlgeschlagen.' >&2; exit 2; }
SOUL_FILE="$ROOT/Saved/Souls/$SOUL_ID.json"
WIN_SOUL="$(cygpath -w "$SOUL_FILE")"
build() {
    python "$ASSET_TOOL" verify
    local source_hash
    source_hash="$(find "$ROOT/Source" "$ROOT/Config" -type f -print0 | sort -z | xargs -0 sha256sum | sha256sum | cut -d ' ' -f 1)"
    if [[ ! -f "$ROOT/Binaries/Win64/UnrealEditor-HALVETHVoid.dll" || ! -f "$ROOT/Saved/VOID-build.sha256" || "$(cat "$ROOT/Saved/VOID-build.sha256")" != "$source_hash" ]]; then
        local BAT
        BAT="$(cygpath -w "$ENGINE/Engine/Build/BatchFiles/Build.bat")"
        printf '@echo off\r\ncall "%s" HALVETHVoidEditor Win64 Development "-Project=%s" -WaitMutex -MaxParallelActions=2 -NoUBA\r\nexit /b %%errorlevel%%\r\n' "$BAT" "$WIN_PROJECT" > "$ROOT/Saved/VOID-build.cmd"
        (cd "$ROOT" && cmd.exe /d /c 'Saved\VOID-build.cmd')
        printf '%s\n' "$source_hash" > "$ROOT/Saved/VOID-build.sha256"
    fi
}
prepare() {
    build
    local pipeline_hash
    pipeline_hash="$(sha256sum "$ROOT/Source/HALVETHVoid/Public/VoidField.h" "$ROOT/Source/HALVETHVoid/Public/VoidSoul.h" "$ROOT/scripts/soulseed.py" "$ROOT/Source/HALVETHVoidEditor/Private/VoidPrepareCommandlet.cpp" "$ROOT/ArtSource/ASSETS.json" | sha256sum | cut -d ' ' -f 1)"
    if [[ "$MODE" == prepare || ! -f "$ROOT/Content/VOID/Soul_$SOUL_ID/Terrain.uasset" || ! -f "$ROOT/Saved/VOID-soul-$SOUL_ID.sha256" || "$(cat "$ROOT/Saved/VOID-soul-$SOUL_ID.sha256")" != "$pipeline_hash" || ! -f "$ROOT/Saved/VOID-imports.json" ]]; then
        if [[ -f "$ROOT/Content/VOID/Soul_$SOUL_ID/Terrain.uasset" ]]; then
            mkdir -p "$ROOT/Saved/history"
            cp -- "$ROOT/Content/VOID/Soul_$SOUL_ID/Terrain.uasset" "$ROOT/Saved/history/Terrain-$SOUL_ID-$(date -u +%Y%m%dT%H%M%SZ).uasset"
        fi
        echo "VOID: baue Welt aus Soulseed ${SOUL_ID:0:12} ..."
        "$CMD" "$WIN_PROJECT" -run=VoidPrepare "-VoidSoul=$WIN_SOUL" -RebuildTerrain -unattended -nop4 -nosound -AllowCommandletRendering "-abslog=$(cygpath -w "$ROOT/Saved/VOID-prepare.log")"
        printf '%s\n' "$pipeline_hash" > "$ROOT/Saved/VOID-soul-$SOUL_ID.sha256"
    fi
    cp -- "$SOUL_FILE" "$ROOT/Saved/VOID-active-soul.json"
}
case "$MODE" in
    download) python "$ASSET_TOOL" download ;;
    build) build ;;
    prepare) prepare ;;
    new|play)
        prepare
        exec "$EDITOR" "$WIN_PROJECT" '/Engine/Maps/Entry?game=/Script/HALVETHVoid.VoidGameMode' -game "-VoidSoul=$WIN_SOUL" -ResX=2560 -ResY=1440 -windowed -borderless -nosplash "-abslog=$(cygpath -w "$ROOT/Saved/VOID-play.log")"
        ;;
    check)
        prepare
        "$CMD" "$WIN_PROJECT" '/Engine/Maps/Entry?game=/Script/HALVETHVoid.VoidGameMode' -game "-VoidSoul=$WIN_SOUL" -VoidTest -NullRHI -unattended -nosound -log "-abslog=$(cygpath -w "$ROOT/Saved/VOID-test.log")"
        python "$ASSET_TOOL" qa
        ;;
    visual)
        prepare
        "$EDITOR" "$WIN_PROJECT" '/Engine/Maps/Entry?game=/Script/HALVETHVoid.VoidGameMode' -game "-VoidSoul=$WIN_SOUL" -VoidVisual -ResX=1920 -ResY=1080 -windowed -nosplash -nosound -log "-abslog=$(cygpath -w "$ROOT/Saved/VOID-visual.log")"
        python "$ASSET_TOOL" qa
        ;;
    soul) echo "$SOUL_ID" ;;
    *) echo 'VOID.sh [play|new|build|prepare|check|visual|download|soul] [SOULSEED oder --soul-file UTF8-DATEI]' >&2; exit 2 ;;
esac
