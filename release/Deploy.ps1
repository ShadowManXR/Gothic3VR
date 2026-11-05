[CmdletBinding()]
param(
 [ValidateSet('Check','Install','Uninstall','CheckUninstall','ApplyBeta')][string]$Mode='Check',
 [Parameter(Mandatory=$true)][string]$GameDirectory,
 [string]$ManifestFile='',
 [string]$PayloadFile='',
 [string]$Components='core,graphics,branding,music',
 [string]$StateDirectory=(Join-Path $env:LOCALAPPDATA 'Gothic3VR\Installations'),
 [string]$ReportFile='',
 [string]$PrerequisitePlan='',
 [string]$BetaDirectory=''
)
$ErrorActionPreference='Stop'
$helperRoot=[IO.Path]::GetDirectoryName($MyInvocation.MyCommand.Path)
if(!$ManifestFile){$ManifestFile=Join-Path $helperRoot 'G3VR.Payload.json'}
if(!$PayloadFile){$PayloadFile=Join-Path $helperRoot 'G3VR.Payload.zip'}
$transactionLock=$null
$script:Events=New-Object 'System.Collections.Generic.List[string]'
function Note([string]$s){$script:Events.Add($s)}
function Need([bool]$ok,[string]$s){if(!$ok){throw $s}}
function Hash([string]$p){$stream=[IO.File]::OpenRead($p);$digest=[Security.Cryptography.SHA256]::Create();try{return ([BitConverter]::ToString($digest.ComputeHash($stream))).Replace('-','')}finally{$stream.Dispose();$digest.Dispose()}}
function Full([string]$p){[IO.Path]::GetFullPath($p).TrimEnd('\')}
function Below([string]$p,[string]$root){(Full $p).StartsWith((Full $root)+'\',[StringComparison]::OrdinalIgnoreCase)}
function NoLinks([string]$p){
 $walk=Full $p
 while($walk){if(Test-Path -LiteralPath $walk){Need (((Get-Item -LiteralPath $walk -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) -eq 0) "Linked paths are not supported: $walk"};$walk=[IO.Path]::GetDirectoryName($walk)}
}
function Relative([string]$root,[string]$name){
 Need (![string]::IsNullOrWhiteSpace($name) -and ![IO.Path]::IsPathRooted($name)) 'Invalid relative payload path.'
 $parts=$name.Replace('/','\').Split('\');foreach($part in $parts){Need ($part -and $part -ne '.' -and $part -ne '..' -and $part -notmatch '[:*?"<>|]' -and !$part.EndsWith('.') -and !$part.EndsWith(' ')) 'Unsafe payload path.'}
 $p=Full (Join-Path $root ($parts -join '\'));Need (Below $p $root) 'Payload path escapes its destination.';NoLinks $p;return $p
}
function JsonSave([string]$p,$value){
 $temp=$p+'.new';[IO.File]::WriteAllText($temp,($value|ConvertTo-Json -Depth 12),[Text.UTF8Encoding]::new($false))
 if(Test-Path -LiteralPath $p){[IO.File]::Replace($temp,$p,[NullString]::Value)}else{[IO.File]::Move($temp,$p)}
}
function CopySafe([string]$from,[string]$to){NoLinks $from;NoLinks $to;[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($to))|Out-Null;[IO.File]::Copy($from,$to,$true)}
function WritePrerequisitePlan($packages, $missing){
 # The manifest pins the final native DLLs. SharedBase comes from CP,
 # GFC from 1.04.11, and the remaining non-PUP DLLs from the beta.
 $cp = 'SharedBase.dll' -in $missing.Path
 $updatePack = $cp -or 'GFC.dll' -in $missing.Path
 $beta = $updatePack -or ($missing.Path -match '^(Engine|Game|GUI|Script)\.dll$|^scripts\\Script_Game\.dll$')
 $parallel = $cp -or ($missing.Path -match '^(scripts\\Script_paru|paru_s1|paru_s1_opt|paru_s2)\.dll$')
 $needed = @($cp, $updatePack, $beta, $parallel)
 $lines = foreach($package in ($packages | Sort-Object Order)){
  if(!$needed[$package.Order - 1]){continue}
  Need ($package.DriveId -match '^[A-Za-z0-9_-]+$') "No automatic download is configured for $($package.Name)."
  $filename = if($package.DriveFilename){$package.DriveFilename}else{$package.Filename}
  $digest = if($package.DriveSha256){$package.DriveSha256}else{$package.Sha256}
  $package.Order
  $package.Name
  $filename
  $digest
  "https://drive.usercontent.google.com/download?id=$($package.DriveId)&export=download&confirm=t"
 }
 Need ($lines.Count -gt 0) 'No automatic download is available for the detected prerequisite files.'
 [IO.File]::WriteAllLines($PrerequisitePlan, [string[]]$lines, [Text.UTF8Encoding]::new($false))
}
function Snapshot([string]$path,[string]$name,[string]$folder){
 if(Test-Path -LiteralPath $path){$saved=Relative $folder $name;CopySafe $path $saved;return [ordered]@{Path=$name;Existed=$true;Saved=$saved;Sha=(Hash $saved)}}
 return [ordered]@{Path=$name;Existed=$false;Saved='';Sha=''}
}
function RestoreSnapshots($entries){
 # Validate every snapshot before restoring any path. Conflicting current
 # content is retained beside the external backup before replacement.
 foreach($e in $entries){$null=Relative $game $e.Path;if($e.Existed){Need ((Below $e.Saved $state) -and ((Hash $e.Saved) -eq $e.Sha)) 'Recovery snapshot is damaged.'}}
 foreach($e in $entries){$target=Relative $game $e.Path
  if(Test-Path -LiteralPath $target){CopySafe $target (Relative (Join-Path $state ('RecoveryCopies\'+[guid]::NewGuid().ToString('N'))) $e.Path)}
  if($e.Existed){CopySafe $e.Saved $target}else{if(Test-Path -LiteralPath $target){[IO.File]::Delete($target)}}
 }
}
Add-Type -TypeDefinition @'
using System;using System.Text;using System.Runtime.InteropServices;
public static class G3VRIni {
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode)] public static extern uint GetPrivateProfileString(string s,string k,string d,StringBuilder b,uint n,string p);
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode)] public static extern bool WritePrivateProfileString(string s,string k,string v,string p);
 public static string Read(string p,string s,string k){var b=new StringBuilder(32768);GetPrivateProfileString(s,k,"__G3VR_MISSING__",b,32768,p);return b.ToString();}
}
'@
try {
 Need ([Environment]::Is64BitOperatingSystem) 'Gothic 3 VR requires 64-bit Windows.'
 foreach($p in @('Gothic3','Gothic3VRLauncher','Gothic3VRMetaHost64','UnionStarter','Gothic3VRUnionGuard')){Need (!(Get-Process -Name $p -ErrorAction SilentlyContinue)) "Close $p before continuing."}
 $game=Full $GameDirectory;$stateBase=Full $StateDirectory;NoLinks $game;NoLinks $stateBase
 Need (Test-Path -LiteralPath (Join-Path $game 'Gothic3.exe') -PathType Leaf) 'Select the folder containing Gothic3.exe.'
 Need (!(Below $stateBase $game) -and $stateBase -ne $game) 'Backups must stay outside the game resource tree.'
 $sha=[Security.Cryptography.SHA256]::Create();try{$key=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($game.ToLowerInvariant())))).Replace('-','').Substring(0,24)}finally{$sha.Dispose()}
 $state=Join-Path $stateBase $key;$journalFile=Join-Path $state 'install.json';$pendingFile=Join-Path $state 'pending.json'
 if($Mode -notin @('Check','CheckUninstall')){[IO.Directory]::CreateDirectory($state)|Out-Null;NoLinks $state;$transactionLock=[IO.File]::Open((Join-Path $state 'transaction.lock'),[IO.FileMode]::OpenOrCreate,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)}
 if(Test-Path -LiteralPath $pendingFile){
  $pending=Get-Content -LiteralPath $pendingFile -Raw -Encoding UTF8|ConvertFrom-Json;Need ($pending.Game -eq $game) 'Recovery belongs to another game.'
  if($Mode -eq 'Check' -and $PrerequisitePlan -and $pending.Kind -eq 'Beta'){
   NoLinks $state
   $transactionLock=[IO.File]::Open((Join-Path $state 'transaction.lock'),[IO.FileMode]::OpenOrCreate,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
   if(Test-Path -LiteralPath $pendingFile){
    $pending=Get-Content -LiteralPath $pendingFile -Raw -Encoding UTF8|ConvertFrom-Json
    if($pending.Game -eq $game -and $pending.Kind -eq 'Beta'){
     RestoreSnapshots $pending.Entries
     [IO.File]::Delete($pendingFile)
     Note 'Recovered the interrupted Public Beta installation.'
    }
   }
   $transactionLock.Dispose();$transactionLock=$null
  }elseif($Mode -in @('Check','CheckUninstall')){Note 'A pending transaction will be recovered before installation.'}
  else{
   $committed=(Test-Path -LiteralPath $journalFile) -and ((Get-Content -LiteralPath $journalFile -Raw -Encoding UTF8|ConvertFrom-Json).TransactionId -eq $pending.Id)
   if(!$committed){RestoreSnapshots $pending.Entries}
   [IO.File]::Delete($pendingFile);Note 'Recovered the interrupted file transaction.'
  }
 }
 if($Mode -eq 'ApplyBeta'){
  $source=Full $BetaDirectory
  Need (!(Below $source $game) -and $source -ne $game) 'Extract the beta outside the game folder first.'
  NoLinks $source
  Need (Test-Path -LiteralPath (Relative $source 'scripts\Script_Game.dll') -PathType Leaf) 'Unexpected Public Beta archive layout.'
  $paths=@(Get-ChildItem -LiteralPath $source -Recurse -File -Force | ForEach-Object {$_.FullName.Substring($source.Length + 1)})
  $session=Join-Path $state ([guid]::NewGuid().ToString('N'))
  $snapshots=@(foreach($name in $paths){Snapshot (Relative $game $name) $name (Join-Path $session 'rollback')})
  $id=[guid]::NewGuid().ToString('N')
  JsonSave $pendingFile ([ordered]@{Game=$game;Id=$id;Kind='Beta';Entries=$snapshots})
  try{
   foreach($name in $paths){CopySafe (Relative $source $name) (Relative $game $name)}
   [IO.File]::Delete($pendingFile)
  }catch{RestoreSnapshots $snapshots;[IO.File]::Delete($pendingFile);throw}
  Note 'Installed the complete Update Pack 1.05.10 Public Beta.'
 }elseif($Mode -in @('Uninstall','CheckUninstall')){
  Need (Test-Path -LiteralPath $journalFile) 'No installer journal exists for this game.'
  $journal=Get-Content -LiteralPath $journalFile -Raw -Encoding UTF8|ConvertFrom-Json;Need ($journal.Game -eq $game -and $journal.State -eq 'Installed') 'No active VR installation is recorded.'
  $restore=@();foreach($e in $journal.Files){$target=Relative $game $e.Path
   if($e.Keep){Note "Kept configuration: $($e.Path)";continue}
   if(!(Test-Path -LiteralPath $target)){continue}
   if((Hash $target) -ne $e.InstalledSha){Note "Kept modified file: $($e.Path)";continue}
   if($e.Original.Existed){Need ((Below $e.Original.Saved $state) -and ((Hash $e.Original.Saved) -eq $e.Original.Sha)) 'Original backup is damaged.'}
   $restore+=@($e)
  }
  if($Mode -eq 'CheckUninstall'){Note 'Uninstall prerequisites and original backups validated.';if($ReportFile){[IO.File]::WriteAllText($ReportFile,($script:Events -join [Environment]::NewLine))};$script:Events|Write-Output;exit 0}
  $session=Join-Path $state ([guid]::NewGuid().ToString('N'));[IO.Directory]::CreateDirectory($session)|Out-Null
  $paths=@($restore.Path)+@($journal.IniKeys.Path)|Where-Object {$_}|Select-Object -Unique
  $snapshots=@();foreach($name in $paths){$snapshots+=Snapshot (Relative $game $name) $name (Join-Path $session 'rollback')}
  $id=[guid]::NewGuid().ToString('N');JsonSave $pendingFile ([ordered]@{Game=$game;Id=$id;Entries=$snapshots});$committed=$false
  try{
   foreach($e in $restore){$target=Relative $game $e.Path;if($e.Original.Existed){CopySafe $e.Original.Saved $target}else{[IO.File]::Delete($target)}}
   foreach($e in $journal.IniKeys){$target=Relative $game $e.Path;if([G3VRIni]::Read($target,$e.Section,$e.Key) -eq $e.InstalledValue){$value=$e.OriginalValue;if($value -eq '__G3VR_MISSING__'){$value=[NullString]::Value};Need ([G3VRIni]::WritePrivateProfileString($e.Section,$e.Key,$value,$target)) 'Could not restore the original Quick Loot setting.'}}
   $journal.State='Uninstalled';$journal|Add-Member -NotePropertyName TransactionId -NotePropertyValue $id -Force;JsonSave $journalFile $journal;$committed=$true;[IO.File]::Delete($pendingFile)
   Note 'VR files removed or restored. Saved settings, modified files and external backups were retained.'
  }catch{if(!$committed){RestoreSnapshots $snapshots;[IO.File]::Delete($pendingFile)};throw}
 }else{
  $manifest=Get-Content -LiteralPath $ManifestFile -Raw -Encoding UTF8|ConvertFrom-Json
  Need ($manifest.Format -eq 1) 'Unsupported payload manifest.'
  $selected=@('core')+@($Components.Split(',')|Where-Object {$_ -and $_ -ne 'core'})
  foreach($c in $selected){Need ($c -in @('core','graphics','branding','music')) 'Unknown installer component.'}
  $missing=@(foreach($r in $manifest.Required){
   $p=Relative $game $r.Path
   if(!(Test-Path -LiteralPath $p -PathType Leaf) -or (Hash $p) -ne $r.Sha){$r}
  })
  if($missing.Count -and ($Mode -ne 'Check' -or !$PrerequisitePlan)){
   throw ("Unsupported prerequisite: "+$missing[0].Path+". "+$missing[0].Help)
  }
  foreach($proxy in @('dxgi.dll','d3d11.dll')){Need (!(Test-Path -LiteralPath (Join-Path $game $proxy))) "Remove the external $proxy / ReShade / dgVoodoo proxy before installing VR."}
  $d3d9=Join-Path $game 'd3d9.dll';if(Test-Path -LiteralPath $d3d9){Need ((Hash $d3d9) -eq $manifest.GraphicsSha) 'Unrecognized d3d9.dll. Remove or disable the external graphics wrapper before installing VR.'}
  $files=@($manifest.Files|Where-Object {$_.Component -in $selected})
  $names=@{};foreach($e in $manifest.Files){$null=Relative $game $e.Path;$base=[IO.Path]::GetFileName($e.Path).ToLowerInvariant();Need (!$names.ContainsKey($base)) 'Duplicate payload resource basename.';$names[$base]=$e.Path}
  # Gothic treats duplicate DLL/INI basenames in different folders as fatal.
  $leaves=@{};foreach($e in $files){$leaves[[IO.Path]::GetFileName($e.Path).ToLowerInvariant()]=Full (Join-Path $game $e.Path)}
  foreach($f in Get-ChildItem -LiteralPath $game -Recurse -File -Force){$base=$f.Name.ToLowerInvariant();if($leaves.ContainsKey($base)){Need ((Full $f.FullName) -eq $leaves[$base]) "Duplicate resource found: $($f.FullName). Move old VR backups outside the game folder."}}
  foreach($name in (@($files.Path)+@('Gothic3VRLauncher.ini','Ini\ge3.ini'))){$p=Relative $game $name;if(Test-Path -LiteralPath $p){Need (((Get-Item -LiteralPath $p).Attributes -band [IO.FileAttributes]::ReadOnly) -eq 0) "Read-only destination: $p"}}
  Need ((Hash $PayloadFile) -eq $manifest.PayloadSha) 'Payload archive checksum mismatch.'
  if($missing.Count){
   Need (!(Test-Path -LiteralPath $pendingFile)) 'A pending VR file transaction needs recovery before native patches can be installed.'
   WritePrerequisitePlan $manifest.PrerequisitePackages $missing
   exit 2
  }
  Note 'Validated CP 1.75.14, Update Pack 1.05.10, PUP 1.1.1, payload paths and resource names.'
  if($Mode -eq 'Install'){
   [IO.Directory]::CreateDirectory($state)|Out-Null;NoLinks $state
   $session=Join-Path $state ([guid]::NewGuid().ToString('N'));[IO.Directory]::CreateDirectory($session)|Out-Null
   if((Test-Path -LiteralPath $journalFile) -and (Get-Content -LiteralPath $journalFile -Raw -Encoding UTF8|ConvertFrom-Json).State -eq 'Installed'){$journal=Get-Content -LiteralPath $journalFile -Raw -Encoding UTF8|ConvertFrom-Json}
   else{$journal=[pscustomobject]@{Game=$game;State='New';Version=$manifest.Version;Files=@();IniKeys=@()}}
   $plan=@();foreach($e in $files){$target=Relative $game $e.Path;if($e.Policy -eq 'if-missing' -and (Test-Path -LiteralPath $target)){Note "Preserved existing settings: $($e.Path)";continue};$plan+=$e}
   Add-Type -AssemblyName System.IO.Compression.FileSystem
   $archive=[IO.Compression.ZipFile]::OpenRead($PayloadFile);$staged=Join-Path $session 'staged'
   try{
    Need ($archive.Entries.Count -eq $manifest.Files.Count) 'Unexpected archive contents.'
    $seen=@{};foreach($entry in $archive.Entries){$rel=$entry.FullName.Replace('/','\');$match=@($manifest.Files|Where-Object {$_.Path -eq $rel});Need ($match.Count -eq 1 -and !$seen.ContainsKey($rel.ToLowerInvariant())) 'Unknown or duplicate archive entry.';$seen[$rel.ToLowerInvariant()]=$true
     $out=Relative $staged $rel;[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($out))|Out-Null;$input=$entry.Open();$output=[IO.File]::Create($out);try{$input.CopyTo($output)}finally{$output.Dispose();$input.Dispose()};Need ((Hash $out) -eq $match[0].Sha) 'Payload file checksum mismatch.'
    }
   }finally{$archive.Dispose()}
   $launcher='Gothic3VRLauncher.ini';$quick='Ini\ge3.ini';$quickFile=Relative $game $quick
   Need (Test-Path -LiteralPath $quickFile) 'Ini\ge3.ini is missing.'
   $mutating=@($plan.Path)+@($launcher,$quick)|Where-Object {$_}|Select-Object -Unique
   $snapshots=@();foreach($name in $mutating){$snapshots+=Snapshot (Relative $game $name) $name (Join-Path $session 'rollback')}
   $id=[guid]::NewGuid().ToString('N');JsonSave $pendingFile ([ordered]@{Game=$game;Id=$id;Entries=$snapshots});$committed=$false
   try{
    foreach($e in $plan){$target=Relative $game $e.Path;$existing=@($journal.Files|Where-Object {$_.Path -eq $e.Path})
     if(!$existing.Count){$original=Snapshot $target $e.Path (Join-Path $session 'original');$record=[pscustomobject]@{Path=$e.Path;Original=$original;InstalledSha=$e.Sha;Keep=($e.Policy -eq 'if-missing')};$journal.Files+=@($record)}else{$existing[0].InstalledSha=$e.Sha}
     CopySafe (Relative $staged $e.Path) $target;Need ((Hash $target) -eq $e.Sha) "Installed checksum mismatch: $($e.Path)"
    }
    $launcherFile=Relative $game $launcher;if(!(Test-Path -LiteralPath $launcherFile)){[IO.File]::WriteAllText($launcherFile,"[Launcher]`r`n",[Text.Encoding]::Unicode)}
    Need ([G3VRIni]::WritePrivateProfileString('Launcher','GameExecutable',(Join-Path $game 'Gothic3.exe'),$launcherFile)) 'Could not save the selected game path.'
    $provider=if(Test-Path -LiteralPath (Join-Path $game 'scripts\Script_QuickLoot.dll')){'false'}else{'true'}
    $keyEntry=@($journal.IniKeys|Where-Object {$_.Path -eq $quick})
    if(!$keyEntry.Count){$journal.IniKeys+=@([pscustomobject]@{Path=$quick;Section='Game';Key='QuickLoot';OriginalValue=[G3VRIni]::Read($quickFile,'Game','QuickLoot');InstalledValue=$provider})}else{$keyEntry[0].InstalledValue=$provider}
    Need ([G3VRIni]::WritePrivateProfileString('Game','QuickLoot',$provider,$quickFile)) 'Could not configure Quick Loot. Check ge3.ini permissions.'
    $journal.Version=$manifest.Version;$journal.State='Installed';$journal|Add-Member -NotePropertyName TransactionId -NotePropertyValue $id -Force;JsonSave $journalFile $journal;$committed=$true;[IO.File]::Delete($pendingFile)
    Note "Installed Gothic 3 VR $($manifest.Version). Configuration was created only where missing."
   }catch{if(!$committed){RestoreSnapshots $snapshots;[IO.File]::Delete($pendingFile)};throw}
  }
 }
 if($transactionLock){$transactionLock.Dispose();$transactionLock=$null}
 if($ReportFile){[IO.File]::WriteAllText($ReportFile,($script:Events -join [Environment]::NewLine),[Text.UTF8Encoding]::new($false))}
 $script:Events|Write-Output;exit 0
}catch{
 if($transactionLock){$transactionLock.Dispose()};$message=$_.Exception.Message;if($ReportFile){[IO.File]::WriteAllText($ReportFile,$message,[Text.UTF8Encoding]::new($false))};Write-Output $message;exit 1
}
