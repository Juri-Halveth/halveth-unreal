#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
[[ ${1:-build} == build && $# -le 1 ]] || { echo 'CHARACTERS.sh build' >&2; exit 2; }
blender=${GARDEN_BLENDER:-}
[[ -n "$blender" && -f "$blender" ]] || { echo 'Set GARDEN_BLENDER to an installed Blender 4.5 executable.' >&2; exit 3; }
export MSYS2_ARG_CONV_EXCL='*'
"$blender" --background --python-exit-code 1 --python "$(cygpath -w "$root/scripts/generate_characters.py")"
"${GARDEN_PYTHON:-python}" "$(cygpath -w "$root/scripts/generate_fabric.py")"
"$blender" --background --python-exit-code 1 --python "$(cygpath -w "$root/scripts/sanitize_character_fbx.py")"
"${GARDEN_PYTHON:-python}" "$(cygpath -w "$root/scripts/index_character_sources.py")"
printf 'Character sources generated. GARDEN.sh prepare binds them to native runtime assets.\n'
