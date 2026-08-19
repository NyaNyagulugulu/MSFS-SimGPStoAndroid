#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
compiler="${MINGW_CXX:-x86_64-w64-mingw32-g++}"
output="${1:-$project_root/build/wine/MSFS-SimConnect-WineBridge.exe}"
mkdir -p "$(dirname "$output")"

"$compiler" \
    -std=c++20 \
    -O2 \
    -DMSFS_USE_MINIMAL_SIMCONNECT \
    -I"$project_root/CPP/MSFSSimConnect/qt" \
    "$project_root/CPP/MSFSSimConnect/qt/wine_bridge.cpp" \
    -static-libgcc \
    -static-libstdc++ \
    -lws2_32 \
    -o "$output"

file "$output"
