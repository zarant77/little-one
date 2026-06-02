#!/usr/bin/env bash

set -euo pipefail

APP_ID="com.catemup.littleone"
SOURCE_APK="app/build/outputs/apk/release/app-release-unsigned.apk"
BUILD_DIR="build"
OUTPUT_APK="$BUILD_DIR/little-one.apk"

SDK="${ANDROID_HOME:-$HOME/Library/Android/sdk}"
BUILD_TOOLS_DIR="$(ls -d "$SDK"/build-tools/* | sort -V | tail -n 1)"
APKSIGNER="$BUILD_TOOLS_DIR/apksigner"
KEYSTORE="$HOME/.android/debug.keystore"

ensure_tools() {
  if [ ! -x "$APKSIGNER" ]; then
    echo "ERROR: apksigner not found: $APKSIGNER"
    exit 1
  fi
}

ensure_keystore() {
  if [ -f "$KEYSTORE" ]; then
    return
  fi

  echo "Creating debug keystore..."
  mkdir -p "$HOME/.android"

  keytool -genkeypair \
    -v \
    -keystore "$KEYSTORE" \
    -storepass android \
    -alias androiddebugkey \
    -keypass android \
    -keyalg RSA \
    -keysize 2048 \
    -validity 10000 \
    -dname "CN=Android Debug,O=Android,C=US"
}

show_logo() {
  cat <<'EOF'
 _      _ _   _   _        ___
| |    (_) | | | | |      / _ \
| |     _| |_| |_| | ___ | | | |_ __   ___
| |    | | __| __| |/ _ \| | | | '_ \ / _ \
| |____| | |_| |_| |  __/| |_| | | | |  __/
|______|_|\__|\__|_|\___| \___/|_| |_|\___|

Tiny cat. Big world.
EOF
}

pack_assets() {
  echo
  echo "Packing assets JSON files..."
  python3 tools/pack_sprites.py
  python3 tools/pack_animations.py
}

sign_apk() {
  "$APKSIGNER" sign \
    --ks "$KEYSTORE" \
    --ks-key-alias androiddebugkey \
    --ks-pass pass:android \
    --key-pass pass:android \
    "$SOURCE_APK"
}

verify_apk() {
  "$APKSIGNER" verify "$SOURCE_APK"
}

build_apk() {
  ensure_tools
  ensure_keystore

  pack_assets

  echo
  echo "SDK: $SDK"
  echo "Build Tools: $BUILD_TOOLS_DIR"
  echo "APK Signer: $APKSIGNER"
  echo

  ./gradlew clean assembleRelease

  sign_apk
  verify_apk

  mkdir -p "$BUILD_DIR"
  cp "$SOURCE_APK" "$OUTPUT_APK"

  show_apk_size
}

install_apk() {
  if [ ! -f "$OUTPUT_APK" ]; then
    echo "ERROR: APK not found. Build first."
    exit 1
  fi

  adb install -r -d "$OUTPUT_APK"
}

launch_app() {
  adb shell monkey -p "$APP_ID" 1
}

show_logs() {
  adb logcat -c
  adb logcat | grep LittleOne
}

clean_project() {
  ./gradlew clean
  rm -rf "$BUILD_DIR"
}

show_apk_size() {
  if [ ! -f "$OUTPUT_APK" ]; then
    echo "ERROR: APK not found."
    return
  fi

  echo
  echo "APK:"
  ls -lh "$OUTPUT_APK"

  echo
  echo "APK contents:"
  unzip -l "$OUTPUT_APK" | tail -n 20
}

show_devices() {
  adb devices
}

while true; do
  clear
  show_logo

  echo
  echo "1) Pack Assets JSON -> C"
  echo "2) Build"
  echo "3) Build + Install + Launch"
  echo "4) Clean"
  echo "5) Logs"
  echo "6) APK size"
  echo "7) Devices"
  echo "0) Exit"
  echo

  read -rp "> " choice

  case "$choice" in
    1) pack_assets ;;
    2) build_apk ;;
    3) build_apk; install_apk; launch_app ;;
    4) clean_project ;;
    5) show_logs ;;
    6) show_apk_size ;;
    7) show_devices ;;
    0) exit 0 ;;
    *) echo "Invalid option" ;;
  esac

  echo
  read -rp "Press Enter to continue..."
done