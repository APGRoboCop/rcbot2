<#
.SYNOPSIS
    RCBot2 AI-Enhanced - Dependency Verification and Installation Script

.DESCRIPTION
    This interactive PowerShell script verifies and helps install all dependencies
    required for both development building and production running of RCBot2.

    Requirements:
    - Development: Visual Studio, Source SDK, ONNX Runtime, Python, AMBuild
    - Production: ONNX Runtime libraries, Source Mod

.NOTES
    Author: RCBot2 AI-Enhanced Team
    Version: 1.0
    Requires: PowerShell 5.1 or later, Administrator privileges for some operations
#>

#Requires -Version 5.1

param(
    [switch]$AutoInstall,
    [switch]$SkipOptional,
    [switch]$Verbose
)

# Script configuration
$ErrorActionPreference = "Stop"
$ProgressPreference = "Continue"

# Color output helpers
function Write-Status {
    param([string]$Message, [string]$Type = "Info")
    $colors = @{
        "Info" = "Cyan"
        "Success" = "Green"
        "Warning" = "Yellow"
        "Error" = "Red"
        "Action" = "Magenta"
    }
    Write-Host "[$Type] " -ForegroundColor $colors[$Type] -NoNewline
    Write-Host $Message
}

function Write-Section {
    param([string]$Title)
    Write-Host "`n$('=' * 80)" -ForegroundColor DarkGray
    Write-Host "  $Title" -ForegroundColor White
    Write-Host "$('=' * 80)" -ForegroundColor DarkGray
}

# Dependency check results
$global:DependencyStatus = @{}
$global:ManualInstalls = @()
$global:FailedChecks = @()

# Initialize check
function Initialize-Check {
    Write-Host "`n" -NoNewline
    Write-Host "RCBot2 AI-Enhanced - Dependency Verification" -ForegroundColor Cyan
    Write-Host "=============================================" -ForegroundColor Cyan
    Write-Host ""

    # Check if running as admin
    $isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    if (-not $isAdmin) {
        Write-Status "Not running as Administrator. Some installations may fail." "Warning"
        Write-Status "Consider running: Start-Process powershell -Verb RunAs -ArgumentList '-File `"$PSCommandPath`"'" "Action"
        Write-Host ""

        $response = Read-Host "Continue anyway? (y/N)"
        if ($response -ne 'y') {
            exit 1
        }
    }
}

# Check Visual Studio / Build Tools
function Test-VisualStudio {
    Write-Section "Checking Visual Studio / MSVC Build Tools"

    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

    if (Test-Path $vsWhere) {
        $vsInstalls = & $vsWhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath

        if ($vsInstalls) {
            Write-Status "Visual Studio found: $($vsInstalls[0])" "Success"
            $global:DependencyStatus["VisualStudio"] = @{
                Status = "Installed"
                Path = $vsInstalls[0]
            }

            # Check for specific components
            $components = & $vsWhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json | ConvertFrom-Json
            if ($components) {
                Write-Status "C++ Build Tools: Installed" "Success"
            }

            return $true
        }
    }

    Write-Status "Visual Studio or Build Tools not found" "Error"
    $global:DependencyStatus["VisualStudio"] = @{ Status = "Missing" }
    $global:FailedChecks += "VisualStudio"

    # Offer installation help
    Write-Host ""
    Write-Host "MANUAL INSTALLATION REQUIRED:" -ForegroundColor Yellow
    Write-Host "1. Download Visual Studio Community (free):" -ForegroundColor White
    Write-Host "   https://visualstudio.microsoft.com/downloads/" -ForegroundColor Cyan
    Write-Host "2. During installation, select:" -ForegroundColor White
    Write-Host "   - Desktop development with C++" -ForegroundColor White
    Write-Host "   - C++ CMake tools for Windows" -ForegroundColor White
    Write-Host "   - Windows 10/11 SDK" -ForegroundColor White

    $global:ManualInstalls += @{
        Name = "Visual Studio Build Tools"
        URL = "https://visualstudio.microsoft.com/downloads/"
        Instructions = "Install with 'Desktop development with C++' workload"
    }

    return $false
}

# Check Python
function Test-Python {
    Write-Section "Checking Python Installation"

    try {
        $pythonVersion = python --version 2>&1
        if ($pythonVersion -match "Python (\d+\.\d+\.\d+)") {
            $version = $Matches[1]
            Write-Status "Python found: $version" "Success"

            # Check if version is >= 3.8
            $versionObj = [version]$version
            if ($versionObj -ge [version]"3.8.0") {
                Write-Status "Python version is compatible (>= 3.8)" "Success"
                $global:DependencyStatus["Python"] = @{
                    Status = "Installed"
                    Version = $version
                }

                # Check pip
                try {
                    $pipVersion = pip --version 2>&1
                    Write-Status "pip: Installed" "Success"
                } catch {
                    Write-Status "pip: Not found" "Warning"
                }

                return $true
            } else {
                Write-Status "Python version too old (need >= 3.8)" "Error"
                $global:FailedChecks += "Python"
            }
        }
    } catch {
        Write-Status "Python not found in PATH" "Error"
        $global:DependencyStatus["Python"] = @{ Status = "Missing" }
        $global:FailedChecks += "Python"
    }

    Write-Host ""
    Write-Host "MANUAL INSTALLATION REQUIRED:" -ForegroundColor Yellow
    Write-Host "1. Download Python 3.8+ from:" -ForegroundColor White
    Write-Host "   https://www.python.org/downloads/" -ForegroundColor Cyan
    Write-Host "2. During installation:" -ForegroundColor White
    Write-Host "   - Check 'Add Python to PATH'" -ForegroundColor White
    Write-Host "   - Install pip (default option)" -ForegroundColor White

    $global:ManualInstalls += @{
        Name = "Python 3.8+"
        URL = "https://www.python.org/downloads/"
        Instructions = "Install with 'Add Python to PATH' checked"
    }

    return $false
}

# Check Python packages for ML training
function Test-PythonPackages {
    Write-Section "Checking Python ML Packages"

    if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
        Write-Status "Skipping (Python not installed)" "Warning"
        return $false
    }

    $requiredPackages = @(
        "numpy",
        "onnx",
        "onnxruntime",
        "torch",
        "scikit-learn"
    )

    $missingPackages = @()

    foreach ($package in $requiredPackages) {
        try {
            $result = python -c "import $package; print($package.__version__)" 2>&1
            if ($LASTEXITCODE -eq 0) {
                Write-Status "$package : Installed ($result)" "Success"
            } else {
                throw
            }
        } catch {
            Write-Status "$package : Not installed" "Warning"
            $missingPackages += $package
        }
    }

    if ($missingPackages.Count -gt 0) {
        Write-Host ""
        Write-Host "Missing packages can be installed with:" -ForegroundColor Yellow
        Write-Host "  pip install $($missingPackages -join ' ')" -ForegroundColor Cyan

        if ($AutoInstall) {
            Write-Status "Attempting to install missing packages..." "Action"
            try {
                pip install $missingPackages
                Write-Status "Packages installed successfully" "Success"
                return $true
            } catch {
                Write-Status "Failed to install packages: $_" "Error"
                return $false
            }
        } else {
            $response = Read-Host "Install missing packages now? (y/N)"
            if ($response -eq 'y') {
                pip install $missingPackages
            }
        }
    } else {
        Write-Status "All required Python packages installed" "Success"
        return $true
    }
}

# Check ONNX Runtime
function Test-ONNXRuntime {
    Write-Section "Checking ONNX Runtime C++ Libraries"

    # Common installation paths
    $possiblePaths = @(
        "$env:ProgramFiles\onnxruntime",
        "$env:ProgramFiles(x86)\onnxruntime",
        "C:\onnxruntime",
        "$env:USERPROFILE\onnxruntime",
        ".\dependencies\onnxruntime"
    )

    # Check environment variable
    if ($env:ONNXRUNTIME_DIR) {
        $possiblePaths = @($env:ONNXRUNTIME_DIR) + $possiblePaths
    }

    foreach ($path in $possiblePaths) {
        if (Test-Path $path) {
            # Check for required files
            $headerPath = Join-Path $path "include\onnxruntime\core\session\onnxruntime_cxx_api.h"
            $libPath = Join-Path $path "lib\onnxruntime.lib"
            $dllPath = Join-Path $path "lib\onnxruntime.dll"

            if ((Test-Path $headerPath) -and (Test-Path $libPath)) {
                Write-Status "ONNX Runtime found: $path" "Success"
                Write-Status "Headers: $headerPath" "Success"
                Write-Status "Library: $libPath" "Success"
                if (Test-Path $dllPath) {
                    Write-Status "DLL: $dllPath" "Success"
                }

                $global:DependencyStatus["ONNXRuntime"] = @{
                    Status = "Installed"
                    Path = $path
                }

                # Set environment variable if not set
                if (-not $env:ONNXRUNTIME_DIR) {
                    Write-Status "Setting ONNXRUNTIME_DIR environment variable" "Action"
                    [System.Environment]::SetEnvironmentVariable("ONNXRUNTIME_DIR", $path, [System.EnvironmentVariableTarget]::User)
                }

                return $true
            }
        }
    }

    Write-Status "ONNX Runtime C++ libraries not found" "Error"
    $global:DependencyStatus["ONNXRuntime"] = @{ Status = "Missing" }
    $global:FailedChecks += "ONNXRuntime"

    Write-Host ""
    Write-Host "MANUAL INSTALLATION REQUIRED:" -ForegroundColor Yellow
    Write-Host "1. Download ONNX Runtime from:" -ForegroundColor White
    Write-Host "   https://github.com/microsoft/onnxruntime/releases" -ForegroundColor Cyan
    Write-Host "2. Download the Windows x64 package (e.g., onnxruntime-win-x64-*.zip)" -ForegroundColor White
    Write-Host "3. Extract to one of these locations:" -ForegroundColor White
    foreach ($path in $possiblePaths[0..3]) {
        Write-Host "   - $path" -ForegroundColor White
    }
    Write-Host "4. Set environment variable:" -ForegroundColor White
    Write-Host "   setx ONNXRUNTIME_DIR `"C:\onnxruntime`"" -ForegroundColor Cyan

    $global:ManualInstalls += @{
        Name = "ONNX Runtime C++ Libraries"
        URL = "https://github.com/microsoft/onnxruntime/releases"
        Instructions = "Download Windows x64 package and extract to C:\onnxruntime"
    }

    return $false
}

# Check Source SDK
function Test-SourceSDK {
    Write-Section "Checking Source SDK / Metamod:Source"

    # Common Source SDK paths
    $steamPath = ""
    $possibleSteamPaths = @(
        "$env:ProgramFiles(x86)\Steam",
        "$env:ProgramFiles\Steam",
        "C:\Steam"
    )

    foreach ($path in $possibleSteamPaths) {
        if (Test-Path "$path\steamapps") {
            $steamPath = $path
            break
        }
    }

    if ($steamPath) {
        Write-Status "Steam found: $steamPath" "Success"

        # Check for Source SDK Base
        $sdkPath = "$steamPath\steamapps\common\Source SDK Base 2013 Multiplayer"
        if (Test-Path $sdkPath) {
            Write-Status "Source SDK Base 2013 MP: Installed" "Success"
            $global:DependencyStatus["SourceSDK"] = @{
                Status = "Installed"
                Path = $sdkPath
            }
        } else {
            Write-Status "Source SDK Base 2013 MP: Not found" "Warning"
            Write-Host "  Install via Steam Library -> Tools -> 'Source SDK Base 2013 Multiplayer'" -ForegroundColor Yellow
        }
    } else {
        Write-Status "Steam not found" "Warning"
    }

    Write-Host ""
    Write-Host "ADDITIONAL REQUIREMENTS:" -ForegroundColor Yellow
    Write-Host "For development, you also need:" -ForegroundColor White
    Write-Host "1. Metamod:Source - https://www.sourcemm.net/downloads.php" -ForegroundColor Cyan
    Write-Host "2. HL2DM SDK headers (included in SDK Base)" -ForegroundColor White

    return $true
}

# Check Git
function Test-Git {
    Write-Section "Checking Git"

    try {
        $gitVersion = git --version 2>&1
        if ($gitVersion -match "git version (.+)") {
            Write-Status "Git installed: $($Matches[1])" "Success"
            $global:DependencyStatus["Git"] = @{
                Status = "Installed"
                Version = $Matches[1]
            }
            return $true
        }
    } catch {
        Write-Status "Git not found" "Warning"
        Write-Host "  Download from: https://git-scm.com/download/win" -ForegroundColor Yellow
    }

    return $false
}

# Check CMake
function Test-CMake {
    Write-Section "Checking CMake"

    try {
        $cmakeVersion = cmake --version 2>&1
        if ($cmakeVersion -match "cmake version (.+)") {
            Write-Status "CMake installed: $($Matches[1])" "Success"
            $global:DependencyStatus["CMake"] = @{
                Status = "Installed"
                Version = $Matches[1]
            }
            return $true
        }
    } catch {
        Write-Status "CMake not found (optional)" "Warning"
        Write-Host "  Download from: https://cmake.org/download/" -ForegroundColor Yellow
    }

    return $false
}

# Check AMBuild
function Test-AMBuild {
    Write-Section "Checking AMBuild"

    try {
        $ambuildVersion = python -m ambuild --version 2>&1
        if ($ambuildVersion -match "AMBuild (.+)") {
            Write-Status "AMBuild installed: $($Matches[1])" "Success"
            $global:DependencyStatus["AMBuild"] = @{
                Status = "Installed"
                Version = $Matches[1]
            }
            return $true
        }
    } catch {
        Write-Status "AMBuild not found" "Warning"
        Write-Host "  Install with: pip install git+https://github.com/alliedmodders/ambuild" -ForegroundColor Yellow

        if ($AutoInstall) {
            Write-Status "Attempting to install AMBuild..." "Action"
            try {
                pip install git+https://github.com/alliedmodders/ambuild
                Write-Status "AMBuild installed successfully" "Success"
                return $true
            } catch {
                Write-Status "Failed to install AMBuild" "Error"
            }
        }
    }

    return $false
}

# Generate dependency report
function Write-DependencyReport {
    Write-Section "Dependency Check Summary"

    $allPassed = $global:FailedChecks.Count -eq 0

    Write-Host ""
    Write-Host "Status Overview:" -ForegroundColor White
    Write-Host "================" -ForegroundColor White

    foreach ($dep in $global:DependencyStatus.Keys | Sort-Object) {
        $status = $global:DependencyStatus[$dep]
        $symbol = if ($status.Status -eq "Installed") { "[✓]" } else { "[✗]" }
        $color = if ($status.Status -eq "Installed") { "Green" } else { "Red" }

        Write-Host "$symbol " -ForegroundColor $color -NoNewline
        Write-Host "$dep : " -NoNewline
        Write-Host "$($status.Status)" -ForegroundColor $color

        if ($status.Version) {
            Write-Host "    Version: $($status.Version)" -ForegroundColor Gray
        }
        if ($status.Path) {
            Write-Host "    Path: $($status.Path)" -ForegroundColor Gray
        }
    }

    Write-Host ""

    if ($global:ManualInstalls.Count -gt 0) {
        Write-Section "Manual Installation Required"

        foreach ($install in $global:ManualInstalls) {
            Write-Host "• $($install.Name)" -ForegroundColor Yellow
            Write-Host "  URL: $($install.URL)" -ForegroundColor Cyan
            Write-Host "  Instructions: $($install.Instructions)" -ForegroundColor White
            Write-Host ""
        }
    }

    if ($allPassed) {
        Write-Host ""
        Write-Status "All critical dependencies are installed!" "Success"
        Write-Host "You can now proceed to build RCBot2." -ForegroundColor Green
        Write-Host ""
        Write-Host "Next steps:" -ForegroundColor White
        Write-Host "1. Run .\ps\build-solution.ps1 to compile the project" -ForegroundColor Cyan
        Write-Host "2. Check docs/ONNX_SETUP.md for runtime configuration" -ForegroundColor Cyan
        Write-Host "3. See docs/ML_TRAINING_GUIDE.md to train ML models" -ForegroundColor Cyan
    } else {
        Write-Host ""
        Write-Status "Some dependencies are missing!" "Error"
        Write-Host "Please install the missing dependencies listed above." -ForegroundColor Red
        Write-Host ""
        Write-Host "After installation:" -ForegroundColor White
        Write-Host "1. Restart PowerShell to refresh environment variables" -ForegroundColor Cyan
        Write-Host "2. Re-run this script to verify: .\ps\verify-dependencies.ps1" -ForegroundColor Cyan
    }

    # Save report to file
    $reportPath = ".\dependency-report.txt"
    $global:DependencyStatus | ConvertTo-Json | Out-File $reportPath
    Write-Host ""
    Write-Status "Full report saved to: $reportPath" "Info"
}

# Main execution
try {
    Initialize-Check

    # Run all checks
    Test-VisualStudio
    Test-Python
    Test-PythonPackages
    Test-ONNXRuntime
    Test-SourceSDK
    Test-Git
    Test-CMake
    Test-AMBuild

    # Generate report
    Write-DependencyReport

} catch {
    Write-Status "An error occurred: $_" "Error"
    Write-Host $_.ScriptStackTrace -ForegroundColor Red
    exit 1
}
