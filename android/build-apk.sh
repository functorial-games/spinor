#!/usr/bin/env bash
set -Eeuo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

native_library=${1:-"$root/build/android/libspinor.so"}
output=${2:-"$root/build/android/spinor.apk"}
abi=${ANDROID_ABI:-armeabi-v7a}

: "${ANDROID_NDK_CHECKOUT:?ANDROID_NDK_CHECKOUT must name the canonical isomorphisms/android-NDK checkout}"

packager="$ANDROID_NDK_CHECKOUT/apk/build-nativeactivity-apk.sh"
[[ -f "$packager" ]] || {
    printf 'canonical NativeActivity packager is missing: %s\n' "$packager" >&2
    exit 2
}

export ANDROID_PACKAGE_ID=${SPINOR_PACKAGE_ID:-org.functorialgames.spinor}
export ANDROID_VERSION_CODE=${SPINOR_VERSION_CODE:-1}
export ANDROID_VERSION_NAME=${SPINOR_VERSION_NAME:-0.1}
export ANDROID_MIN_SDK=${ANDROID_API:-21}
export ANDROID_TARGET_SDK=${SPINOR_TARGET_SDK:-34}

export ANDROID_KEYSTORE=${SPINOR_KEYSTORE:?SPINOR_KEYSTORE is required}
export ANDROID_KEYSTORE_TYPE=${SPINOR_KEYSTORE_TYPE:?SPINOR_KEYSTORE_TYPE is required}
export ANDROID_KEY_ALIAS=${SPINOR_KEY_ALIAS:?SPINOR_KEY_ALIAS is required}
export ANDROID_STORE_PASSWORD=${SPINOR_STORE_PASSWORD:?SPINOR_STORE_PASSWORD is required}
export ANDROID_KEY_PASSWORD=${SPINOR_KEY_PASSWORD:?SPINOR_KEY_PASSWORD is required}
export ANDROID_EXPECTED_CERT_SHA256=${SPINOR_EXPECTED_CERT_SHA256:?SPINOR_EXPECTED_CERT_SHA256 is required}
export ANDROID_REQUIRE_NO_DEX=1
export ANDROID_SOURCE_COMMIT=${ANDROID_SOURCE_COMMIT:-$(git -C "$root" rev-parse HEAD)}

exec bash "$packager"     "$root/android/AndroidManifest.xml"     "$native_library"     "$abi"     "$output"
