[CmdletBinding()]
param([switch]$Install, [string]$GameDirectory = '', [string]$BuildDirectory = '',
      [string]$SdkDirectory = '', [string]$PackageDirectory = '')
$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path $PSScriptRoot -Parent
if ($Install -and (!$GameDirectory -or !$PackageDirectory)) {
    throw '-Install requires explicit -GameDirectory and -PackageDirectory. Build without -Install, create a complete package, then deploy it.'
}
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio Installer/vswhere is required.' }
$vsDirectory = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vsDirectory) { throw 'Install Visual Studio 2022 C++ Build Tools.' }
$cmake = Join-Path $vsDirectory 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ctest = Join-Path (Split-Path $cmake -Parent) 'ctest.exe'
$buildDirectory = if($BuildDirectory){[System.IO.Path]::GetFullPath($BuildDirectory)}else{Join-Path $projectDirectory 'build'}
$configureArguments = @('-S', $projectDirectory, '-B', $buildDirectory, '-G', 'Visual Studio 17 2022', '-A', 'Win32', '-DBUILD_TESTING=ON')
if ($SdkDirectory) { $configureArguments += "-DGOTHIC3SDK_DIR=$([IO.Path]::GetFullPath($SdkDirectory))" }
& $cmake @configureArguments
if ($LASTEXITCODE) { throw 'CMake configuration failed.' }
& $cmake --build $buildDirectory --config Release --parallel 4
if ($LASTEXITCODE) { throw 'Build failed.' }
& $ctest --test-dir $buildDirectory -C Release --output-on-failure
if ($LASTEXITCODE) { throw 'Hook tests failed; plugin was not installed.' }
if ($Install) { & (Join-Path $PSScriptRoot 'Install-Diagnostics.ps1') -GameDirectory $GameDirectory -BuildDirectory $buildDirectory -PackageDirectory $PackageDirectory }
