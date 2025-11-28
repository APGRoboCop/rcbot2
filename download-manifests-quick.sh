#!/bin/bash
################################################################################
# Quick Fix - Download hl2sdk-manifests files from Metamod:Source
################################################################################

set -e

PROJECT_ROOT="$(pwd)"
MANIFESTS_DIR="${PROJECT_ROOT}/hl2sdk-manifests"

echo "Creating hl2sdk-manifests directory..."
mkdir -p "$MANIFESTS_DIR"

echo "Downloading SDK helper files from Metamod:Source..."

# Base URL for raw files from Metamod:Source GitHub
BASE_URL="https://raw.githubusercontent.com/alliedmodders/metamod-source/master/hl2sdk-manifests"

# Download the essential files
files=(
    "SdkHelpers.ambuild"
    "sdks.json"
)

for file in "${files[@]}"; do
    echo "  Downloading $file..."
    wget -q "$BASE_URL/$file" -O "$MANIFESTS_DIR/$file" || curl -sS "$BASE_URL/$file" -o "$MANIFESTS_DIR/$file"
    
    if [ -f "$MANIFESTS_DIR/$file" ]; then
        echo "    ✓ $file"
    else
        echo "    ✗ Failed to download $file"
        exit 1
    fi
done

echo ""
echo "✓ hl2sdk-manifests files downloaded successfully!"
echo ""
echo "Files in $MANIFESTS_DIR:"
ls -lh "$MANIFESTS_DIR"
echo ""
echo "You can now run: ./build-linux.sh --auto"
