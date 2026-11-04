#ifndef PackageDir
  #error PackageDir must point to a verified release staging directory
#endif
#define Version "1.0.6"

[Setup]
AppId={{872AAFD4-B6F2-4BE7-812F-2D5B19A22425}
AppName=Gothic 3 VR
AppVersion={#Version}
AppPublisher=Gothic 3 VR
AppSupportURL=https://ko-fi.com/pecos2k5
DefaultDirName={code:FindGame}
AppendDefaultDirName=no
DisableDirPage=no
DirExistsWarning=no
DefaultGroupName=Gothic 3 VR
AllowNoIcons=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
MinVersion=10.0
ArchitecturesAllowed=x64compatible
UninstallFilesDir={localappdata}\Gothic3VR\Setup
SetupIconFile=..\tools\assets\launcher.ico
WizardStyle=modern
WizardSizePercent=110
InfoBeforeFile={#PackageDir}\BEFORE-INSTALL.txt
OutputDir={#PackageDir}
OutputBaseFilename=Gothic3VR-{#Version}-Setup
Compression=lzma2
SolidCompression=yes
CloseApplications=no
RestartApplications=no
UninstallDisplayIcon={app}\Gothic3VRLauncher.exe
SetupLogging=yes
ExtraDiskSpaceRequired=150000000

[Languages]
Name: english; MessagesFile: compiler:Default.isl

[Types]
Name: standard; Description: Creator preset (NVIDIA FFR backend, artwork and music)
Name: custom; Description: Custom installation; Flags: iscustom

[Components]
Name: core; Description: VR core, launcher and runtime files (required); Types: standard custom; Flags: fixed
Name: graphics; Description: DXVK / NVIDIA FFR backend (Vulkan 1.4 required); Types: standard
Name: branding; Description: Gothic 3 VR main-menu artwork; Types: standard
Name: music; Description: Custom main-menu music; Types: standard

[Tasks]
Name: desktopicon; Description: Create a desktop shortcut; GroupDescription: Shortcuts:; Flags: unchecked

[Files]
Source: "{#PackageDir}\G3VR.Payload.zip"; Flags: dontcopy
Source: "{#PackageDir}\G3VR.Payload.json"; Flags: dontcopy
Source: "{#PackageDir}\Deploy.ps1"; Flags: dontcopy
Source: "{#PackageDir}\Deploy.ps1"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
Source: "{#PackageDir}\G3VR.Payload.json"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
Source: "{#PackageDir}\Prerequisites.json"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
Source: "{#PackageDir}\START-HERE.html"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
Source: "{#PackageDir}\START-HERE.md"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
Source: "{#PackageDir}\RELEASE-NOTES.md"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
Source: "{#PackageDir}\CONTROLLER-MAPPING.md"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
Source: "{#PackageDir}\CONTROLLER-COMPATIBILITY.md"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
Source: "{#PackageDir}\ITEM-LOCATOR.md"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
Source: "{#PackageDir}\INI-TWEAKS.md"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
Source: "{#PackageDir}\OPTIONAL-MODS-UNION.md"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
Source: "{#PackageDir}\VR-MOD-COMPATIBILITY.md"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
Source: "{#PackageDir}\Gothic3VR-{#Version}-Source.zip"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
Source: "{#PackageDir}\THIRD-PARTY-NOTICES.txt"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
Source: "{#PackageDir}\licenses\*"; DestDir: "{localappdata}\Gothic3VR\Setup\licenses"; Flags: ignoreversion

[Icons]
Name: "{group}\Gothic 3 VR"; Filename: "{app}\Gothic3VRLauncher.exe"; WorkingDir: "{app}"
Name: "{group}\Installation guide"; Filename: "{localappdata}\Gothic3VR\Setup\START-HERE.html"
Name: "{group}\Uninstall Gothic 3 VR"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Gothic 3 VR"; Filename: "{app}\Gothic3VRLauncher.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{localappdata}\Gothic3VR\Setup\START-HERE.html"; Description: Read the installation and first-launch guide; Flags: shellexec postinstall skipifsilent
Filename: "{app}\Gothic3VRLauncher.exe"; WorkingDir: "{app}"; Description: Open Gothic 3 VR settings; Flags: nowait postinstall unchecked skipifsilent

[Code]
var
  PayloadExtracted: Boolean;

function FindGame(Param: String): String;
var Steam: String;
begin
  Result := ExpandConstant('{sd}\Games\Gothic 3');
  if RegQueryStringValue(HKCU, 'Software\Valve\Steam', 'SteamPath', Steam) then
    if FileExists(Steam + '\steamapps\common\Gothic 3\Gothic3.exe') then
      Result := Steam + '\steamapps\common\Gothic 3';
end;

function Quoted(Value: String): String;
begin
  Result := '"' + Value + '"';
end;

function RunDeployment(Mode, Helper: String; var ErrorText: String): Boolean;
var Code: Integer; Args, Report, Content: String; Raw: AnsiString;
begin
  Report := ExpandConstant('{tmp}\Gothic3VR-' + Mode + '.txt');
  Args := '-NoProfile -NonInteractive -ExecutionPolicy Bypass -File ' + Quoted(Helper) +
    ' -Mode ' + Mode + ' -GameDirectory ' + Quoted(ExpandConstant('{app}')) +
    ' -ReportFile ' + Quoted(Report);
  if Mode = 'Check' then
    Args := Args + ' -ManifestFile ' + Quoted(ExpandConstant('{tmp}\G3VR.Payload.json')) +
      ' -PayloadFile ' + Quoted(ExpandConstant('{tmp}\G3VR.Payload.zip')) +
      ' -Components ' + Quoted(WizardSelectedComponents(False));
  if Mode = 'Install' then
    Args := Args + ' -ManifestFile ' + Quoted(ExpandConstant('{tmp}\G3VR.Payload.json')) +
      ' -PayloadFile ' + Quoted(ExpandConstant('{tmp}\G3VR.Payload.zip')) +
      ' -Components ' + Quoted(WizardSelectedComponents(False));
  Code := -1;
  Log('Deployment helper: ' + Helper + ' / mode ' + Mode);
  Result := Exec(ExpandConstant('{sys}\WindowsPowerShell\v1.0\powershell.exe'), Args, '', SW_HIDE, ewWaitUntilTerminated, Code) and (Code = 0);
  Log('Deployment result code: ' + IntToStr(Code));
  Content := '';
  if LoadStringFromFile(Report, Raw) then Content := UTF8Decode(Raw);
  Log(Mode + ': ' + Content);
  if not Result then begin
    ErrorText := Content;
    if ErrorText = '' then ErrorText := 'The deployment helper could not run. Check Windows PowerShell and folder permissions.';
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var ErrorText: String;
begin
  Result := '';
  if not PayloadExtracted then begin
    ExtractTemporaryFile('Deploy.ps1');
    ExtractTemporaryFile('G3VR.Payload.json');
    ExtractTemporaryFile('G3VR.Payload.zip');
    PayloadExtracted := True;
  end;
  if not RunDeployment('Check', ExpandConstant('{tmp}\Deploy.ps1'), ErrorText) then
    Result := ErrorText;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var ErrorText: String;
begin
  if CurStep = ssPostInstall then
    if not RunDeployment('Install', ExpandConstant('{localappdata}\Gothic3VR\Setup\Deploy.ps1'), ErrorText) then
      RaiseException('VR installation failed: ' + ErrorText);
end;

function InitializeUninstall(): Boolean;
var ErrorText: String;
begin
  Result := RunDeployment('CheckUninstall', ExpandConstant('{localappdata}\Gothic3VR\Setup\Deploy.ps1'), ErrorText);
  if not Result then SuppressibleMsgBox(ErrorText, mbError, MB_OK, IDOK);
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var ErrorText: String;
begin
  if CurUninstallStep = usUninstall then
    if not RunDeployment('Uninstall', ExpandConstant('{localappdata}\Gothic3VR\Setup\Deploy.ps1'), ErrorText) then
      RaiseException('VR removal stopped: ' + ErrorText);
end;
