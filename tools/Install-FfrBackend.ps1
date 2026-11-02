[CmdletBinding()]
param([string]$GameDirectory='D:\SteamLibrary\steamapps\common\Gothic 3')
$ErrorActionPreference='Stop'
if(Get-Process -Name Gothic3 -ErrorAction SilentlyContinue){throw 'Close Gothic 3 before installing the graphics backend.'}
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
if(!(Test-Path -LiteralPath (Join-Path $game 'Gothic3.exe'))){throw 'Gothic3.exe missing.'}
if(!(Test-Path -LiteralPath (Join-Path $game 'scripts\Script_Gothic3VRDiagnostics.dll'))){throw 'Install the VR plugin first.'}
$project=Split-Path $PSScriptRoot -Parent
$source=Join-Path $project 'third_party\dxvk-ffr\x32\d3d9.dll'
$expected='5545BDCEC6923418E5280E7D7DE9541D04D9BAEB1CFA23238F6F0277802E18D5'
if((Get-FileHash -LiteralPath $source).Hash -ne $expected){throw 'FFR backend differs from the GPU-tested binary.'}
$target=Join-Path $game 'd3d9.dll'
if(Test-Path -LiteralPath $target){
    $old=(Get-FileHash -LiteralPath $target).Hash
    if($old -eq $expected){Write-Output 'FFR DXVK backend already installed.';return}
    if($old -notin @('265888C31CA78DFFA290C39CB7E50BFB02762590E41927906E46FB32F01497FA','7F46AAE802EA499182EB2C270907AA5073774D4677FA8F5257551911D5A25438')){throw 'Unrecognized existing d3d9.dll; no file changed.'}
    $backup=Join-Path $project ('backups\'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff')+'-before-ffr')
    New-Item -ItemType Directory -Path $backup | Out-Null
    Copy-Item -LiteralPath $target -Destination (Join-Path $backup 'd3d9.dll')
}
try {
    Copy-Item -LiteralPath $source -Destination $target -Force
    Copy-Item -LiteralPath (Join-Path $project 'third_party\dxvk-ffr\LICENSE') -Destination (Join-Path $game 'scripts\Gothic3VRDiagnostics\DXVK-FFR-LICENSE.txt') -Force
    if((Get-FileHash -LiteralPath $target).Hash -ne $expected){throw 'Installed FFR backend hash mismatch.'}
} catch {
    if($backup){Copy-Item -LiteralPath (Join-Path $backup 'd3d9.dll') -Destination $target -Force}
    elseif(Test-Path -LiteralPath $target){Remove-Item -LiteralPath $target -Force}
    throw
}
Write-Output 'FFR backend installed. Select a profile in the VR launcher and restart the game.'
