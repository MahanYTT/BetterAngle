[Setup]
AppId={{DA1CF62B-D790-488B-8A5E-49A8EDBBF1E3}
AppName=BetterAngle Pro
AppVersion={#AppVer}
AppPublisher=Mahan
AppPublisherURL=https://github.com/MahanYTT/BetterAngle
AppSupportURL=https://github.com/MahanYTT/BetterAngle
AppUpdatesURL=https://github.com/MahanYTT/BetterAngle
DefaultDirName={autopf}\BetterAngle Pro
DisableProgramGroupPage=yes
PrivilegesRequired=admin
OutputDir=bin
OutputBaseFilename=BetterAngle_Setup
SetupIconFile=assets\app_icon.ico
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Dirs]
Name: "{localappdata}\BetterAngle"; Permissions: users-modify
Name: "{app}"; Permissions: users-modify

[Files]
Source: "build\Release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\Release\uninstaller.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\BetterAngle Pro"; Filename: "{app}\BetterAngle.exe"
Name: "{autodesktop}\BetterAngle Pro"; Filename: "{app}\BetterAngle.exe"; Tasks: desktopicon
Name: "{autoprograms}\BetterAngle Pro\Uninstall BetterAngle"; Filename: "{app}\uninstaller.exe"

[Run]
; Install Visual C++ Runtime silently before launching the app.
; Required on clean systems without Visual Studio installed.
Filename: "{app}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Visual C++ Runtime..."; Flags: waituntilterminated runhidden
Filename: "{app}\BetterAngle.exe"; Description: "{cm:LaunchProgram,BetterAngle Pro}"; Flags: nowait postinstall skipifsilent
