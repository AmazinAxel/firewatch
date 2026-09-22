#!/usr/bin/env bash
# Build and run the host-side dashboard preview; outputs PNGs into
# tools/preview/out/. Needs the GFX lib fetched once via `pio run`.
# Run inside `nix develop` (provides g++ and imagemagick).
set -euo pipefail
cd "$(dirname "$0")/../.."

GFX=".pio/libdeps/seeed_xiao_esp32s3/Adafruit GFX Library"
OUT=tools/preview/out
mkdir -p "$OUT"

g++ -std=c++17 -O2 -DARDUINO=200 \
    -Itools/preview/stub -Iinclude -Isrc -I"$GFX" \
    tools/preview/preview.cpp src/render.cpp src/chart.cpp src/aqi.cpp \
    src/timeutil.cpp src/icon_map.cpp "$GFX/Adafruit_GFX.cpp" \
    -o "$OUT/preview"

(cd "$OUT" && ./preview)
for f in "$OUT"/*.pgm; do
    magick "$f" "${f%.pgm}.png" && rm "$f"
done
echo "previews in $OUT/"
