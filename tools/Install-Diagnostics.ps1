[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$GameDirectory,
    [string]$BuildDirectory = '',
    [string]$PackageDirectory = '',
    [switch]$FfrBackend,
    [string]$StateDirectory = ''
)
$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path $PSScriptRoot -Parent
$buildRoot = if ($BuildDirectory) { [IO.Path]::GetFullPath($BuildDirectory) } else { Join-Path $projectDirectory 'build' }
function Get-SourceHash([string]$Path) {
    $stream = [IO.File]::OpenRead($Path)
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($algorithm.ComputeHash($stream))).Replace('-', '') }
    finally { $stream.Dispose(); $algorithm.Dispose() }
}

# A loose DLL copy cannot update the Meta host, Union protection, preferences
# and rollback journal consistently. All installs use the release deployer.
if (!$PackageDirectory) {
    throw 'Developer installation requires -PackageDirectory pointing to a complete package created with release/Build-Package.py. No game files were changed.'
}
$packageRoot = (Resolve-Path -LiteralPath $PackageDirectory).Path
$manifestFile = Join-Path $packageRoot 'G3VR.Payload.json'
$payloadFile = Join-Path $packageRoot 'G3VR.Payload.zip'
foreach ($path in @($manifestFile, $payloadFile)) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing package file: $path" }
}
$manifest = Get-Content -LiteralPath $manifestFile -Raw -Encoding UTF8 | ConvertFrom-Json
if ($manifest.Format -ne 1) { throw 'Unsupported payload manifest.' }
$required = @(
    'Gothic3VRLauncher.exe',
    'Gothic3VRUnionGuard.exe',
    'patches\Patch_Gothic3VRResolution.dll',
    'scripts\Script_Gothic3VRDiagnostics.dll',
    'scripts\Script_Gothic3VRDiagnostics.ini',
    'scripts\Gothic3VRDiagnostics\openxr_loader.dll',
    'scripts\Gothic3VRDiagnostics\openvr_api.dll',
    'scripts\Gothic3VRDiagnostics\meta64\Gothic3VRMetaHost64.exe',
    'scripts\Gothic3VRDiagnostics\meta64\openxr_loader64.dll'
)
foreach ($name in $required) {
    $entries = @($manifest.Files | Where-Object { $_.Path.Replace('/', '\') -eq $name -and $_.Component -eq 'core' })
    if ($entries.Count -ne 1) { throw "Incomplete developer package: $name" }
}
# Refuse stale packages instead of silently installing an unrelated build.
$builtFiles = @{
    'Gothic3VRLauncher.exe' = 'Gothic3VRLauncher.exe'
    'Gothic3VRUnionGuard.exe' = 'Gothic3VRUnionGuard.exe'
    'patches\Patch_Gothic3VRResolution.dll' = 'Patch_Gothic3VRResolution.dll'
    'scripts\Script_Gothic3VRDiagnostics.dll' = 'Script_Gothic3VRDiagnostics.dll'
}
foreach ($name in $builtFiles.Keys) {
    $source = Join-Path $buildRoot ('bin\Release\' + $builtFiles[$name])
    if (!(Test-Path -LiteralPath $source -PathType Leaf)) { throw "Build file missing: $source" }
    $entry = @($manifest.Files | Where-Object { $_.Path.Replace('/', '\') -eq $name })
    if ((Get-SourceHash $source) -ne $entry[0].Sha) {
        throw "Package does not match the selected build: $name. Recreate the package before installation."
    }
}
$deployArguments = @('-Mode', 'Install', '-GameDirectory', $GameDirectory,
    '-ManifestFile', $manifestFile, '-PayloadFile', $payloadFile,
    '-Components', $(if ($FfrBackend) { 'core,graphics' } else { 'core' }))
if ($StateDirectory) { $deployArguments += @('-StateDirectory', $StateDirectory) }
$powershell = Join-Path $env:WINDIR 'System32\WindowsPowerShell\v1.0\powershell.exe'
& $powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -File (Join-Path $projectDirectory 'release\Deploy.ps1') @deployArguments
if ($LASTEXITCODE) { throw 'Transactional deployment failed. See the deployment error above.' }
