#!/bin/sh
# This script builds DCTelnet Lha package under Linux/POSIX Shell
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
BUILD_DIR="$ROOT_DIR/build"
STAGING_DIR=$(mktemp -d "${TMPDIR:-/tmp}/dctelnet-package.XXXXXX")
trap 'rm -rf "$STAGING_DIR"' 0

echo "Build the LHA Debug binary only package:"
DEBUG_DIR="$STAGING_DIR/debug"
RELEASE_DIR="$STAGING_DIR/release"
mkdir -p "$DEBUG_DIR" "$RELEASE_DIR"

cp "$BUILD_DIR/vbcc-68000-debug/DCTelnet.68000"      "$DEBUG_DIR/DCTelnet-debug"
cp "$BUILD_DIR/package/DCTelnet/DCTelnet.info"       "$DEBUG_DIR/DCTelnet-debug.info"
cp "$ROOT_DIR/docs/debug/FILE_ID.DIZ"                "$DEBUG_DIR/FILE_ID.DIZ"

rm -f "$BUILD_DIR/DCTelnet-debug.lha"
(
    cd "$DEBUG_DIR"
    lha a -q "$BUILD_DIR/DCTelnet-debug.lha" DCTelnet-debug DCTelnet-debug.info FILE_ID.DIZ
)
printf 'Created %s\n' "$BUILD_DIR/DCTelnet-debug.lha"

echo "Build the LHA package:"
cp -R "$BUILD_DIR/package/." "$RELEASE_DIR/"
mkdir -p "$RELEASE_DIR/DCTelnet/Devs" "$RELEASE_DIR/DCTelnet/Docs"
cp "$BUILD_DIR/vbcc-68000-release/DCTelnet.68000"    "$RELEASE_DIR/DCTelnet/DCTelnet"
cp "$BUILD_DIR/vbcc-68020-release/DCTelnet.68020"    "$RELEASE_DIR/DCTelnet/DCTelnet.020"
cp "$ROOT_DIR/ibmcon/ibmcon.device"                  "$RELEASE_DIR/DCTelnet/Devs/ibmcon.device"
cp "$ROOT_DIR/LICENSE"                               "$RELEASE_DIR/DCTelnet/Docs/"
cp "$ROOT_DIR"/docs/DCTelnet.*                       "$RELEASE_DIR/DCTelnet/Docs/"
cp "$ROOT_DIR/docs/FILE_ID.DIZ"                      "$RELEASE_DIR/FILE_ID.DIZ"

# Match the Amiga script: remove the root icon and icons for immediate subdirectories.
find "$RELEASE_DIR/DCTelnet" -mindepth 1 -maxdepth 2 -type f -name '.info' -delete

rm -f "$BUILD_DIR/DCTelnet.lha"
(
    cd "$RELEASE_DIR"
    lha a -q "$BUILD_DIR/DCTelnet.lha" DCTelnet DCTelnet.info FILE_ID.DIZ
)

printf 'Created %s\n' "$BUILD_DIR/DCTelnet.lha"