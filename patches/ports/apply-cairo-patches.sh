#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
CAIRO_SRC="$1"
patch -p0 -d "$CAIRO_SRC" < "$SCRIPT_DIR/cairo-1.12.18-wait-macros.patch"
