<#
.SYNOPSIS
    RCBot2 AI-Enhanced - Build Script with Verbose Error Logging

.DESCRIPTION
    This interactive PowerShell script builds the RCBot2 solution with comprehensive
    error logging and analysis capabilities. All compilation output is logged to files
    that can be uploaded to Claude for analysis.

.PARAMETER Configuration
    Build configuration: Debug or Release (default: Release)

.PARAMETER Clean
    Perform a clean build (delete previous build artifacts)

.PARAMETER LogDir
    Directory for log files (default: .\build-logs)

.PARAMETER SkipTests
    Skip running tests after build

.PARAMETER Parallel
    Enable parallel compilation (faster but harder to debug)

.NOTES
    Author: RCBot2 AI-Enhanced Team
    Version: 1.0
    Requires: PowerShell 5.1+, Visual Studio or Build Tools
#>

#Requires -Version 5.1

param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [switch]$Clean,
    [string]$LogDir = ".\build-logs",
    [switch]$SkipTests,
    [switch]$Parallel,
    [switch]$Verbose
)

# Script configuration
$ErrorActionPreference = "Stop"
$global:BuildSuccess = $false
$global:BuildStartTime = Get-Date

# Color output helpers
function Write-BuildStatus {
    param([string]$Message, [string]$Type = "Info")
    $timestamp = Get-Date -Format "HH:mm:ss"
    $colors = @{
        "Info" = "Cyan"
        "Success" = "Green"
        "Warning" = "Yellow"
        "Error" = "Red"
        "Action" = "Magenta"
        "Build" = "White"
    }
    Write-Host "[$timestamp][$Type] " -ForegroundColor $colors[$Type] -NoNewline
    Write-Host $Message
}

function Write-BuildSection {
    param([string]$Title)
    $line = "=" * 80
    Write-Host "`n$line" -ForegroundColor DarkGray
    Write-Host "  $Title" -ForegroundColor White
    Write-Host "$line`n" -ForegroundColor DarkGray
}

# Initialize build environment
function Initialize-Build {
    Write-BuildSection "RCBot2 AI-Enhanced Build System"

    Write-BuildStatus "Configuration: $Configuration" "Info"
    Write-BuildStatus "Clean Build: $Clean" "Info"
    Write-BuildStatus "Log Directory: $LogDir" "Info"
    Write-BuildStatus "Parallel Build: $Parallel" "Info"

    # Create log directory
    if (-not (Test-Path $LogDir)) {
        New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
        Write-BuildStatus "Created log directory: $LogDir" "Success"
    }

    # Clean old logs
    $oldLogs = Get-ChildItem $LogDir -Filter "build-*.log" |
               Where-Object { $_.LastWriteTime -lt (Get-Date).AddDays(-7) }
    if ($oldLogs) {
        $oldLogs | Remove-Item -Force
        Write-BuildStatus "Cleaned $($oldLogs.Count) old log files" "Info"
    }

    # Set log file paths
    $timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $global:MainLogFile = Join-Path $LogDir "build-$timestamp.log"
    $global:ErrorLogFile = Join-Path $LogDir "errors-$timestamp.log"
    $global:WarningLogFile = Join-Path $LogDir "warnings-$timestamp.log"
    $global:SummaryLogFile = Join-Path $LogDir "summary-$timestamp.txt"

    Write-BuildStatus "Build log: $global:MainLogFile" "Info"
    Write-BuildStatus "Error log: $global:ErrorLogFile" "Info"

    # Write log headers
    @"
================================================================================
RCBot2 AI-Enhanced Build Log
================================================================================
Build Configuration: $Configuration
Build Type: $(if ($Clean) { 'Clean' } else { 'Incremental' })
Start Time: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
PowerShell Version: $($PSVersionTable.PSVersion)
OS: $([System.Environment]::OSVersion.VersionString)
================================================================================

"@ | Out-File $global:MainLogFile -Encoding UTF8
}

# Find Visual Studio
function Find-VisualStudio {
    Write-BuildSection "Locating Visual Studio"

    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

    if (-not (Test-Path $vsWhere)) {
        throw "vswhere.exe not found. Please install Visual Studio Build Tools."
    }

    # Find latest VS installation with C++ tools
    $vsPath = & $vsWhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath

    if (-not $vsPath) {
        throw "Visual Studio with C++ tools not found. Please run verify-dependencies.ps1"
    }

    Write-BuildStatus "Found Visual Studio: $vsPath" "Success"

    # Find MSBuild
    $msbuildPath = & $vsWhere -latest -requires Microsoft.Component.MSBuild `
        -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1

    if (-not $msbuildPath) {
        throw "MSBuild not found in Visual Studio installation"
    }

    Write-BuildStatus "Found MSBuild: $msbuildPath" "Success"

    # Find vcvarsall.bat for environment setup
    $vcvarsPath = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"
    if (Test-Path $vcvarsPath) {
        Write-BuildStatus "Found vcvarsall: $vcvarsPath" "Success"
    }

    return @{
        VSPath = $vsPath
        MSBuild = $msbuildPath
        VCVarsAll = $vcvarsPath
    }
}

# Find solution/project files
function Find-BuildSystem {
    Write-BuildSection "Detecting Build System"

    $buildInfo = @{
        Type = "Unknown"
        Files = @()
    }

    # Check for Visual Studio solution
    $slnFiles = Get-ChildItem -Path . -Filter "*.sln" -Recurse -Depth 2
    if ($slnFiles) {
        $buildInfo.Type = "VisualStudio"
        $buildInfo.Files = $slnFiles
        Write-BuildStatus "Found Visual Studio solution(s): $($slnFiles.Count)" "Success"
        foreach ($sln in $slnFiles) {
            Write-BuildStatus "  - $($sln.FullName)" "Info"
        }
    }

    # Check for CMakeLists.txt
    $cmakeFiles = Get-ChildItem -Path . -Filter "CMakeLists.txt" -Recurse -Depth 2
    if ($cmakeFiles) {
        if ($buildInfo.Type -eq "Unknown") {
            $buildInfo.Type = "CMake"
        }
        Write-BuildStatus "Found CMake project(s): $($cmakeFiles.Count)" "Success"
    }

    # Check for AMBuild
    $ambuildFiles = Get-ChildItem -Path . -Filter "AMBuilder" -Recurse -Depth 2
    if ($ambuildFiles) {
        if ($buildInfo.Type -eq "Unknown") {
            $buildInfo.Type = "AMBuild"
        }
        $buildInfo.AMBuilder = $ambuildFiles[0].FullName
        Write-BuildStatus "Found AMBuild configuration" "Success"
    }

    # Check for Makefile
    $makefiles = Get-ChildItem -Path . -Filter "Makefile" -Recurse -Depth 2
    if ($makefiles) {
        if ($buildInfo.Type -eq "Unknown") {
            $buildInfo.Type = "Make"
        }
        Write-BuildStatus "Found Makefile(s): $($makefiles.Count)" "Success"
    }

    if ($buildInfo.Type -eq "Unknown") {
        Write-BuildStatus "No recognized build system found" "Warning"
        Write-BuildStatus "Attempting manual compilation..." "Action"
    }

    return $buildInfo
}

# Build with Visual Studio / MSBuild
function Build-WithMSBuild {
    param($VSInfo, $SolutionFile)

    Write-BuildSection "Building with MSBuild"

    $msbuild = $VSInfo.MSBuild
    $platform = "x64"

    # MSBuild arguments
    $msbuildArgs = @(
        "`"$($SolutionFile.FullName)`""
        "/t:$(if ($Clean) { 'Rebuild' } else { 'Build' })"
        "/p:Configuration=$Configuration"
        "/p:Platform=$platform"
        "/v:detailed"
        "/fl"
        "/flp:LogFile=`"$global:MainLogFile`";Verbosity=detailed;Encoding=UTF-8"
        "/flp1:LogFile=`"$global:ErrorLogFile`";ErrorsOnly;Encoding=UTF-8"
        "/flp2:LogFile=`"$global:WarningLogFile`";WarningsOnly;Encoding=UTF-8"
    )

    if ($Parallel) {
        $msbuildArgs += "/m"
    }

    # Add ONNX Runtime paths if available
    if ($env:ONNXRUNTIME_DIR) {
        Write-BuildStatus "Using ONNX Runtime: $env:ONNXRUNTIME_DIR" "Info"
        $msbuildArgs += "/p:ONNXRUNTIME_DIR=`"$env:ONNXRUNTIME_DIR`""
    }

    Write-BuildStatus "Build command: $msbuild $($msbuildArgs -join ' ')" "Info"
    Write-BuildStatus "Starting build..." "Action"

    $buildOutput = & $msbuild @msbuildArgs 2>&1
    $buildExitCode = $LASTEXITCODE

    # Write output to console
    $buildOutput | ForEach-Object {
        $line = $_.ToString()
        if ($line -match "error") {
            Write-Host $line -ForegroundColor Red
        } elseif ($line -match "warning") {
            Write-Host $line -ForegroundColor Yellow
        } elseif ($Verbose) {
            Write-Host $line -ForegroundColor Gray
        }
    }

    # Append to main log
    $buildOutput | Out-File $global:MainLogFile -Append -Encoding UTF8

    return $buildExitCode -eq 0
}

# Build with AMBuild
function Build-WithAMBuild {
    param($AMBuilderPath)

    Write-BuildSection "Building with AMBuild"

    # Check Python and AMBuild
    try {
        $ambuildVersion = python -m ambuild --version 2>&1
        Write-BuildStatus "AMBuild version: $ambuildVersion" "Info"
    } catch {
        throw "AMBuild not found. Install with: pip install git+https://github.com/alliedmodders/ambuild"
    }

    # Create build directory
    $buildDir = ".\build-$Configuration"
    if ($Clean -and (Test-Path $buildDir)) {
        Write-BuildStatus "Cleaning build directory: $buildDir" "Action"
        Remove-Item -Recurse -Force $buildDir
    }

    if (-not (Test-Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir | Out-Null
    }

    Push-Location $buildDir

    try {
        # Configure
        Write-BuildStatus "Configuring with AMBuild..." "Action"
        $configOutput = python -m ambuild --config $Configuration 2>&1 | Tee-Object -Variable configLog
        $configOutput | Out-File $global:MainLogFile -Append -Encoding UTF8

        if ($LASTEXITCODE -ne 0) {
            throw "AMBuild configuration failed"
        }

        # Build
        Write-BuildStatus "Building..." "Action"
        $buildOutput = python -m ambuild build 2>&1 | Tee-Object -Variable buildLog
        $buildOutput | Out-File $global:MainLogFile -Append -Encoding UTF8

        $success = $LASTEXITCODE -eq 0

        Pop-Location
        return $success

    } catch {
        Pop-Location
        throw
    }
}

# Build with CMake
function Build-WithCMake {
    Write-BuildSection "Building with CMake"

    # Check CMake
    try {
        $cmakeVersion = cmake --version 2>&1
        Write-BuildStatus "CMake version: $cmakeVersion" "Info"
    } catch {
        throw "CMake not found. Download from https://cmake.org/download/"
    }

    # Create build directory
    $buildDir = ".\build-cmake-$Configuration"
    if ($Clean -and (Test-Path $buildDir)) {
        Write-BuildStatus "Cleaning build directory: $buildDir" "Action"
        Remove-Item -Recurse -Force $buildDir
    }

    if (-not (Test-Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir | Out-Null
    }

    Push-Location $buildDir

    try {
        # Configure
        Write-BuildStatus "Configuring with CMake..." "Action"
        $cmakeArgs = @(
            ".."
            "-G", "Visual Studio 17 2022"
            "-A", "x64"
            "-DCMAKE_BUILD_TYPE=$Configuration"
        )

        if ($env:ONNXRUNTIME_DIR) {
            $cmakeArgs += "-DONNXRUNTIME_DIR=`"$env:ONNXRUNTIME_DIR`""
        }

        $configOutput = cmake @cmakeArgs 2>&1 | Tee-Object -Variable configLog
        $configOutput | Out-File $global:MainLogFile -Append -Encoding UTF8

        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed"
        }

        # Build
        Write-BuildStatus "Building..." "Action"
        $buildOutput = cmake --build . --config $Configuration $(if ($Parallel) { "--parallel" } else { "" }) 2>&1 | Tee-Object -Variable buildLog
        $buildOutput | Out-File $global:MainLogFile -Append -Encoding UTF8

        $success = $LASTEXITCODE -eq 0

        Pop-Location
        return $success

    } catch {
        Pop-Location
        throw
    }
}

# Manual compilation fallback
function Build-ManualCompilation {
    Write-BuildSection "Manual Compilation"

    Write-BuildStatus "Scanning for C++ source files..." "Action"

    $sourceFiles = Get-ChildItem -Path ".\utils\RCBot2_meta" -Filter "*.cpp" -Recurse
    Write-BuildStatus "Found $($sourceFiles.Count) source files" "Info"

    Write-BuildStatus "This project requires a proper build system." "Warning"
    Write-Host ""
    Write-Host "Recommended setup:" -ForegroundColor Yellow
    Write-Host "1. If using Source SDK, use AMBuild:" -ForegroundColor White
    Write-Host "   pip install git+https://github.com/alliedmodders/ambuild" -ForegroundColor Cyan
    Write-Host "   python -m ambuild configure" -ForegroundColor Cyan
    Write-Host "   python -m ambuild build" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "2. Or create a CMakeLists.txt for CMake builds" -ForegroundColor White
    Write-Host "3. Or create a Visual Studio solution (.sln)" -ForegroundColor White

    return $false
}

# Analyze build errors
function Analyze-BuildErrors {
    Write-BuildSection "Analyzing Build Results"

    $errorCount = 0
    $warningCount = 0
    $errorSummary = @()
    $warningSummary = @()

    # Parse error log
    if (Test-Path $global:ErrorLogFile) {
        $errors = Get-Content $global:ErrorLogFile -Encoding UTF8
        $errorCount = ($errors | Where-Object { $_ -match "error" }).Count

        # Extract unique errors
        $errorSummary = $errors |
            Where-Object { $_ -match "error" } |
            ForEach-Object {
                if ($_ -match "(?<file>[^(]+)\((?<line>\d+)\): error (?<code>\w+): (?<message>.+)") {
                    [PSCustomObject]@{
                        File = $matches.file
                        Line = $matches.line
                        Code = $matches.code
                        Message = $matches.message
                    }
                }
            } |
            Group-Object Code |
            ForEach-Object {
                [PSCustomObject]@{
                    ErrorCode = $_.Name
                    Count = $_.Count
                    Example = $_.Group[0].Message
                    Files = ($_.Group | Select-Object -ExpandProperty File -Unique)
                }
            }
    }

    # Parse warning log
    if (Test-Path $global:WarningLogFile) {
        $warnings = Get-Content $global:WarningLogFile -Encoding UTF8
        $warningCount = ($warnings | Where-Object { $_ -match "warning" }).Count
    }

    # Generate summary
    $summary = @"
================================================================================
BUILD SUMMARY
================================================================================
Configuration: $Configuration
Result: $(if ($global:BuildSuccess) { 'SUCCESS' } else { 'FAILED' })
Duration: $((Get-Date) - $global:BuildStartTime)
Errors: $errorCount
Warnings: $warningCount
================================================================================

"@

    if ($errorSummary) {
        $summary += "`nERROR BREAKDOWN:`n"
        $summary += "================`n"
        foreach ($err in $errorSummary) {
            $summary += "`nError $($err.ErrorCode): $($err.Count) occurrence(s)`n"
            $summary += "Example: $($err.Example)`n"
            $summary += "Affected files: $($err.Files -join ', ')`n"
        }
    }

    $summary += "`nLOG FILES:`n"
    $summary += "==========`n"
    $summary += "Main Log: $global:MainLogFile`n"
    $summary += "Errors: $global:ErrorLogFile`n"
    $summary += "Warnings: $global:WarningLogFile`n"
    $summary += "Summary: $global:SummaryLogFile`n"

    $summary | Out-File $global:SummaryLogFile -Encoding UTF8

    # Display summary
    Write-Host $summary

    if (-not $global:BuildSuccess) {
        Write-Host "`nTO ANALYZE ERRORS WITH CLAUDE:" -ForegroundColor Yellow
        Write-Host "===============================" -ForegroundColor Yellow
        Write-Host "Upload these files for analysis:" -ForegroundColor White
        Write-Host "1. $global:ErrorLogFile (compilation errors)" -ForegroundColor Cyan
        Write-Host "2. $global:SummaryLogFile (error summary)" -ForegroundColor Cyan
        Write-Host "3. $global:MainLogFile (full build log - if needed)" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "Sample prompt for Claude:" -ForegroundColor White
        Write-Host '"I am getting compilation errors in my C++ project. ' -ForegroundColor Gray
        Write-Host 'Please analyze the attached error log and suggest fixes."' -ForegroundColor Gray
    }
}

# Main execution
try {
    Initialize-Build

    # Find Visual Studio
    $vsInfo = Find-VisualStudio

    # Detect build system
    $buildSystem = Find-BuildSystem

    # Execute appropriate build
    Write-BuildSection "Executing Build"

    switch ($buildSystem.Type) {
        "VisualStudio" {
            $global:BuildSuccess = Build-WithMSBuild -VSInfo $vsInfo -SolutionFile $buildSystem.Files[0]
        }
        "AMBuild" {
            $global:BuildSuccess = Build-WithAMBuild -AMBuilderPath $buildSystem.AMBuilder
        }
        "CMake" {
            $global:BuildSuccess = Build-WithCMake
        }
        default {
            $global:BuildSuccess = Build-ManualCompilation
        }
    }

    # Analyze results
    Analyze-BuildErrors

    if ($global:BuildSuccess) {
        Write-BuildStatus "BUILD SUCCEEDED!" "Success"
        exit 0
    } else {
        Write-BuildStatus "BUILD FAILED!" "Error"
        exit 1
    }

} catch {
    Write-BuildStatus "Build script error: $_" "Error"
    Write-Host $_.ScriptStackTrace -ForegroundColor Red

    # Log exception
    "EXCEPTION: $_`n$($_.ScriptStackTrace)" | Out-File $global:ErrorLogFile -Append -Encoding UTF8

    exit 1
}
