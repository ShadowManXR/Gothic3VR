[CmdletBinding()]
param([ValidateRange(1,120)][int]$Seconds = 15)
$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path $PSScriptRoot -Parent
$probe = Join-Path $projectDirectory 'build\bin\Release\Gothic3VRVdxrProbe.exe'
$loader = Join-Path $projectDirectory 'third_party\openxr\openxr_loader.dll'
if (!(Test-Path -LiteralPath $probe)) { throw 'Build the project first.' }
$logs = Join-Path $projectDirectory 'logs'
New-Item -ItemType Directory -Path $logs -Force | Out-Null
$path = Join-Path $logs ('tracking-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.jsonl')
& $probe --seconds $Seconds $loader | Tee-Object -FilePath $path
$result = $LASTEXITCODE
[PSCustomObject]@{Log=$path;ExitCode=$result}
if ($result -eq 2) { Write-Warning 'No valid headset pose. Connect Quest 3 through Virtual Desktop with VDXR selected, then retry.' }
elseif ($result) { throw 'OpenXR / VDXR tracking probe failed to load.' }
