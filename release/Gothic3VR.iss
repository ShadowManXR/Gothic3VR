#ifndef PackageDir
  #error PackageDir must point to a verified release staging directory
#endif
#define Version "1.1.6-beta"

[Setup]
AppId={{872AAFD4-B6F2-4BE7-812F-2D5B19A22425}
AppName=Gothic 3 VR
AppVersion=1.1.6 beta
VersionInfoVersion=1.1.6.0
AppPublisher=PeCOS2k5
AppPublisherURL=https://github.com/PeCOS2k5/Gothic3VR
AppSupportURL=https://github.com/PeCOS2k5/Gothic3VR/issues
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
ArchiveExtraction=basic
SetupMutex=Gothic3VRSetup
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
Source: "{#PackageDir}\AUTHORS.md"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
Source: "{#PackageDir}\BRANDING.md"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
Source: "{#PackageDir}\VERIFY-RELEASES.md"; DestDir: "{localappdata}\Gothic3VR\Setup"; Flags: ignoreversion
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
  PatchDownloads: TDownloadWizardPage;

procedure InitializeWizard();
begin
  PatchDownloads := CreateDownloadPage('Download Gothic 3 patches',
    'Downloading and verifying the required game patches.', nil);
end;

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

function RunDeployment(Mode, Helper: String; var ErrorText: String): Integer;
var Code: Integer; Args, Report, Content: String; Raw: AnsiString;
begin
  Report := ExpandConstant('{tmp}\Gothic3VR-' + Mode + '.txt');
  Args := '-NoProfile -NonInteractive -ExecutionPolicy Bypass -File ' + Quoted(Helper) +
    ' -Mode ' + Mode + ' -GameDirectory ' + Quoted(ExpandConstant('{app}')) +
    ' -ReportFile ' + Quoted(Report);
  if (Mode = 'Check') or (Mode = 'Install') then
    Args := Args + ' -ManifestFile ' + Quoted(ExpandConstant('{tmp}\G3VR.Payload.json')) +
      ' -PayloadFile ' + Quoted(ExpandConstant('{tmp}\G3VR.Payload.zip')) +
      ' -Components ' + Quoted(WizardSelectedComponents(False));
  if Mode = 'Check' then
    Args := Args + ' -PrerequisitePlan ' + Quoted(ExpandConstant('{tmp}\NativePatches.txt'));
  if Mode = 'ApplyBeta' then
    Args := Args + ' -BetaDirectory ' + Quoted(ExpandConstant('{tmp}\Beta\UpdatePack v1.05.10 PublicBeta'));
  Code := -1;
  Log('Deployment helper: ' + Helper + ' / mode ' + Mode);
  DeleteFile(Report);
  if Exec(ExpandConstant('{sys}\WindowsPowerShell\v1.0\powershell.exe'), Args, '', SW_HIDE,
    ewWaitUntilTerminated, Code) then
    Result := Code
  else
    Result := -1;
  Log('Deployment result code: ' + IntToStr(Code));
  Content := '';
  if LoadStringFromFile(Report, Raw) then Content := UTF8Decode(Raw);
  Log(Mode + ': ' + Content);
  if Result <> 0 then begin
    ErrorText := Content;
    if ErrorText = '' then ErrorText := 'The deployment helper could not run. Check Windows PowerShell and folder permissions.';
  end;
end;

function InstallPrerequisites(Helper: String): String;
var
  Plan: TArrayOfString;
  I, Code: Integer;
  Names, Game, Patch, Args, ErrorText: String;
begin
  if not LoadStringsFromFile(ExpandConstant('{tmp}\NativePatches.txt'), Plan) or
    (GetArrayLength(Plan) = 0) or (GetArrayLength(Plan) mod 5 <> 0) then begin
    Result := 'Setup could not read the required game patch list.';
    Exit;
  end;
  Names := '';
  PatchDownloads.Clear;
  I := 0;
  while I < GetArrayLength(Plan) do begin
    Names := Names + #13#10 + '  ' + Plan[I + 1];
    PatchDownloads.Add(Plan[I + 4], Plan[I + 2], Plan[I + 3]);
    I := I + 5;
  end;
  Game := ExpandConstant('{app}');
  Result := 'Install these required game patches before continuing:' + Names;
  if WizardSilent then Exit;
  if MsgBox('Setup needs to install or reapply these game patches:' + Names + #13#10#13#10 +
    'Download from Google Drive and install into ' + Game + '?',
    mbConfirmation, MB_YESNO or MB_DEFBUTTON2) <> IDYES then Exit;

  try
    PatchDownloads.Show;
    try
      PatchDownloads.Download;
    finally
      PatchDownloads.Hide;
    end;
    I := 0;
    while I < GetArrayLength(Plan) do begin
      Patch := ExpandConstant('{tmp}\') + Plan[I + 2];
      if Plan[I] = '3' then begin
        ExtractArchive(Patch, ExpandConstant('{tmp}\Beta'), '', True, nil);
        if RunDeployment('ApplyBeta', Helper, ErrorText) <> 0 then
          RaiseException(ErrorText);
      end else begin
        if Plan[I] = '2' then
          { NSIS takes the unquoted /D argument last, even for paths with spaces. }
          Args := '/S /D=' + Game
        else
          Args := '/SP- /SILENT /NORESTART /RESTARTEXITCODE=3010 /DIR=' + Quoted(Game);
        if not ShellExec('', Patch, Args, Game, SW_SHOWNORMAL, ewWaitUntilTerminated, Code) then
          RaiseException('Could not launch ' + Plan[I + 1] + ': ' + SysErrorMessage(Code));
        if Code = 3010 then
          RaiseException('Restart Windows to finish ' + Plan[I + 1] + ', then run Setup again.');
        if Code <> 0 then
          RaiseException(Plan[I + 1] + ' exited with code ' + IntToStr(Code) + '.');
      end;
      I := I + 5;
    end;
    Result := '';
  except
    Result := 'Game patch installation stopped. ' + GetExceptionMessage;
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var ErrorText, Helper: String; Code: Integer;
begin
  Result := '';
  if not PayloadExtracted then begin
    ExtractTemporaryFile('Deploy.ps1');
    ExtractTemporaryFile('G3VR.Payload.json');
    ExtractTemporaryFile('G3VR.Payload.zip');
    PayloadExtracted := True;
  end;
  Helper := ExpandConstant('{tmp}\Deploy.ps1');
  Code := RunDeployment('Check', Helper, ErrorText);
  if Code = 2 then begin
    Result := InstallPrerequisites(Helper);
    if Result <> '' then Exit;
    Code := RunDeployment('Check', Helper, ErrorText);
    if Code = 2 then begin
      Result := 'The required game patch files could not be verified after installation. See the installation guide.';
      Exit;
    end;
  end;
  if Code <> 0 then
    Result := ErrorText;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var ErrorText: String;
begin
  if CurStep = ssPostInstall then
    if RunDeployment('Install', ExpandConstant('{localappdata}\Gothic3VR\Setup\Deploy.ps1'), ErrorText) <> 0 then
      RaiseException('VR installation failed: ' + ErrorText);
end;

function InitializeUninstall(): Boolean;
var ErrorText: String;
begin
  Result := RunDeployment('CheckUninstall', ExpandConstant('{localappdata}\Gothic3VR\Setup\Deploy.ps1'), ErrorText) = 0;
  if not Result then SuppressibleMsgBox(ErrorText, mbError, MB_OK, IDOK);
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var ErrorText: String;
begin
  if CurUninstallStep = usUninstall then
    if RunDeployment('Uninstall', ExpandConstant('{localappdata}\Gothic3VR\Setup\Deploy.ps1'), ErrorText) <> 0 then
      RaiseException('VR removal stopped: ' + ErrorText);
end;
