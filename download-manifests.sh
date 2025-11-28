#!/bin/bash
################################################################################
# HL2SDK Manifests Download Script
################################################################################
# This script downloads the HL2SDK manifest files from GitHub when git
# submodules cannot be used or as a fallback mechanism.
#
# It uses raw GitHub URLs to fetch the actual file content instead of HTML.
################################################################################

set -e  # Exit on error
set -o pipefail  # Catch errors in pipes

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MANIFEST_DIR="${SCRIPT_DIR}/hl2sdk-manifests"
GITHUB_RAW_BASE="https://raw.githubusercontent.com/alliedmodders/hl2sdk-manifests/master"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

log_info() {
    echo -e "${CYAN}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Validate downloaded file is Python code, not HTML error page
validate_file() {
    local file="$1"
    local filename=$(basename "$file")

    # Check if file exists and is not empty
    if [ ! -s "$file" ]; then
        log_error "$filename: File is empty or doesn't exist"
        return 1
    fi

    # Check for HTML error indicators
    if grep -q "404: Not Found\|<html>\|<HTML>\|<!DOCTYPE" "$file" 2>/dev/null; then
        log_error "$filename: Downloaded HTML error page instead of actual file"
        log_error "Content preview:"
        head -n 5 "$file"
        return 1
    fi

    # For .ambuild files, verify they contain valid Python code
    if [[ "$filename" == *.ambuild ]]; then
        # Check for common Python keywords
        if ! grep -q "def\|class\|import" "$file" 2>/dev/null; then
            log_warning "$filename: Doesn't appear to contain Python code"
            return 1
        fi
    fi

    log_success "$filename: Validated successfully"
    return 0
}

# Download a single file with validation
download_file() {
    local filename="$1"
    local url="${GITHUB_RAW_BASE}/${filename}"
    local dest="${MANIFEST_DIR}/${filename}"

    log_info "Downloading: $filename"
    log_info "URL: $url"

    # Create directory if needed
    local dest_dir=$(dirname "$dest")
    mkdir -p "$dest_dir"

    # Download with error handling
    if wget -q --show-progress -O "$dest" "$url" 2>&1; then
        # Validate the downloaded file
        if validate_file "$dest"; then
            return 0
        else
            log_error "Validation failed for $filename"
            rm -f "$dest"
            return 1
        fi
    else
        log_error "Failed to download $filename from $url"
        rm -f "$dest"
        return 1
    fi
}

# Download with retry logic
download_with_retry() {
    local filename="$1"
    local max_retries=3
    local retry=0

    while [ $retry -lt $max_retries ]; do
        if download_file "$filename"; then
            return 0
        fi

        retry=$((retry + 1))
        if [ $retry -lt $max_retries ]; then
            log_warning "Retry $retry/$max_retries for $filename"
            sleep 2
        fi
    done

    log_error "Failed to download $filename after $max_retries attempts"
    return 1
}

main() {
    log_info "HL2SDK Manifests Download Script"
    log_info "================================="
    echo ""

    # Check if wget is available
    if ! command -v wget &> /dev/null; then
        log_error "wget is not installed"
        log_error "Install with: sudo apt-get install wget"
        exit 1
    fi

    # Create manifest directory
    log_info "Creating manifest directory: $MANIFEST_DIR"
    mkdir -p "$MANIFEST_DIR"

    # List of files to download
    local files=(
        "SdkHelpers.ambuild"
        "README.md"
    )

    log_info "Downloading ${#files[@]} manifest files from GitHub..."
    echo ""

    local failed_files=()
    local success_count=0

    for file in "${files[@]}"; do
        if download_with_retry "$file"; then
            success_count=$((success_count + 1))
        else
            failed_files+=("$file")
        fi
        echo ""
    done

    # Summary
    echo ""
    log_info "Download Summary"
    log_info "================"
    log_success "Successfully downloaded: $success_count/${#files[@]} files"

    if [ ${#failed_files[@]} -gt 0 ]; then
        log_error "Failed downloads:"
        for file in "${failed_files[@]}"; do
            echo "  - $file"
        done
        echo ""
        log_error "Some files failed to download!"
        log_info "You can try:"
        log_info "  1. Using git submodules instead: git submodule update --init hl2sdk-manifests"
        log_info "  2. Checking your internet connection"
        log_info "  3. Manually downloading from: https://github.com/alliedmodders/hl2sdk-manifests"
        exit 1
    else
        log_success "All manifest files downloaded successfully!"
        log_info "Manifest directory: $MANIFEST_DIR"

        # List downloaded files
        echo ""
        log_info "Downloaded files:"
        ls -lh "$MANIFEST_DIR"

        echo ""
        log_success "You can now run configure.py"
    fi
}

main "$@"
