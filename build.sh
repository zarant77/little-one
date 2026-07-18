#!/usr/bin/env bash

set -euo pipefail

APP_ID="com.catemup.littleone"

SOURCE_APK_SIGNED="app/build/outputs/apk/release/app-release.apk"
SOURCE_AAB="app/build/outputs/bundle/release/app-release.aab"

BUILD_DIR="build"
OUTPUT_APK="$BUILD_DIR/little-one.apk"
OUTPUT_AAB="$BUILD_DIR/little-one.aab"

SDK="${ANDROID_HOME:-$HOME/Library/Android/sdk}"
export ANDROID_HOME="$SDK"
BUILD_TOOLS_DIR="$(ls -d "$SDK"/build-tools/* | sort -V | tail -n 1)"
APKSIGNER="$BUILD_TOOLS_DIR/apksigner"
ADB="$SDK/platform-tools/adb"

ensure_tools() {
    if [ ! -x "$APKSIGNER" ]; then
        echo "ERROR: apksigner not found: $APKSIGNER"
        exit 1
    fi
}

ensure_adb() {
    if [ ! -x "$ADB" ]; then
        echo "ERROR: adb not found: $ADB"
        exit 1
    fi
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

unify_colors() {
    echo
    echo "Unifying colors..."
    
    python3 tools/unify_sprite_colors.py src/assets/sprites \
    --write \
    --target-colors 256 \
    --force-low-usage \
    --max-uses 2 \
    --force-max-distance 36 \
    --report color_report.txt
}

pack_assets() {
    echo
    echo "Packing assets JSON files..."
    python3 tools/pack_animations.py
    python3 tools/pack_fonts.py
    python3 tools/pack_music.py
    python3 tools/pack_sfx.py
    python3 tools/pack_sprites.py
}

show_build_outputs() {
    echo
    echo "Existing build outputs:"
    find app/build/outputs -type f \( -name "*.apk" -o -name "*.aab" \) 2>/dev/null || true
}

get_release_apk() {
    if [ -f "$SOURCE_APK_SIGNED" ]; then
        echo "$SOURCE_APK_SIGNED"
        return
    fi
    
    echo "ERROR: Release APK not found." >&2
    echo "Expected: $SOURCE_APK_SIGNED" >&2
    show_build_outputs >&2
    exit 1
}

verify_apk() {
    local apk
    apk="$(get_release_apk)"
    
    echo
    echo "Verifying APK:"
    echo "$apk"
    
    "$APKSIGNER" verify "$apk"
}

verify_aab() {
    if [ ! -f "$SOURCE_AAB" ]; then
        echo "ERROR: AAB not found:"
        echo "$SOURCE_AAB"
        show_build_outputs
        exit 1
    fi
    
    echo
    echo "Verifying AAB:"
    echo "$SOURCE_AAB"
    
    jarsigner -verify "$SOURCE_AAB" >/dev/null
}

build_apk() {
    ensure_tools
    
    pack_assets
    
    echo
    echo "SDK: $SDK"
    echo "Build Tools: $BUILD_TOOLS_DIR"
    echo "APK Signer: $APKSIGNER"
    echo
    
    ./gradlew clean :app:assembleRelease
    
    verify_apk
    
    mkdir -p "$BUILD_DIR"
    cp "$(get_release_apk)" "$OUTPUT_APK"
    
    show_apk_size
}

build_aab() {
    pack_assets
    
    echo
    echo "Building Android App Bundle..."
    echo
    
    ./gradlew clean :app:bundleRelease
    
    if [ ! -f "$SOURCE_AAB" ]; then
        echo "ERROR: AAB not found:"
        echo "$SOURCE_AAB"
        show_build_outputs
        exit 1
    fi
    
    verify_aab
    
    mkdir -p "$BUILD_DIR"
    cp "$SOURCE_AAB" "$OUTPUT_AAB"
    
    show_aab_size
}

build_release() {
    ensure_tools
    
    pack_assets
    
    echo
    echo "SDK: $SDK"
    echo "Build Tools: $BUILD_TOOLS_DIR"
    echo "APK Signer: $APKSIGNER"
    echo
    
    ./gradlew clean :app:assembleRelease :app:bundleRelease
    
    verify_apk
    verify_aab
    
    if [ ! -f "$SOURCE_AAB" ]; then
        echo "ERROR: AAB not found:"
        echo "$SOURCE_AAB"
        show_build_outputs
        exit 1
    fi
    
    mkdir -p "$BUILD_DIR"
    cp "$(get_release_apk)" "$OUTPUT_APK"
    cp "$SOURCE_AAB" "$OUTPUT_AAB"
    
    show_apk_size
    show_aab_size
}

install_apk() {
    ensure_adb
    if [ ! -f "$OUTPUT_APK" ]; then
        echo "ERROR: APK not found. Build first."
        exit 1
    fi
    
    "$ADB" install -r -d "$OUTPUT_APK"
}

launch_app() {
    "$ADB" shell monkey -p "$APP_ID" 1
}

show_logs() {
    ensure_adb
    "$ADB" logcat -c
    "$ADB" logcat | grep LittleOne
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

show_aab_size() {
    if [ ! -f "$OUTPUT_AAB" ]; then
        echo "ERROR: AAB not found."
        return
    fi
    
    echo
    echo "AAB:"
    ls -lh "$OUTPUT_AAB"
    
    echo
    echo "AAB contents:"
    unzip -l "$OUTPUT_AAB" | tail -n 20
}

show_devices() {
    ensure_adb
    "$ADB" devices
}

if [ -t 1 ]; then
    clear
fi
show_logo

echo
echo "1) Build APK + Install + Launch"
echo "2) Build APK"
echo "3) Build AAB"
echo "4) Build APK + AAB"
echo "5) Pack Assets JSON -> C"
echo "6) Unify Colors"
echo "7) Clean"
echo "8) Logs"
echo "9) APK size"
echo "10) AAB size"
echo "11) Devices"
echo "12) Build outputs"
echo "0) Exit"
echo

read -rp "> " choice

case "$choice" in
    1) build_apk; install_apk; launch_app ;;
    2) build_apk ;;
    3) build_aab ;;
    4) build_release ;;
    5) pack_assets ;;
    6) unify_colors ;;
    7) clean_project ;;
    8) show_logs ;;
    9) show_apk_size ;;
    10) show_aab_size ;;
    11) show_devices ;;
    12) show_build_outputs ;;
    0) exit 0 ;;
    *) echo "Invalid option"; exit 1 ;;
esac
