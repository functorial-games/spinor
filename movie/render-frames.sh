#!/bin/sh
set -eu

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH= cd -- "$here/.." && pwd)

output_directory=${1:-"$root/build/movie/frames"}
frame_count=${SPINOR_MOVIE_FRAMES:-240}
width=${SPINOR_MOVIE_WIDTH:-480}
height=${SPINOR_MOVIE_HEIGHT:-480}

case $frame_count in
    ''|*[!0-9]*) printf '%s\n' "invalid SPINOR_MOVIE_FRAMES: $frame_count" >&2; exit 2 ;;
esac
case $width in
    ''|*[!0-9]*) printf '%s\n' "invalid SPINOR_MOVIE_WIDTH: $width" >&2; exit 2 ;;
esac
case $height in
    ''|*[!0-9]*) printf '%s\n' "invalid SPINOR_MOVIE_HEIGHT: $height" >&2; exit 2 ;;
esac

[ "$frame_count" -ge 2 ] || {
    printf '%s\n' "SPINOR_MOVIE_FRAMES must be at least 2" >&2
    exit 2
}
[ "$width" -ge 64 ] && [ "$height" -ge 64 ] || {
    printf '%s\n' "movie dimensions must be at least 64x64" >&2
    exit 2
}

build_directory=$root/build/movie
renderer=$build_directory/render-frames

mkdir -p "$build_directory" "$output_directory"
rm -f "$output_directory"/frame-*.ppm
rm -f "$output_directory"/trajectory.tsv
rm -f "$output_directory"/frames.sha256

"${CC:-cc}" \
    -std=c11 \
    -O2 \
    -Wall \
    -Wextra \
    -Werror \
    "$here/render_frames.c" \
    "$root/native/spinor_core.c" \
    "$root/native/spinor_field.c" \
    "$root/native/spinor_ribbons.c" \
    -lm \
    -o "$renderer"

"$renderer" \
    "$output_directory" \
    "$frame_count" \
    "$width" \
    "$height"

actual=$(
    find "$output_directory" \
        -maxdepth 1 \
        -type f \
        -name 'frame-*.ppm' \
        -print |
    wc -l |
    tr -d ' '
)

[ "$actual" = "$frame_count" ] || {
    printf '%s\n' \
        "expected $frame_count frames, found $actual" >&2
    exit 1
}

(
    cd "$output_directory"
    sha256sum frame-*.ppm > frames.sha256
)

printf '%s\n' \
    "spinor movie frames: $frame_count" \
    "dimensions: ${width}x${height}" \
    "trajectory: $output_directory/trajectory.tsv" \
    "checksums: $output_directory/frames.sha256"
