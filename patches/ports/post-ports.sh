#!/usr/bin/env bash
set -euo pipefail
PATCH_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
CAIRO_DIR="$1/cairo-1.12.18"
if [ -d "$CAIRO_DIR" ]; then
  patch -p0 -d "$CAIRO_DIR" < "$PATCH_DIR/cairo-1.12.18-wait-macros.patch"
fi
