#!/usr/bin/env bash
set -Eeuo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
abi=${ANDROID_ABI:-armeabi-v7a}

native="$root/build/android/$abi/libspinor.so"
apk="$root/build/android/$abi/spinor-$abi.apk"

bash "$root/android/build-native.sh" "$native"
bash "$root/android/build-apk.sh" "$native" "$apk"

printf 'SPINOR_APK\t%s\n' "$apk"
