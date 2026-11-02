[CmdletBinding()]
param([string]$GameDirectory='D:\SteamLibrary\steamapps\common\Gothic 3')
$ErrorActionPreference='Stop'
if(Get-Process -Name Gothic3 -ErrorAction SilentlyContinue){throw 'Close Gothic 3 before changing the graphics backend.'}
$game=[IO.Path]::GetFullPath($GameDirectory).TrimEnd('\')
$dll=Join-Path $game 'd3d9.dll'
if(!(Test-Path -LiteralPath $dll)){Write-Output 'Native Direct3D 9 is already active.';return}
$expected=@('265888C31CA78DFFA290C39CB7E50BFB02762590E41927906E46FB32F01497FA','7F46AAE802EA499182EB2C270907AA5073774D4677FA8F5257551911D5A25438','5545BDCEC6923418E5280E7D7DE9541D04D9BAEB1CFA23238F6F0277802E18D5')
if((Get-FileHash -LiteralPath $dll).Hash -notin $expected){throw 'This d3d9.dll differs from the tested DXVK 3.1.1. No files changed.'}
$destination=Join-Path $game ('d3d9.dxvk-disabled-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff')+'.dll')
foreach($path in @($dll,$destination)){if([IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($path)) -ne $game){throw 'Unexpected target path.'}}
Move-Item -LiteralPath $dll -Destination $destination
Write-Output 'DXVK disabled. Launch normally: the VR plugin automatically uses native Direct3D 9.'
