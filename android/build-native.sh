#!/usr/bin/env bash
set -Eeuo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

output=${1:-"$root/build/android/libspinor.so"}
abi=${ANDROID_ABI:-armeabi-v7a}
api=${ANDROID_API:-21}

case "$abi" in
    armeabi-v7a) target=armv7a-linux-androideabi; ick_target=arm-linux-gnueabi; header_target=arm-linux-androideabi; ick_flags=(-marm -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp) ;;
    arm64-v8a) target=aarch64-linux-android; ick_target=aarch64-linux-gnu; header_target=aarch64-linux-android; ick_flags=(-ffixed-x18) ;;
    x86) target=i686-linux-android; ick_target=i686-linux-gnu; header_target=i686-linux-android; ick_flags=(-march=i686 -mssse3 -mfpmath=sse -mstackrealign) ;;
    x86_64) target=x86_64-linux-android; ick_target=x86_64-linux-gnu; header_target=x86_64-linux-android; ick_flags=(-march=x86-64-v2 -mno-avx -mno-movbe) ;;
    *)
        printf 'unsupported Android ABI: %s\n' "$abi" >&2
        exit 2
        ;;
esac

ick=${ICK_CC:-${ICK_ROOT:+$ICK_ROOT/bin/${ick_target}-gcc}}
[[ -n $ick && -x $ick ]] || {
    printf 'Set ICK_CC or ICK_ROOT for %s.\n' "$ick_target" >&2
    exit 2
}
[[ $("$ick" -dumpmachine) == "$ick_target" ]] || exit 2
builtin_include=$("$ick" -print-file-name=include)
[[ -f $builtin_include/stddef.h ]] || exit 2

ndk=${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}
if [[ -z $ndk ]]; then
    android_home=${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}
    [[ -n $android_home ]] || {
        echo 'ANDROID_NDK_HOME or ANDROID_HOME is required' >&2
        exit 2
    }
    ndk=$(
        find "$android_home/ndk" -mindepth 1 -maxdepth 1 -type d |
            sort -V |
            tail -n 1
    )
fi

[[ -d $ndk ]] || {
    printf 'Android NDK not found: %s\n' "$ndk" >&2
    exit 2
}

toolchain=$(
    find "$ndk/toolchains/llvm/prebuilt"         -mindepth 1 -maxdepth 1 -type d |
        head -n 1
)
[[ -n $toolchain ]] || {
    echo 'Android NDK LLVM toolchain not found' >&2
    exit 2
}

clang="$toolchain/bin/${target}${api}-clang"
readelf="$toolchain/bin/llvm-readelf"
strip="$toolchain/bin/llvm-strip"
glue_dir="$ndk/sources/android/native_app_glue"
glue_source="$glue_dir/android_native_app_glue.c"

for required in "$clang" "$readelf" "$strip" "$glue_source"; do
    [[ -e $required ]] || {
        printf 'missing Android build input: %s\n' "$required" >&2
        exit 2
    }
done

mkdir -p "$(dirname -- "$output")"
work=$(mktemp -d "${RUNNER_TEMP:-${TMPDIR:-/tmp}}/spinor-native.XXXXXX")
trap 'rm -rf "$work"' EXIT

common_compile=(
    -std=c11
    -O2
    -fPIC
    -I "$glue_dir"
    -I "$root/native"
    -I "$root/android"
)

strict_compile=(
    "${common_compile[@]}"
    -Wall
    -Wextra
    -Werror
    -Wpedantic
)

sources=(
    "$root/android/spinor_android.c"
    "$root/android/spinor_renderer.c"
    "$root/native/spinor_core.c"
    "$root/native/spinor_field.c"
    "$root/native/spinor_ribbons.c"
)

objects=()
sysroot="$toolchain/sysroot"
for source in "${sources[@]}"; do
    base=$(basename -- "$source" .c)
    object="$work/$base.o"
    "$ick" "${ick_flags[@]}" "${strict_compile[@]}" \
        --sysroot="$sysroot" -nostdinc -isystem "$builtin_include" \
        -isystem "$sysroot/usr/include" -isystem "$sysroot/usr/include/$header_target" \
        -D__ANDROID__ -D__ANDROID_API__="$api" -D__ANDROID_MIN_SDK_VERSION__="$api" \
        -DBIONIC_IOCTL_NO_SIGNEDNESS_OVERLOAD -S "$source" -o "$work/$base.s"
    "$clang" "${ick_flags[@]}" -fPIC -c "$work/$base.s" -o "$object"
    objects+=("$object")
done

# android_native_app_glue is vendored by the NDK. Keep our sources fail-closed
# under strict warnings without promoting upstream format-pedantic warnings to
# application defects.
glue_object="$work/android_native_app_glue.o"
"$clang"     "${common_compile[@]}"     -Wall     -Wextra     -Wno-format-pedantic     -c "$glue_source"     -o "$glue_object"
objects+=("$glue_object")

link_alignment=()
case "$abi" in
    arm64-v8a|x86_64)
        link_alignment=(
            -Wl,-z,max-page-size=16384
            -Wl,-z,common-page-size=16384
        )
        ;;
esac

"$clang"     -shared     "${objects[@]}"     -Wl,--no-undefined     -Wl,-soname,libspinor.so     "${link_alignment[@]}"     -landroid     -llog     -lEGL     -lGLESv2     -lm     -o "$output"

"$strip" --strip-unneeded "$output"

symbols=$("$readelf" -Ws "$output")
grep -Fq 'ANativeActivity_onCreate' <<<"$symbols"
grep -Fq 'android_main' <<<"$symbols"

printf 'SPINOR_NATIVE\tPASS\n'
printf 'ABI\t%s\n' "$abi"
printf 'API\t%s\n' "$api"
printf 'LIBRARY\t%s\n' "$output"
printf 'C_FRONTEND\t%s\n' "$ick"
