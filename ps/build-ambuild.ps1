<#
.SYNOPSIS
    RCBot2 - AMBuild Compilation Script

.DESCRIPTION
    Build script specifically for RCBot2 using AMBuild (AlliedModders Build System).
    This is the recommended build method for RCBot2 as a Metamod:Source plugin.

.PARAMETER SDKPath
    Path to HL2SDK root directory containing sdk folders (e.g., hl2sdk-tf2)

.PARAMETER MMSPath
    Path to Metamod:Source installation

.PARAMETER SMPath
    Optional: Path to SourceMod for extension support

.PARAMETER Configuration
    Build configuration: debug or release (default: release)

.PARAMETER Clean
    Perform a clean build

.PARAMETER Verbose
    Show detailed build output

.EXAMPLE
    .\ps\build-ambuild.ps1 -SDKPath "C:\hl2sdk" -MMSPath "C:\metamod-source"

.NOTES
    Requires:
    - Python 3.8+
    - AMBuild: pip install git+https://github.com/alliedmodders/ambuild
    - HL2SDK (Source SDK 2013)
    - Metamod:Source
    - ONNX Runtime (for ML features)
#>

#Requires -Version 5.1

param(
    [Parameter(Mandatory=$false)]
    [string]$SDKPath,

    [Parameter(Mandatory=$false)]
    [string]$MMSPath,

    [Parameter(Mandatory=$false)]
    [string]$SMPath,

    [ValidateSet("debug", "release")]
    [string]$Configuration = "release",

    [switch]$Clean,
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"

# Color output helpers
function Write-BuildLog {
    param([string]$Message, [string]$Type = "Info")
    $timestamp = Get-Date -Format "HH:mm:ss"
    $colors = @{
        "Info" = "Cyan"
        "Success" = "Green"
        "Warning" = "Yellow"
        "Error" = "Red"
        "Action" = "Magenta"
    }
    Write-Host "[$timestamp][$Type] " -ForegroundColor $colors[$Type] -NoNewline
    Write-Host $Message
}

function Write-BuildHeader {
    param([string]$Title)
    Write-Host "`n$('=' * 80)" -ForegroundColor DarkCyan
    Write-Host "  $Title" -ForegroundColor White
    Write-Host "$('=' * 80)`n" -ForegroundColor DarkCyan
}

# Initialize
Write-BuildHeader "RCBot2 AMBuild Compilation"

# Check Python
Write-BuildLog "Checking Python..." "Info"
try {
    $pythonVersion = python --version 2>&1
    if ($pythonVersion -match "Python (\d+\.\d+\.\d+)") {
        Write-BuildLog "Python: $($Matches[1])" "Success"
    } else {
        throw "Python version not detected"
    }
} catch {
    Write-BuildLog "Python not found! Install from https://www.python.org/downloads/" "Error"
    exit 1
}

# Check AMBuild
Write-BuildLog "Checking AMBuild..." "Info"
try {
    $ambuildCheck = python -c "import ambuild2; print(ambuild2.__version__)" 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-BuildLog "AMBuild: $ambuildCheck" "Success"
    } else {
        throw "AMBuild import failed"
    }
} catch {
    Write-BuildLog "AMBuild not found!" "Error"
    Write-Host ""
    Write-Host "Install AMBuild with:" -ForegroundColor Yellow
    Write-Host "  pip install git+https://github.com/alliedmodders/ambuild" -ForegroundColor Cyan
    exit 1
}

# Check for AMBuilder file
if (-not (Test-Path ".\AMBuilder")) {
    Write-BuildLog "AMBuilder not found in current directory!" "Error"
    Write-BuildLog "Please run this script from the RCBot2 root directory" "Error"
    exit 1
}

# Detect or prompt for SDK paths
Write-BuildHeader "Configuring Build Environment"

# Check for environment variables
if (-not $SDKPath) {
    if ($env:HL2SDKROOT) {
        $SDKPath = $env:HL2SDKROOT
        Write-BuildLog "Using HL2SDKROOT: $SDKPath" "Info"
    }
}

if (-not $MMSPath) {
    if ($env:MMSOURCE_DEV) {
        $MMSPath = $env:MMSOURCE_DEV
        Write-BuildLog "Using MMSOURCE_DEV: $MMSPath" "Info"
    }
}

# Prompt for missing paths
if (-not $SDKPath) {
    Write-Host ""
    Write-Host "HL2SDK Path Required" -ForegroundColor Yellow
    Write-Host "===================" -ForegroundColor Yellow
    Write-Host "The HL2SDK root directory should contain folders like:" -ForegroundColor White
    Write-Host "  - hl2sdk-tf2" -ForegroundColor Gray
    Write-Host "  - hl2sdk-hl2dm" -ForegroundColor Gray
    Write-Host "  - hl2sdk-csgo" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Download from: https://github.com/alliedmodders/hl2sdk" -ForegroundColor Cyan
    Write-Host ""
    $SDKPath = Read-Host "Enter HL2SDK root path (or Ctrl+C to exit)"

    if (-not (Test-Path $SDKPath)) {
        Write-BuildLog "Path does not exist: $SDKPath" "Error"
        exit 1
    }
}

if (-not $MMSPath) {
    Write-Host ""
    Write-Host "Metamod:Source Path Required" -ForegroundColor Yellow
    Write-Host "============================" -ForegroundColor Yellow
    Write-Host "Download from: https://www.sourcemm.net/downloads.php" -ForegroundColor Cyan
    Write-Host ""
    $MMSPath = Read-Host "Enter Metamod:Source path (or Ctrl+C to exit)"

    if (-not (Test-Path $MMSPath)) {
        Write-BuildLog "Path does not exist: $MMSPath" "Error"
        exit 1
    }
}

# Check ONNX Runtime
Write-BuildLog "Checking ONNX Runtime..." "Info"
$onnxPath = $env:ONNXRUNTIME_DIR
if ($onnxPath -and (Test-Path $onnxPath)) {
    Write-BuildLog "ONNX Runtime: $onnxPath" "Success"
    Write-BuildLog "ML features will be enabled (RCBOT_WITH_ONNX)" "Success"
} else {
    Write-BuildLog "ONNX Runtime not found - ML features will be disabled" "Warning"
    Write-Host "  Set ONNXRUNTIME_DIR environment variable to enable ML" -ForegroundColor Yellow
}

# Create build directory
$buildDir = ".\build-$Configuration"
if ($Clean -and (Test-Path $buildDir)) {
    Write-BuildLog "Cleaning build directory: $buildDir" "Action"
    Remove-Item -Recurse -Force $buildDir
}

if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
    Write-BuildLog "Created build directory: $buildDir" "Success"
}

Push-Location $buildDir

try {
    Write-BuildHeader "Configuring with AMBuild"

    # Build AMBuild configure command
    $configArgs = @(
        "-c", "../configure.py"
        "--enable-$Configuration"
        "--hl2sdk-root=`"$SDKPath`""
        "--mms-path=`"$MMSPath`""
    )

    if ($SMPath) {
        $configArgs += "--sm-path=`"$SMPath`""
        Write-BuildLog "SourceMod support enabled: $SMPath" "Success"
    }

    # Add ONNX support if available
    if ($onnxPath) {
        $configArgs += "--enable-onnx"
    }

    Write-BuildLog "Configuration command:" "Info"
    Write-Host "  python $($configArgs -join ' ')" -ForegroundColor Gray
    Write-Host ""

    # Run configure
    $configOutput = python @configArgs 2>&1
    $configOutput | ForEach-Object {
        if ($Verbose) {
            Write-Host $_ -ForegroundColor Gray
        }
    }

    if ($LASTEXITCODE -ne 0) {
        Write-BuildLog "Configuration failed!" "Error"
        Write-Host ""
        Write-Host "Configuration output:" -ForegroundColor Yellow
        $configOutput | ForEach-Object { Write-Host $_ }
        throw "AMBuild configuration failed with exit code $LASTEXITCODE"
    }

    Write-BuildLog "Configuration successful!" "Success"

    Write-BuildHeader "Building RCBot2"

    # Run build
    $buildStartTime = Get-Date
    $buildOutput = python -c "import ambuild2; ambuild2.run.Build()" 2>&1 | Tee-Object -Variable buildLog

    $buildDuration = (Get-Date) - $buildStartTime

    if ($Verbose) {
        $buildOutput | ForEach-Object {
            if ($_ -match "error") {
                Write-Host $_ -ForegroundColor Red
            } elseif ($_ -match "warning") {
                Write-Host $_ -ForegroundColor Yellow
            } else {
                Write-Host $_ -ForegroundColor Gray
            }
        }
    }

    if ($LASTEXITCODE -ne 0) {
        Write-BuildLog "Build failed!" "Error"
        Write-Host ""
        Write-Host "Build output:" -ForegroundColor Yellow
        $buildOutput | ForEach-Object {
            if ($_ -match "error") {
                Write-Host $_ -ForegroundColor Red
            } else {
                Write-Host $_
            }
        }
        throw "Build failed with exit code $LASTEXITCODE"
    }

    Write-BuildHeader "Build Successful!"

    Write-BuildLog "Configuration: $Configuration" "Success"
    Write-BuildLog "Build time: $($buildDuration.ToString('mm\:ss'))" "Success"

    # Find built binaries
    Write-Host ""
    Write-Host "Built binaries:" -ForegroundColor Green
    Get-ChildItem -Recurse -Include "*.so", "*.dll" | ForEach-Object {
        Write-Host "  $($_.FullName)" -ForegroundColor Cyan
    }

    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor White
    Write-Host "1. Copy the .dll/.so files to your game's addons/metamod folder" -ForegroundColor Gray
    Write-Host "2. If using ML features, copy ONNX Runtime DLLs to the same folder" -ForegroundColor Gray
    Write-Host "3. See docs/ONNX_SETUP.md for runtime configuration" -ForegroundColor Gray

    Pop-Location
    exit 0

} catch {
    Pop-Location
    Write-BuildLog "Build error: $_" "Error"
    Write-Host $_.ScriptStackTrace -ForegroundColor Red
    exit 1
}
