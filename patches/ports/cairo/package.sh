#!/bin/bash

REMOTE_ARCHIVE="https://www.cairographics.org/releases/cairo-1.12.18.tar.xz"
ARCHIVE="cairo-1.12.18.tar.xz"
UNPACKED_DIR="cairo-1.12.18"

# patched flags for static-only build
CONFIGURE_OPTS="--prefix=$SYSROOT/system --disable-shared --enable-static"

port_unpack() {
  tar xf $ARCHIVE
  # apply wait.h include patch for test runner
  patch -p0 -d cairo-1.12.18 < "$ROOT/patches/ports/cairo-1.12.18-wait-macros.patch"
}

port_install() {
  cd cairo-1.12.18
  CFLAGS="${CFLAGS:---sysroot=$SYSROOT}" \
  CPPFLAGS="${CPPFLAGS:--I$SYSROOT/system/include}" \
  LDFLAGS="${LDFLAGS:--L$SYSROOT/system/lib}" \
  ./configure $CONFIGURE_OPTS
  make -j8
  make install
}
cmake --build build --target ports --clean-first