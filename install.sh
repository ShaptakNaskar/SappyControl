#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${project_dir}/build"
install_prefix="${SAPPY_CONTROLS_PREFIX:-${HOME}/.local}"

cmake -S "$project_dir" -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir"
ctest --test-dir "$build_dir" --output-on-failure

cmake --install "$build_dir" --prefix "$install_prefix"

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$install_prefix/share/applications"
fi

printf '\nInstalled Sappy\x27s Controls to %s. Open it from the application menu or run: %s/bin/sappy-controls\n' "$install_prefix" "$install_prefix"
