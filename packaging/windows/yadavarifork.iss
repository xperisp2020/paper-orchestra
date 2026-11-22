#define MyAppName "Yadavarifork"
#define MyAppVersion "0.1.0"
#define MyAppPublisher "xperisp2020"
#define MyAppExeName "yadavarifork.exe"

[Setup]
AppId={{B2E0B31A-9C87-4A2F-A0D8-7A9C0E5F1142}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={localappdata}\Programs\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
OutputDir=..\..\build\installer
OutputBaseFilename=Yadavarifork-Setup-0.1.0
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\{#MyAppExeName}
SetupLogging=yes
CloseApplications=yes
RestartApplications=no
Uninstallable=yes
VersionInfoVersion=0.1.0.0
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription=Yadavarifork research and proposal assistant
VersionInfoProductName={#MyAppName}

[Dirs]
; Project databases are user data and must survive uninstall.
Name: "{app}\data"; Flags: uninsneveruninstall

[Files]
Source: "..\..\build\Release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; WorkingDir: "{app}"; Flags: postinstall nowait skipifsilent
