#!/bin/bash
################################################################################
# RCBot2 AI-Enhanced - Linux Build Script for Ubuntu Server 24.x
################################################################################
# This script automates the complete build process for RCBot2 on Linux:
# 1. Installs all required dependencies
# 2. Downloads and configures Source SDK and Metamod:Source
# 3. Sets up ONNX Runtime for ML features
# 4. Builds both Debug and Release configurations
# 5. Generates comprehensive build logs for error analysis
#
# Usage:
#   ./build-linux.sh                    # Interactive mode
#   ./build-linux.sh --auto             # Auto-install dependencies
#   ./build-linux.sh --dev-only         # Build debug only
#   ./build-linux.sh --release-only     # Build release only
#   ./build-linux.sh --skip-deps        # Skip dependency installation
#
# Requirements:
#   - Ubuntu Server 24.x (or compatible)
#   - sudo privileges
#   - Internet connection
################################################################################

set -e  # Exit on error
set -o pipefail  # Catch errors in pipes

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_LOGS_DIR="${SCRIPT_DIR}/build-logs"
DEPS_DIR="${SCRIPT_DIR}/dependencies"
TIMESTAMP=$(date +%Y%m%d-%H%M%S)

# Parse command line arguments
AUTO_INSTALL=false
DEV_ONLY=false
RELEASE_ONLY=false
SKIP_DEPS=false
VERBOSE=false
TARGET_ARCHS=""

for arg in "$@"; do
    case $arg in
        --auto) AUTO_INSTALL=true ;;
        --dev-only) DEV_ONLY=true ;;
        --release-only) RELEASE_ONLY=true ;;
        --skip-deps) SKIP_DEPS=true ;;
        --verbose) VERBOSE=true ;;
        --targets=*) TARGET_ARCHS="${arg#*=}" ;;
        --x86-only) TARGET_ARCHS="x86" ;;
        --x64-only) TARGET_ARCHS="x86_64" ;;
        --help)
            echo "RCBot2 Linux Build Script"
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --auto          Auto-install dependencies without prompts"
            echo "  --dev-only      Build debug configuration only"
            echo "  --release-only  Build release configuration only"
            echo "  --skip-deps     Skip dependency installation"
            echo "  --verbose       Show verbose output"
            echo "  --targets=ARCH  Specify target architectures (x86, x86_64, or x86,x86_64)"
            echo "  --x86-only      Build 32-bit binaries only (for 32-bit game servers)"
            echo "  --x64-only      Build 64-bit binaries only"
            echo "  --help          Show this help message"
            echo ""
            echo "Target Architecture Notes:"
            echo "  Most Source engine game servers (HL2DM, TF2, CSS, etc.) are 32-bit."
            echo "  Use --x86-only for these servers. The plugin will fail to load with"
            echo "  'wrong ELF class: ELFCLASS64' if you use 64-bit binaries on a 32-bit server."
            exit 0
            ;;
    esac
done

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Status tracking
declare -A DEPENDENCY_STATUS
FAILED_CHECKS=()
BUILD_SUCCESS=false
BUILD_START_TIME=$(date +%s)

################################################################################
# Utility Functions
################################################################################

log_info() {
    echo -e "${CYAN}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_action() {
    echo -e "${MAGENTA}[ACTION]${NC} $1"
}

log_section() {
    echo ""
    echo -e "${BOLD}${BLUE}================================================================================${NC}"
    echo -e "${BOLD}${BLUE}  $1${NC}"
    echo -e "${BOLD}${BLUE}================================================================================${NC}"
    echo ""
}

confirm() {
    if [ "$AUTO_INSTALL" = true ]; then
        return 0
    fi

    # Check if running in non-interactive mode (no TTY)
    if [ ! -t 0 ]; then
        local default="${2:-n}"
        log_info "Non-interactive mode detected, using default: $default"
        case "$default" in
            [yY]) return 0 ;;
            *) return 1 ;;
        esac
    fi

    local prompt="$1"
    local default="${2:-n}"

    if [ "$default" = "y" ]; then
        prompt="$prompt [Y/n]: "
    else
        prompt="$prompt [y/N]: "
    fi

    read -p "$prompt" response
    response=${response:-$default}

    case "$response" in
        [yY][eE][sS]|[yY]) return 0 ;;
        *) return 1 ;;
    esac
}

check_root() {
    if [ "$EUID" -eq 0 ]; then
        log_warning "Running as root. Files will be owned by root."

        # Get the actual user if running via sudo
        if [ -n "$SUDO_USER" ]; then
            ACTUAL_USER="$SUDO_USER"
            ACTUAL_UID=$(id -u "$SUDO_USER")
            ACTUAL_GID=$(id -g "$SUDO_USER")
            log_info "Will set ownership to user: $ACTUAL_USER (${ACTUAL_UID}:${ACTUAL_GID})"
        else
            ACTUAL_USER=""
            ACTUAL_UID=""
            ACTUAL_GID=""
            log_warning "Running as root directly (not via sudo). Files will be owned by root."
        fi
    else
        ACTUAL_USER="$USER"
        ACTUAL_UID=$(id -u)
        ACTUAL_GID=$(id -g)
    fi
}

create_directories() {
    log_action "Creating build directories..."

    # Create directories with proper error handling
    if ! mkdir -p "$BUILD_LOGS_DIR" 2>/dev/null; then
        log_error "Failed to create build-logs directory: $BUILD_LOGS_DIR"
        log_error "Try running with sudo: sudo ./build-linux.sh --auto"
        exit 1
    fi

    if ! mkdir -p "$DEPS_DIR" 2>/dev/null; then
        log_error "Failed to create dependencies directory: $DEPS_DIR"
        log_error "Try running with sudo: sudo ./build-linux.sh --auto"
        exit 1
    fi

    # Fix ownership if running as sudo
    if [ "$EUID" -eq 0 ] && [ -n "$ACTUAL_USER" ]; then
        chown -R "${ACTUAL_UID}:${ACTUAL_GID}" "$BUILD_LOGS_DIR" 2>/dev/null || true
        chown -R "${ACTUAL_UID}:${ACTUAL_GID}" "$DEPS_DIR" 2>/dev/null || true
        log_info "Set ownership to $ACTUAL_USER"
    fi

    log_success "Directories created"
}

################################################################################
# Dependency Installation and Verification
################################################################################

check_system() {
    log_section "System Information"

    log_info "Operating System: $(lsb_release -d | cut -f2)"
    log_info "Kernel: $(uname -r)"
    log_info "Architecture: $(uname -m)"

    # Check if Ubuntu 24.x
    if ! lsb_release -d | grep -q "Ubuntu 24"; then
        log_warning "This script is designed for Ubuntu 24.x. Your version may work but is untested."
        if ! confirm "Continue anyway?"; then
            exit 1
        fi
    fi

    log_success "System check complete"
}

install_base_dependencies() {
    log_section "Installing Base Dependencies"

    if [ "$SKIP_DEPS" = true ]; then
        log_warning "Skipping dependency installation (--skip-deps flag)"
        return 0
    fi

    log_action "Updating package lists..."

    # Use sudo only if not already root
    if [ "$EUID" -eq 0 ]; then
        apt-get update -qq || {
            log_error "Failed to update package lists"
            exit 1
        }
    else
        sudo apt-get update -qq || {
            log_error "Failed to update package lists"
            exit 1
        }
    fi

    local packages=(
        build-essential
        gcc-multilib
        g++-multilib
        git
        python3
        python3-pip
        python3-venv
        wget
        curl
        unzip
        lib32z1-dev
        libc6-dev-i386
        linux-libc-dev:i386
    )

    log_info "Installing required packages..."
    log_info "Packages: ${packages[*]}"

    for package in "${packages[@]}"; do
        if dpkg -l | grep -q "^ii  $package "; then
            log_success "$package: Already installed"
        else
            log_action "Installing $package..."
            if [ "$EUID" -eq 0 ]; then
                apt-get install -y -qq "$package" || {
                    log_error "Failed to install $package"
                    FAILED_CHECKS+=("$package")
                }
            else
                sudo apt-get install -y -qq "$package" || {
                    log_error "Failed to install $package"
                    FAILED_CHECKS+=("$package")
                }
            fi
        fi
    done

    DEPENDENCY_STATUS[base_packages]="installed"
    log_success "Base dependencies installed"
}

check_python() {
    log_section "Checking Python Installation"

    if command -v python3 &> /dev/null; then
        local python_version=$(python3 --version | awk '{print $2}')
        log_success "Python: $python_version"
        DEPENDENCY_STATUS[python]="installed:$python_version"

        # Check if version >= 3.8
        local major=$(echo "$python_version" | cut -d. -f1)
        local minor=$(echo "$python_version" | cut -d. -f2)

        if [ "$major" -ge 3 ] && [ "$minor" -ge 8 ]; then
            log_success "Python version is compatible (>= 3.8)"
        else
            log_error "Python version too old. Need >= 3.8"
            FAILED_CHECKS+=("python_version")
            return 1
        fi
    else
        log_error "Python 3 not found"
        FAILED_CHECKS+=("python")
        return 1
    fi
}

install_ambuild() {
    log_section "Installing AMBuild"

    if python3 -c "import ambuild2" 2>/dev/null; then
        local version=$(python3 -c "import ambuild2; print(ambuild2.__version__)" 2>/dev/null || echo "unknown")
        log_success "AMBuild already installed: $version"
        DEPENDENCY_STATUS[ambuild]="installed:$version"
        return 0
    fi

    log_action "Installing AMBuild from GitHub..."

    if [ "$AUTO_INSTALL" = true ] || confirm "Install AMBuild now?"; then
        # Install as root if running as root, otherwise use --user
        # Ubuntu 24.x requires --break-system-packages for system-wide install
        if [ "$EUID" -eq 0 ]; then
            log_info "Installing AMBuild system-wide (using --break-system-packages for Ubuntu 24.x)..."
            python3 -m pip install --break-system-packages git+https://github.com/alliedmodders/ambuild || {
                log_error "Failed to install AMBuild"
                log_info "Try manually: python3 -m pip install --break-system-packages git+https://github.com/alliedmodders/ambuild"
                FAILED_CHECKS+=("ambuild")
                return 1
            }
        else
            python3 -m pip install --user git+https://github.com/alliedmodders/ambuild || {
                log_error "Failed to install AMBuild"
                log_info "Try manually: python3 -m pip install --user git+https://github.com/alliedmodders/ambuild"
                FAILED_CHECKS+=("ambuild")
                return 1
            }
        fi

        log_success "AMBuild installed successfully"
        DEPENDENCY_STATUS[ambuild]="installed"

        # Verify installation
        if python3 -c "import ambuild2" 2>/dev/null; then
            log_success "AMBuild verified"
        else
            log_error "AMBuild installation verification failed"
            FAILED_CHECKS+=("ambuild_verify")
            return 1
        fi
    else
        log_warning "Skipped AMBuild installation"
        FAILED_CHECKS+=("ambuild")
        return 1
    fi
}

install_python_ml_packages() {
    log_section "Installing Python ML Packages (Optional)"

    local packages=(
        "numpy"
        "onnx"
        "onnxruntime"
        "torch"
        "scikit-learn"
    )

    local missing_packages=()

    for package in "${packages[@]}"; do
        if python3 -c "import $package" 2>/dev/null; then
            local version=$(python3 -c "import $package; print($package.__version__)" 2>/dev/null || echo "unknown")
            log_success "$package: $version"
        else
            log_warning "$package: Not installed"
            missing_packages+=("$package")
        fi
    done

    if [ ${#missing_packages[@]} -gt 0 ]; then
        log_info "Missing packages: ${missing_packages[*]}"

        if [ "$AUTO_INSTALL" = true ] || confirm "Install missing ML packages?"; then
            log_action "Installing ML packages..."
            if [ "$EUID" -eq 0 ]; then
                # Ubuntu 24.x requires --break-system-packages
                python3 -m pip install --break-system-packages "${missing_packages[@]}" || {
                    log_warning "Some ML packages failed to install (this is optional)"
                }
            else
                python3 -m pip install --user "${missing_packages[@]}" || {
                    log_warning "Some ML packages failed to install (this is optional)"
                }
            fi
        else
            log_info "Skipped ML package installation (only needed for training)"
        fi
    else
        log_success "All ML packages installed"
    fi
}

download_hl2sdk() {
    log_section "Downloading HL2SDK"

    # NOTE: This project uses git submodules for SDKs (in alliedmodders/ directory)
    # This function is for environments where submodules cannot be used

    # First check if git submodules are available
    local submodule_root="${SCRIPT_DIR}/alliedmodders"
    if [ -d "${submodule_root}/hl2sdk-hl2dm" ] || [ -d "${submodule_root}/hl2sdk-tf2" ]; then
        log_success "Using HL2SDK from git submodules: $submodule_root"
        log_info "Skipping separate SDK download (submodules take precedence)"
        DEPENDENCY_STATUS[hl2sdk]="submodule:$submodule_root"
        return 0
    fi

    local hl2sdk_root="${DEPS_DIR}/hl2sdk"
    local sdks=("hl2dm" "tf2" "css" "dods")

    if [ -d "$hl2sdk_root" ]; then
        log_info "HL2SDK directory exists: $hl2sdk_root"

        # Check if SDKs are already cloned
        local existing_count=0
        for sdk in "${sdks[@]}"; do
            if [ -d "${hl2sdk_root}/hl2sdk-${sdk}" ]; then
                existing_count=$((existing_count + 1))
            fi
        done

        if [ $existing_count -gt 0 ]; then
            log_success "Found $existing_count existing SDK(s)"

            if ! confirm "Re-download SDKs?" "n"; then
                log_info "Keeping existing SDKs"
                DEPENDENCY_STATUS[hl2sdk]="existing:$hl2sdk_root"
                return 0
            fi
        fi
    fi

    mkdir -p "$hl2sdk_root"

    log_info "Cloning HL2SDK repositories (this may take a while)..."

    for sdk in "${sdks[@]}"; do
        local sdk_dir="${hl2sdk_root}/hl2sdk-${sdk}"

        if [ -d "$sdk_dir" ]; then
            log_info "Updating hl2sdk-${sdk}..."
            (cd "$sdk_dir" && git pull) || log_warning "Failed to update hl2sdk-${sdk}"
        else
            log_action "Cloning hl2sdk-${sdk}..."
            git clone --depth 1 -b "$sdk" https://github.com/alliedmodders/hl2sdk.git "$sdk_dir" || {
                log_error "Failed to clone hl2sdk-${sdk}"
                FAILED_CHECKS+=("hl2sdk_${sdk}")
                continue
            }
        fi

        log_success "hl2sdk-${sdk} ready"
    done

    DEPENDENCY_STATUS[hl2sdk]="installed:$hl2sdk_root"
    log_success "HL2SDK installation complete"
}

download_metamod() {
    log_section "Downloading Metamod:Source"

    # NOTE: This project uses git submodules for Metamod (in alliedmodders/ directory)
    # This function is for environments where submodules cannot be used

    # First check if git submodule is available
    local submodule_mms="${SCRIPT_DIR}/alliedmodders/metamod-source"
    if [ -d "$submodule_mms" ]; then
        log_success "Using Metamod:Source from git submodule: $submodule_mms"
        log_info "Skipping separate download (submodule takes precedence)"
        DEPENDENCY_STATUS[metamod]="submodule:$submodule_mms"
        return 0
    fi

    local mms_dir="${DEPS_DIR}/metamod-source"

    if [ -d "$mms_dir" ]; then
        log_info "Metamod:Source directory exists: $mms_dir"

        if ! confirm "Re-download Metamod:Source?" "n"; then
            log_info "Keeping existing Metamod:Source"
            DEPENDENCY_STATUS[metamod]="existing:$mms_dir"
            return 0
        fi
    fi

    log_action "Cloning Metamod:Source..."

    if [ -d "$mms_dir" ]; then
        (cd "$mms_dir" && git pull) || log_warning "Failed to update Metamod:Source"
    else
        git clone https://github.com/alliedmodders/metamod-source.git "$mms_dir" || {
            log_error "Failed to clone Metamod:Source"
            FAILED_CHECKS+=("metamod")
            return 1
        }
    fi

    DEPENDENCY_STATUS[metamod]="installed:$mms_dir"
    log_success "Metamod:Source ready"
}

download_hl2sdk_manifests() {
    log_section "Checking HL2SDK Manifests"

    local manifests_dir="${SCRIPT_DIR}/hl2sdk-manifests"
    local sdkhelpers_file="${manifests_dir}/SdkHelpers.ambuild"

    # Check if manifests are already available
    if [ -f "$sdkhelpers_file" ]; then
        # Validate it's not an HTML error page
        if grep -q "404: Not Found\|<html>\|<HTML>" "$sdkhelpers_file" 2>/dev/null; then
            log_error "SdkHelpers.ambuild contains HTML error page (corrupted)"
            log_warning "Removing corrupted file and re-downloading..."
            rm -f "$sdkhelpers_file"
        else
            log_success "HL2SDK manifests already present: $manifests_dir"
            DEPENDENCY_STATUS[hl2sdk_manifests]="existing:$manifests_dir"
            return 0
        fi
    fi

    log_warning "HL2SDK manifests not found or corrupted"
    log_info "Manifests are required for SDK detection (SdkHelpers.ambuild)"

    # Try git submodule first
    if [ -d "${SCRIPT_DIR}/.git" ]; then
        log_action "Attempting to initialize git submodule..."

        if git submodule update --init hl2sdk-manifests 2>&1; then
            if [ -f "$sdkhelpers_file" ]; then
                log_success "Manifests initialized via git submodule"
                DEPENDENCY_STATUS[hl2sdk_manifests]="submodule:$manifests_dir"
                return 0
            fi
        else
            log_warning "Git submodule initialization failed"
        fi
    fi

    # Fallback to download script
    log_info "Using download script as fallback..."

    if [ -f "${SCRIPT_DIR}/download-manifests.sh" ]; then
        if "${SCRIPT_DIR}/download-manifests.sh"; then
            log_success "Manifests downloaded successfully"
            DEPENDENCY_STATUS[hl2sdk_manifests]="downloaded:$manifests_dir"
            return 0
        else
            log_error "Failed to download manifests"
            FAILED_CHECKS+=("hl2sdk_manifests")
            return 1
        fi
    else
        log_error "download-manifests.sh script not found"
        log_error "Please run: git submodule update --init hl2sdk-manifests"
        FAILED_CHECKS+=("hl2sdk_manifests")
        return 1
    fi
}

download_onnx_runtime() {
    log_section "Downloading ONNX Runtime (for ML Features)"

    local onnx_dir="${DEPS_DIR}/onnxruntime"

    if [ -d "$onnx_dir" ] && [ -f "$onnx_dir/lib/libonnxruntime.so" ]; then
        log_success "ONNX Runtime already exists: $onnx_dir"

        if ! confirm "Re-download ONNX Runtime?"; then
            DEPENDENCY_STATUS[onnxruntime]="existing:$onnx_dir"
            export ONNXRUNTIME_DIR="$onnx_dir"
            return 0
        fi
    fi

    log_info "Detecting architecture..."
    local arch=$(uname -m)
    local onnx_version="1.16.3"
    local onnx_url=""
    local onnx_file=""

    if [ "$arch" = "x86_64" ]; then
        onnx_file="onnxruntime-linux-x64-${onnx_version}.tgz"
        onnx_url="https://github.com/microsoft/onnxruntime/releases/download/v${onnx_version}/${onnx_file}"
    elif [ "$arch" = "aarch64" ]; then
        onnx_file="onnxruntime-linux-aarch64-${onnx_version}.tgz"
        onnx_url="https://github.com/microsoft/onnxruntime/releases/download/v${onnx_version}/${onnx_file}"
    else
        log_error "Unsupported architecture: $arch"
        log_warning "ML features will be disabled"
        DEPENDENCY_STATUS[onnxruntime]="unsupported"
        return 1
    fi

    log_action "Downloading ONNX Runtime ${onnx_version} for ${arch}..."
    log_info "URL: $onnx_url"

    mkdir -p "$onnx_dir"
    cd "${DEPS_DIR}"

    wget -q --show-progress "$onnx_url" || {
        log_error "Failed to download ONNX Runtime"
        log_warning "ML features will be disabled"
        DEPENDENCY_STATUS[onnxruntime]="download_failed"
        return 1
    }

    log_action "Extracting ONNX Runtime..."
    tar -xzf "$onnx_file" || {
        log_error "Failed to extract ONNX Runtime"
        return 1
    }

    # Move extracted contents to onnxruntime dir
    local extracted_dir="${onnx_file%.tgz}"
    if [ -d "$extracted_dir" ]; then
        rm -rf "$onnx_dir"
        mv "$extracted_dir" "$onnx_dir"
    fi

    rm -f "$onnx_file"
    cd "$SCRIPT_DIR"

    # Verify installation
    if [ -f "$onnx_dir/lib/libonnxruntime.so" ]; then
        log_success "ONNX Runtime installed: $onnx_dir"
        export ONNXRUNTIME_DIR="$onnx_dir"
        DEPENDENCY_STATUS[onnxruntime]="installed:$onnx_dir"
    else
        log_error "ONNX Runtime installation verification failed"
        FAILED_CHECKS+=("onnxruntime_verify")
        return 1
    fi
}

verify_dependencies() {
    log_section "Dependency Verification Summary"

    local all_passed=true

    echo ""
    echo "Status Overview:"
    echo "================"

    for dep in "${!DEPENDENCY_STATUS[@]}"; do
        local status="${DEPENDENCY_STATUS[$dep]}"

        if [[ "$status" == installed* ]] || [[ "$status" == existing* ]]; then
            echo -e "${GREEN}[✓]${NC} $dep: $status"
        else
            echo -e "${RED}[✗]${NC} $dep: $status"
            all_passed=false
        fi
    done

    echo ""

    if [ ${#FAILED_CHECKS[@]} -gt 0 ]; then
        log_warning "Some dependency checks failed:"
        for failed in "${FAILED_CHECKS[@]}"; do
            echo "  - $failed"
        done
        echo ""

        if ! confirm "Continue with build anyway?" "n"; then
            log_error "Build cancelled by user"
            exit 1
        fi
    else
        log_success "All critical dependencies verified!"
    fi

    # Save dependency report
    local report_file="${BUILD_LOGS_DIR}/dependency-report-${TIMESTAMP}.txt"
    {
        echo "RCBot2 Dependency Report"
        echo "========================"
        echo "Date: $(date)"
        echo "System: $(lsb_release -d | cut -f2)"
        echo ""
        echo "Dependencies:"
        for dep in "${!DEPENDENCY_STATUS[@]}"; do
            echo "  $dep: ${DEPENDENCY_STATUS[$dep]}"
        done
        echo ""
        echo "Failed Checks:"
        for failed in "${FAILED_CHECKS[@]}"; do
            echo "  - $failed"
        done
    } > "$report_file"

    log_info "Dependency report saved: $report_file"
}

################################################################################
# Build Configuration
################################################################################

configure_build() {
    local config="$1"  # debug or release
    local build_dir="${SCRIPT_DIR}/build-${config}"

    log_section "Configuring ${config} Build"

    # Use git submodules paths instead of separate dependencies directory
    local hl2sdk_root="${SCRIPT_DIR}/alliedmodders"
    local mms_path="${SCRIPT_DIR}/alliedmodders/metamod-source"
    local sm_path="${SCRIPT_DIR}/alliedmodders/sourcemod"

    # Check if paths exist
    if [ ! -d "$hl2sdk_root" ]; then
        log_error "AlliedModders submodules directory not found: $hl2sdk_root"
        log_error "Run: git submodule update --init --recursive"
        return 1
    fi

    if [ ! -d "$mms_path" ]; then
        log_error "Metamod:Source not found: $mms_path"
        log_error "Run: git submodule update --init alliedmodders/metamod-source"
        return 1
    fi

    if [ ! -d "$sm_path" ]; then
        log_error "SourceMod not found: $sm_path"
        log_error "Run: git submodule update --init alliedmodders/sourcemod"
        return 1
    fi

    # Create build directory
    mkdir -p "$build_dir"
    cd "$build_dir"

    log_info "Build directory: $build_dir"
    log_info "HL2SDK root: $hl2sdk_root"
    log_info "Metamod:Source: $mms_path"
    log_info "SourceMod: $sm_path"

    # Build configure command
    local config_args=(
        "${SCRIPT_DIR}/configure.py"
        "--hl2sdk-root=${hl2sdk_root}"
        "--mms-path=${mms_path}"
        "--sm-path=${sm_path}"
        "--sdks=hl2dm,tf2"
    )

    if [ "$config" = "debug" ]; then
        config_args+=("--enable-debug")
        log_info "Debug configuration enabled"
    else
        config_args+=("--enable-optimize")
        log_info "Optimize configuration enabled"
    fi

    # Add target architecture if specified
    if [ -n "$TARGET_ARCHS" ]; then
        config_args+=("--targets=${TARGET_ARCHS}")
        log_info "Target architectures: $TARGET_ARCHS"
    else
        # Default to x86 for most game servers (HL2DM, TF2, CSS are 32-bit)
        config_args+=("--targets=x86")
        log_info "Target architectures: x86 (default - most game servers are 32-bit)"
        log_warning "Use --x86-only or --x64-only to change target. See --help for details."
    fi

    # Add ONNX support if available
    if [ -n "$ONNXRUNTIME_DIR" ] && [ -d "$ONNXRUNTIME_DIR" ]; then
        log_success "ONNX Runtime detected: $ONNXRUNTIME_DIR"
        log_success "ML features will be enabled (RCBOT_WITH_ONNX)"
        export ONNXRUNTIME_DIR
    else
        log_warning "ONNX Runtime not available - ML features will be disabled"
    fi

    log_info "Configuration command: python3 ${config_args[*]}"

    # Run configuration
    local config_log="${BUILD_LOGS_DIR}/configure-${config}-${TIMESTAMP}.log"
    log_action "Running AMBuild configuration..."

    python3 "${config_args[@]}" 2>&1 | tee "$config_log"
    local config_result=${PIPESTATUS[0]}

    if [ $config_result -ne 0 ]; then
        log_error "Configuration failed!"
        log_error "See log: $config_log"
        cd "$SCRIPT_DIR"
        return 1
    fi

    log_success "Configuration successful"
    cd "$SCRIPT_DIR"
    return 0
}

build_project() {
    local config="$1"  # debug or release
    local build_dir="${SCRIPT_DIR}/build-${config}"

    log_section "Building ${config} Configuration"

    if [ ! -d "$build_dir" ]; then
        log_error "Build directory not found: $build_dir"
        log_error "Run configuration first"
        return 1
    fi

    cd "$build_dir"

    # Set up log files
    local build_log="${BUILD_LOGS_DIR}/build-${config}-${TIMESTAMP}.log"
    local error_log="${BUILD_LOGS_DIR}/errors-${config}-${TIMESTAMP}.log"
    local summary_log="${BUILD_LOGS_DIR}/summary-${config}-${TIMESTAMP}.txt"

    # Fix ownership of build directory if running as sudo
    if [ "$EUID" -eq 0 ] && [ -n "$ACTUAL_USER" ]; then
        chown -R "${ACTUAL_UID}:${ACTUAL_GID}" "$build_dir" 2>/dev/null || true
    fi

    log_info "Build log: $build_log"
    log_info "Error log: $error_log"

    # Run build
    local build_start=$(date +%s)
    log_action "Starting build..."

    # Run AMBuild using the correct modern API
    # Modern AMBuild2 uses the 'ambuild' command directly from the build directory
    # The build directory must contain the .ambuild2 folder created during configuration
    if [ ! -d ".ambuild2" ]; then
        log_error "Build not configured! Missing .ambuild2 directory"
        log_error "Run configuration first: configure_build $config"
        cd "$SCRIPT_DIR"
        return 1
    fi

    # Check if ambuild command is available
    if ! command -v ambuild &> /dev/null; then
        log_error "ambuild command not found!"
        log_error "Ensure AMBuild is installed and in PATH"
        log_error "Try: python3 -m pip install --user git+https://github.com/alliedmodders/ambuild"
        cd "$SCRIPT_DIR"
        return 1
    fi

    log_info "Running: ambuild"
    ambuild 2>&1 | tee "$build_log"
    local build_result=${PIPESTATUS[0]}
    local build_end=$(date +%s)
    local build_duration=$((build_end - build_start))

    # Extract errors to separate file
    grep -i "error" "$build_log" > "$error_log" 2>/dev/null || true

    # Generate summary
    {
        echo "================================================================================
"
        echo "BUILD SUMMARY - ${config}"
        echo "================================================================================"
        echo "Configuration: ${config}"
        echo "Result: $([ $build_result -eq 0 ] && echo 'SUCCESS' || echo 'FAILED')"
        echo "Duration: ${build_duration}s ($(date -d@${build_duration} -u +%M:%S))"
        echo "Timestamp: $(date)"
        echo "================================================================================"
        echo ""

        if [ $build_result -ne 0 ]; then
            echo "BUILD FAILED"
            echo "============"
            echo ""
            echo "Error log: $error_log"
            echo "Full build log: $build_log"
            echo ""
            echo "Common issues:"
            echo "  1. Missing dependencies - run with --verbose to see details"
            echo "  2. SDK path issues - verify HL2SDK and Metamod paths"
            echo "  3. Architecture mismatch - ensure 32-bit libraries are installed"
            echo ""
            echo "To analyze with Claude:"
            echo "  1. Upload: $error_log"
            echo "  2. Upload: $summary_log (this file)"
            echo "  3. Prompt: 'Analyze these build errors and suggest fixes'"
        else
            echo "BUILD SUCCESSFUL"
            echo "==============="
            echo ""
            echo "Built binaries:"
            find . -name "*.so" -type f | while read -r file; do
                echo "  $(realpath "$file")"
            done
            echo ""
            echo "Installation:"
            echo "  1. Copy .so files to your game server's addons/rcbot2/bin/"
            echo "  2. If using ML features, copy ONNX Runtime libraries"
            echo "  3. See docs/ONNX_SETUP.md for configuration"
        fi
        echo ""
        echo "Log files:"
        echo "  Build: $build_log"
        echo "  Errors: $error_log"
        echo "  Summary: $summary_log"
        echo ""
    } > "$summary_log"

    cat "$summary_log"

    # Fix ownership of all created files if running as sudo
    if [ "$EUID" -eq 0 ] && [ -n "$ACTUAL_USER" ]; then
        chown -R "${ACTUAL_UID}:${ACTUAL_GID}" "$BUILD_LOGS_DIR" 2>/dev/null || true
        chown -R "${ACTUAL_UID}:${ACTUAL_GID}" "$build_dir" 2>/dev/null || true
        log_info "Set ownership of build artifacts to $ACTUAL_USER"
    fi

    cd "$SCRIPT_DIR"

    return $build_result
}

################################################################################
# Main Execution
################################################################################

main() {
    log_section "RCBot2 AI-Enhanced Linux Build Script"

    log_info "Started: $(date)"
    log_info "Script directory: $SCRIPT_DIR"

    # Pre-flight checks
    check_root
    create_directories
    check_system

    # Install and verify dependencies
    if [ "$SKIP_DEPS" != true ]; then
        install_base_dependencies
        check_python
        install_ambuild
        install_python_ml_packages
        download_hl2sdk_manifests  # Must be before SDK detection
        download_hl2sdk
        download_metamod
        download_onnx_runtime
    else
        log_warning "Skipping dependency installation (--skip-deps)"
        # Even when skipping deps, we need manifests for build to work
        download_hl2sdk_manifests
    fi

    verify_dependencies

    # Build configurations
    local build_configs=()

    if [ "$DEV_ONLY" = true ]; then
        build_configs=("debug")
        log_info "Building debug only (--dev-only)"
    elif [ "$RELEASE_ONLY" = true ]; then
        build_configs=("release")
        log_info "Building release only (--release-only)"
    else
        build_configs=("debug" "release")
        log_info "Building both debug and release"
    fi

    local all_builds_passed=true

    for config in "${build_configs[@]}"; do
        if configure_build "$config"; then
            if build_project "$config"; then
                log_success "${config} build completed successfully!"
            else
                log_error "${config} build failed!"
                all_builds_passed=false
            fi
        else
            log_error "${config} configuration failed!"
            all_builds_passed=false
        fi
    done

    # Final summary
    log_section "Build Complete"

    local end_time=$(date +%s)
    local total_duration=$((end_time - BUILD_START_TIME))

    log_info "Total time: ${total_duration}s ($(date -d@${total_duration} -u +%H:%M:%S))"

    if [ "$all_builds_passed" = true ]; then
        log_success "All builds completed successfully!"
        echo ""
        echo "Next steps:"
        echo "  1. Check build output in build-debug/ and build-release/"
        echo "  2. Copy .so files to your game server"
        echo "  3. Configure bot settings (see docs/)"
        echo ""
        exit 0
    else
        log_error "Some builds failed!"
        echo ""
        echo "Troubleshooting:"
        echo "  1. Check build logs in ${BUILD_LOGS_DIR}/"
        echo "  2. Upload error logs to Claude for analysis"
        echo "  3. Review summary files for common issues"
        echo ""
        exit 1
    fi
}

# Run main function
main "$@"
