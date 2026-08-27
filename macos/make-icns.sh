#!/usr/bin/env sh
# Regenerate macos/omacut.icns from the packaged SVG icon.
# Needs librsvg (`brew install librsvg`); iconutil ships with macOS.
# The .icns is committed, so this only needs running when the SVG changes.
set -eu

HERE="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
SRC="$HERE/../pkgbuild/omacut.svg"
SET="$HERE/omacut.iconset"

command -v rsvg-convert >/dev/null 2>&1 || {
  echo "rsvg-convert is required (brew install librsvg)." >&2; exit 1; }

rm -rf "$SET"
mkdir -p "$SET"
render() { rsvg-convert -w "$1" -h "$1" "$SRC" -o "$SET/icon_$2.png"; }

render 16   16x16
render 32   16x16@2x
render 32   32x32
render 64   32x32@2x
render 128  128x128
render 256  128x128@2x
render 256  256x256
render 512  256x256@2x
render 512  512x512
render 1024 512x512@2x

iconutil -c icns "$SET" -o "$HERE/omacut.icns"
rm -rf "$SET"
echo "Wrote $HERE/omacut.icns"
