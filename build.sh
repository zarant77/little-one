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

  echo "SDK: $SDK"
  echo "Build Tools: $BUILD_TOOLS_DIR"
  echo "APK Signer: $APKSIGNER"

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

uninstall_app() {
  adb uninstall "$APP_ID" || true
}

show_logs() {
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
  echo
  echo "=== Little One ==="
  echo "1) Build"
  echo "2) Install"
  echo "3) Launch"
  echo "4) Logs"
  echo "5) Build + Install"
  echo "6) Build + Install + Launch"
  echo "7) Uninstall"
  echo "8) Clean"
  echo "9) APK size"
  echo "10) Devices"
  echo "0) Exit"
  echo

  read -rp "> " choice

  case "$choice" in
    1) build_apk ;;
    2) install_apk ;;
    3) launch_app ;;
    4) show_logs ;;
    5) build_apk; install_apk ;;
    6) build_apk; install_apk; launch_app ;;
    7) uninstall_app ;;
    8) clean_project ;;
    9) show_apk_size ;;
    10) show_devices ;;
    0) exit 0 ;;
    *) echo "Invalid option" ;;
  esac
done