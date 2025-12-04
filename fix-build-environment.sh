#!/bin/bash
################################################################################
# RCBot2 Build Environment Fix Script
################################################################################
# This script fixes environment mismatches between your local machine and the
# working build environment. It addresses:
#
# 1. Missing/uninitialized git submodules (especially hl2sdk-manifests)
# 2. Missing 32-bit development libraries (linux-libc-dev:i386)
# 3. Git proxy issues that prevent submodule initialization
# 4. Missing manifest directory structure
# 5. Broken submodule configurations
#
# Usage:
#   ./fix-build-environment.sh              # Interactive mode
#   ./fix-build-environment.sh --auto       # Auto-fix everything
#   ./fix-build-environment.sh --no-sudo    # Skip steps requiring sudo
################################################################################

set -e  # Exit on error
trap 'handle_error $? $LINENO' ERR

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AUTO_MODE=false
NO_SUDO=false
LOG_FILE="${SCRIPT_DIR}/fix-environment.log"

# Parse arguments
for arg in "$@"; do
    case $arg in
        --auto) AUTO_MODE=true ;;
        --no-sudo) NO_SUDO=true ;;
        --help)
            echo "RCBot2 Build Environment Fix Script"
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --auto      Auto-fix without prompts"
            echo "  --no-sudo   Skip steps requiring sudo"
            echo "  --help      Show this help"
            exit 0
            ;;
    esac
done

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
NC='\033[0m'

log_info() {
    echo -e "${CYAN}[INFO]${NC} $1" | tee -a "$LOG_FILE"
}

log_success() {
    echo -e "${GREEN}[✓ SUCCESS]${NC} $1" | tee -a "$LOG_FILE"
}

log_error() {
    echo -e "${RED}[✗ ERROR]${NC} $1" | tee -a "$LOG_FILE"
}

log_warning() {
    echo -e "${YELLOW}[! WARNING]${NC} $1" | tee -a "$LOG_FILE"
}

log_action() {
    echo -e "${BLUE}[→]${NC} $1" | tee -a "$LOG_FILE"
}

log_section() {
    echo "" | tee -a "$LOG_FILE"
    echo -e "${BOLD}${MAGENTA}═══════════════════════════════════════════════════════════${NC}" | tee -a "$LOG_FILE"
    echo -e "${BOLD}${MAGENTA} $1${NC}" | tee -a "$LOG_FILE"
    echo -e "${BOLD}${MAGENTA}═══════════════════════════════════════════════════════════${NC}" | tee -a "$LOG_FILE"
    echo "" | tee -a "$LOG_FILE"
}

handle_error() {
    local exit_code=$1
    local line_number=$2
    log_error "Script failed at line $line_number with exit code $exit_code"
    log_info "Check log file: $LOG_FILE"
    exit $exit_code
}

confirm() {
    if [ "$AUTO_MODE" = true ]; then
        return 0
    fi

    local prompt="$1"
    while true; do
        read -p "$(echo -e ${YELLOW}${prompt}${NC} [y/N]: )" response
        case $response in
            [Yy]* ) return 0 ;;
            [Nn]* | "" ) return 1 ;;
            * ) echo "Please answer yes or no." ;;
        esac
    done
}

check_sudo() {
    if [ "$NO_SUDO" = true ]; then
        return 1
    fi

    if ! sudo -n true 2>/dev/null; then
        log_warning "Some fixes require sudo privileges"
        if ! confirm "Do you want to proceed with sudo commands?"; then
            return 1
        fi
    fi
    return 0
}

################################################################################
# Fix Functions
################################################################################

fix_git_submodules() {
    log_section "Fixing Git Submodules"

    cd "$SCRIPT_DIR"

    # Check if we have .gitmodules
    if [ ! -f ".gitmodules" ]; then
        log_error ".gitmodules file not found"
        return 1
    fi

    log_info "Current git remote:"
    git remote -v | tee -a "$LOG_FILE"

    # Check if submodules are registered in .git/config
    log_action "Checking submodule registration..."
    if ! git config --get-regexp submodule >/dev/null 2>&1; then
        log_warning "Submodules not registered in .git/config"
        log_action "Re-syncing submodules..."

        # This registers submodules from .gitmodules into .git/config
        if git submodule sync 2>&1 | tee -a "$LOG_FILE"; then
            log_success "Submodules synced to .git/config"
        else
            log_warning "Submodule sync had issues (may be due to proxy)"
        fi
    fi

    # Try to initialize hl2sdk-manifests specifically
    log_action "Initializing hl2sdk-manifests submodule..."

    # First try: normal submodule update
    if git submodule update --init --recursive hl2sdk-manifests 2>&1 | tee -a "$LOG_FILE"; then
        log_success "hl2sdk-manifests initialized successfully"

        # Verify it has content
        if [ -f "${SCRIPT_DIR}/hl2sdk-manifests/SdkHelpers.ambuild" ]; then
            log_success "hl2sdk-manifests has required files"
            return 0
        else
            log_warning "hl2sdk-manifests initialized but missing files"
        fi
    else
        log_warning "Git submodule initialization failed (likely due to proxy)"
    fi

    # Fallback: Use download script
    log_action "Falling back to download-manifests.sh..."
    if [ -f "${SCRIPT_DIR}/download-manifests.sh" ]; then
        if bash "${SCRIPT_DIR}/download-manifests.sh" 2>&1 | tee -a "$LOG_FILE"; then
            log_success "Manifests downloaded via fallback script"
            return 0
        else
            log_error "Fallback download also failed"
            return 1
        fi
    else
        log_error "download-manifests.sh not found"
        return 1
    fi
}

fix_hl2sdk_manifest_structure() {
    log_section "Fixing HL2SDK Manifest Directory Structure"

    local manifests_dir="${SCRIPT_DIR}/hl2sdk-manifests"
    local manifests_subdir="${manifests_dir}/manifests"

    # Check if main directory exists
    if [ ! -d "$manifests_dir" ]; then
        log_error "hl2sdk-manifests directory not found at $manifests_dir"
        return 1
    fi

    # Check if SdkHelpers.ambuild exists
    if [ ! -f "${manifests_dir}/SdkHelpers.ambuild" ]; then
        log_error "SdkHelpers.ambuild not found"
        log_info "Run fix_git_submodules first"
        return 1
    fi

    log_success "hl2sdk-manifests directory structure is correct"

    # List contents
    log_info "Contents of hl2sdk-manifests:"
    ls -la "$manifests_dir" | tee -a "$LOG_FILE"

    return 0
}

fix_32bit_dependencies() {
    log_section "Fixing 32-bit Development Dependencies"

    if [ "$NO_SUDO" = true ]; then
        log_warning "Skipping (--no-sudo flag set)"
        return 0
    fi

    if ! check_sudo; then
        log_warning "Skipping sudo-required fixes"
        return 0
    fi

    log_info "Checking for linux-libc-dev:i386..."

    if dpkg -l | grep -q "linux-libc-dev:i386"; then
        log_success "linux-libc-dev:i386 is already installed"
        return 0
    fi

    log_warning "linux-libc-dev:i386 is missing"

    if ! confirm "Install linux-libc-dev:i386?"; then
        log_warning "Skipping 32-bit dependency installation"
        return 0
    fi

    log_action "Adding i386 architecture..."
    if sudo dpkg --add-architecture i386 2>&1 | tee -a "$LOG_FILE"; then
        log_success "i386 architecture added"
    else
        log_error "Failed to add i386 architecture"
        return 1
    fi

    log_action "Updating package lists..."
    if sudo apt-get update 2>&1 | tee -a "$LOG_FILE"; then
        log_success "Package lists updated"
    else
        log_error "Failed to update package lists"
        return 1
    fi

    log_action "Installing linux-libc-dev:i386..."
    if sudo apt-get install -y linux-libc-dev:i386 2>&1 | tee -a "$LOG_FILE"; then
        log_success "linux-libc-dev:i386 installed"
    else
        log_error "Failed to install linux-libc-dev:i386"
        return 1
    fi

    return 0
}

fix_alliedmodders_submodules() {
    log_section "Fixing AlliedModders Submodules"

    cd "$SCRIPT_DIR"

    local submodules=(
        "alliedmodders/metamod-source"
        "alliedmodders/sourcemod"
        "alliedmodders/hl2sdk-tf2"
        "alliedmodders/hl2sdk-hl2dm"
        "alliedmodders/hl2sdk-css"
        "alliedmodders/hl2sdk-dods"
    )

    log_info "Attempting to initialize core AlliedModders submodules..."
    log_warning "This may fail due to git proxy - we'll note which ones fail"

    local failed_submodules=()
    local success_count=0

    for submodule in "${submodules[@]}"; do
        log_action "Initializing $submodule..."

        if git submodule update --init --depth=1 "$submodule" 2>&1 | tee -a "$LOG_FILE"; then
            if [ -d "$submodule" ] && [ "$(ls -A $submodule 2>/dev/null)" ]; then
                log_success "$submodule initialized"
                success_count=$((success_count + 1))
            else
                log_warning "$submodule directory empty after init"
                failed_submodules+=("$submodule")
            fi
        else
            log_warning "$submodule initialization failed"
            failed_submodules+=("$submodule")
        fi
    done

    echo ""
    log_info "Submodule Summary: $success_count/${#submodules[@]} initialized"

    if [ ${#failed_submodules[@]} -gt 0 ]; then
        log_warning "Failed submodules (may need manual initialization):"
        for submodule in "${failed_submodules[@]}"; do
            echo "  - $submodule" | tee -a "$LOG_FILE"
        done
    fi

    return 0
}

fix_git_submodule_corruption() {
    log_section "Checking for Git Submodule Corruption"

    cd "$SCRIPT_DIR"

    # Check if there's a mismatch between .gitmodules and actual submodules
    log_action "Checking for submodule mismatches..."

    if git submodule status 2>&1 | grep -q "^-"; then
        log_info "Found uninitialized submodules (normal for fresh clone)"
    fi

    # Check for problematic submodule: dependencies/hl2sdk/hl2sdk-css
    if git submodule status 2>&1 | grep -q "dependencies/hl2sdk/hl2sdk-css"; then
        log_error "Found corrupted submodule reference: dependencies/hl2sdk/hl2sdk-css"
        log_warning "This path exists in git internals but not in .gitmodules"

        if confirm "Attempt to clean up corrupted submodule references?"; then
            log_action "Removing corrupted submodule from git cache..."
            git rm --cached dependencies/hl2sdk/hl2sdk-css 2>&1 | tee -a "$LOG_FILE" || true
            log_success "Corrupted reference removed"
        fi
    fi

    return 0
}

clean_build_artifacts() {
    log_section "Cleaning Build Artifacts"

    cd "$SCRIPT_DIR"

    local dirs_to_clean=("build/debug" "build/release")

    for dir in "${dirs_to_clean[@]}"; do
        if [ -d "$dir" ]; then
            log_action "Cleaning $dir..."
            rm -rf "$dir"
            log_success "$dir cleaned"
        fi
    done

    log_info "Build artifacts cleaned (forcing fresh configure)"
    return 0
}

verify_environment() {
    log_section "Verifying Build Environment"

    local checks_passed=0
    local checks_failed=0

    # Check 1: hl2sdk-manifests
    log_action "Checking hl2sdk-manifests..."
    if [ -f "${SCRIPT_DIR}/hl2sdk-manifests/SdkHelpers.ambuild" ]; then
        log_success "hl2sdk-manifests: OK"
        checks_passed=$((checks_passed + 1))
    else
        log_error "hl2sdk-manifests: MISSING"
        checks_failed=$((checks_failed + 1))
    fi

    # Check 2: linux-libc-dev:i386
    log_action "Checking linux-libc-dev:i386..."
    if dpkg -l | grep -q "linux-libc-dev:i386"; then
        log_success "linux-libc-dev:i386: INSTALLED"
        checks_passed=$((checks_passed + 1))
    else
        log_warning "linux-libc-dev:i386: NOT INSTALLED (may cause issues)"
        checks_failed=$((checks_failed + 1))
    fi

    # Check 3: Python
    log_action "Checking Python..."
    if command -v python3 >/dev/null 2>&1; then
        local python_version=$(python3 --version 2>&1)
        log_success "Python: $python_version"
        checks_passed=$((checks_passed + 1))
    else
        log_error "Python: NOT FOUND"
        checks_failed=$((checks_failed + 1))
    fi

    # Check 4: AMBuild
    log_action "Checking AMBuild..."
    if python3 -c "import ambuild2" 2>/dev/null; then
        log_success "AMBuild: INSTALLED"
        checks_passed=$((checks_passed + 1))
    else
        log_warning "AMBuild: NOT FOUND"
        checks_failed=$((checks_failed + 1))
    fi

    # Check 5: Build tools
    log_action "Checking build tools..."
    if command -v gcc >/dev/null 2>&1 && command -v g++ >/dev/null 2>&1; then
        local gcc_version=$(gcc --version | head -n1)
        log_success "GCC: $gcc_version"
        checks_passed=$((checks_passed + 1))
    else
        log_error "GCC/G++: NOT FOUND"
        checks_failed=$((checks_failed + 1))
    fi

    # Check 6: 32-bit support
    log_action "Checking 32-bit compiler support..."
    if gcc -m32 -x c /dev/null -o /dev/null 2>/dev/null; then
        log_success "32-bit compilation: SUPPORTED"
        checks_passed=$((checks_passed + 1))
    else
        log_warning "32-bit compilation: NOT SUPPORTED"
        checks_failed=$((checks_failed + 1))
    fi

    echo ""
    log_info "Environment Check: $checks_passed passed, $checks_failed failed"

    if [ $checks_failed -eq 0 ]; then
        log_success "Environment is ready for building!"
        return 0
    else
        log_warning "Environment has some issues but may still work"
        return 1
    fi
}

################################################################################
# Main Execution
################################################################################

main() {
    echo -e "${BOLD}${CYAN}"
    cat << "EOF"
╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║         RCBot2 Build Environment Fix Script                  ║
║                                                               ║
║  This script will fix environment mismatches between your     ║
║  local machine and the working build environment.            ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝
EOF
    echo -e "${NC}"

    # Initialize log
    echo "=== RCBot2 Build Environment Fix ===" > "$LOG_FILE"
    echo "Date: $(date)" >> "$LOG_FILE"
    echo "Script: $0 $@" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"

    log_info "Log file: $LOG_FILE"
    echo ""

    # Run fixes in order
    local exit_code=0

    # Fix 1: Git submodule corruption
    if ! fix_git_submodule_corruption; then
        log_warning "Submodule corruption check had issues"
    fi

    # Fix 2: Git submodules (critical)
    if ! fix_git_submodules; then
        log_error "Failed to fix git submodules"
        exit_code=1
    fi

    # Fix 3: Manifest structure
    if ! fix_hl2sdk_manifest_structure; then
        log_error "Failed to fix manifest structure"
        exit_code=1
    fi

    # Fix 4: 32-bit dependencies
    if ! fix_32bit_dependencies; then
        log_warning "32-bit dependency installation had issues"
    fi

    # Fix 5: AlliedModders submodules (optional)
    if confirm "Initialize AlliedModders submodules (metamod, hl2sdk)?"; then
        fix_alliedmodders_submodules
    else
        log_info "Skipping AlliedModders submodule initialization"
    fi

    # Fix 6: Clean build artifacts
    if confirm "Clean build artifacts to force fresh configure?"; then
        clean_build_artifacts
    else
        log_info "Skipping build artifact cleanup"
    fi

    # Verify environment
    echo ""
    verify_environment

    # Final summary
    log_section "Summary"

    if [ $exit_code -eq 0 ]; then
        log_success "Environment fixes completed successfully!"
        echo ""
        log_info "Next steps:"
        echo "  1. Run: ./build-linux.sh --skip-deps"
        echo "  2. Or run: python3 configure.py --enable-optimize"
        echo "  3. Check logs if build fails: build-logs/"
    else
        log_error "Some fixes failed - check the log file"
        echo ""
        log_info "Manual steps you can try:"
        echo "  1. git submodule sync"
        echo "  2. git submodule update --init --recursive hl2sdk-manifests"
        echo "  3. bash download-manifests.sh"
        echo "  4. sudo apt-get install linux-libc-dev:i386"
    fi

    echo ""
    log_info "Full log saved to: $LOG_FILE"

    exit $exit_code
}

# Run main function
main "$@"
