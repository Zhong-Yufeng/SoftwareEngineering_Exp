# CSHTP build entry point.
#   .\run.ps1 run   -- build and launch the CLI program (default)
#   .\run.ps1 test  -- build and run the smoke test suite, exit code 0 = all passed
param(
    [Parameter(Position = 0)]
    [ValidateSet('run', 'test')]
    [string]$Task = 'run'
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

# ---------------------------------------------------------------------------
# 1. Locate vcvars64.bat and import the MSVC x64 environment
# ---------------------------------------------------------------------------
$VsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$VcVars = $null

if (Test-Path $VsWhere) {
    $InstallPath = & $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($InstallPath) {
        $Candidate = Join-Path $InstallPath 'VC\Auxiliary\Build\vcvars64.bat'
        if (Test-Path $Candidate) { $VcVars = $Candidate }
    }
}
if (-not $VcVars) {
    $Fallback = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat'
    if (Test-Path $Fallback) { $VcVars = $Fallback }
}
if (-not $VcVars) {
    Write-Host '[ERROR] vcvars64.bat not found. Install "Desktop development with C++" in Visual Studio.'
    exit 1
}

$EnvLines = cmd /c "`"$VcVars`" >nul 2>&1 && set"
if ($LASTEXITCODE -ne 0) {
    Write-Host '[ERROR] failed to initialize the MSVC environment.'
    exit 1
}
foreach ($Line in $EnvLines) {
    if ($Line -match '^([^=]+)=(.*)$') {
        Set-Item -Path ("env:" + $Matches[1]) -Value $Matches[2] -ErrorAction SilentlyContinue
    }
}

# ---------------------------------------------------------------------------
# 2. Build helper
# ---------------------------------------------------------------------------
function Build-Executable {
    param(
        [string[]]$Sources,
        [string[]]$IncludeDirs,
        [string]$ExeName
    )

    $OutDir = Join-Path $env:TEMP ("cshtp_build_" + [System.IO.Path]::GetFileNameWithoutExtension($ExeName))
    if (Test-Path $OutDir) { Remove-Item $OutDir -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

    $CompilerOutput = & cl /nologo /std:c++17 /EHsc /utf-8 /W3 $IncludeDirs @Sources `
                           "/Fe:$OutDir\$ExeName" "/Fo:$OutDir\" 2>&1
    $CompilerOutput | Write-Host
    if ($LASTEXITCODE -ne 0) {
        Write-Host '[ERROR] build failed.'
        exit 1
    }
    return (Join-Path $OutDir $ExeName)
}

$SrcDir = Join-Path $Root 'src'
$CoreDir = Join-Path $SrcDir 'core'
$StoreDir = Join-Path $SrcDir 'store'
$InMemoryDir = Join-Path $StoreDir 'InMemory'
$DbDir = Join-Path $StoreDir 'DB'
$ServiceDir = Join-Path $SrcDir 'service'
$CliDir = Join-Path $SrcDir 'cli'
$TestDir = Join-Path $Root 'test'

$CommonSources = @((Get-ChildItem -Path $CoreDir, $StoreDir, $ServiceDir -Filter *.cpp -Recurse | Sort-Object FullName | ForEach-Object { $_.FullName })
                   (Join-Path $DbDir 'sqlite3.c'))

# ---------------------------------------------------------------------------
# 3. Dispatch
# ---------------------------------------------------------------------------
if ($Task -eq 'run') {
    $Sources = @((Join-Path $SrcDir 'main.cpp'), (Join-Path $CliDir 'Cli.cpp')) + $CommonSources
    $IncludeDirs = @('/I', $SrcDir, '/I', $CoreDir, '/I', $StoreDir, '/I', $InMemoryDir, '/I', $DbDir,
                     '/I', $ServiceDir, '/I', $CliDir)
    Write-Host 'Building CSHTP ...'
    $Exe = Build-Executable -Sources $Sources -IncludeDirs $IncludeDirs -ExeName 'cshtp.exe'
    Write-Host ''
    Write-Host "[OK] $Exe"
    Write-Host ''
    & $Exe
    exit $LASTEXITCODE
}

# ---------------------------------------------------------------------------
# 4. Smoke tests
# ---------------------------------------------------------------------------
$TestSources = @((Get-ChildItem -Path $TestDir -Filter *.cpp | Sort-Object Name | ForEach-Object { $_.FullName })) + $CommonSources
$TestIncludeDirs = @('/I', $SrcDir, '/I', $CoreDir, '/I', $StoreDir, '/I', $InMemoryDir, '/I', $DbDir,
                     '/I', $ServiceDir, '/I', $TestDir)
Write-Host 'Building smoke tests ...'
$TestExe = Build-Executable -Sources $TestSources -IncludeDirs $TestIncludeDirs -ExeName 'cshtp_tests.exe'

Write-Host ''
Write-Host 'Running smoke tests ...'
Write-Host ''
& $TestExe
$Code = $LASTEXITCODE
Write-Host ''
if ($Code -eq 0) {
    Write-Host '[OK] all smoke tests passed.'
} else {
    Write-Host "[ERROR] smoke tests failed (exit code $Code)."
}
exit $Code
