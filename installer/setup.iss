; Inno Setup Script for QuickInput IME
[Setup]
AppName=QuickInput 智能五笔输入法
AppVersion=1.0.0
DefaultDirName={pf}\QuickInput
DefaultGroupName=QuickInput
OutputBaseFilename=QuickInput_Setup
Compression=lzma
SolidCompression=yes
PrivilegesRequired=lowest
ArchitecturesInstallIn64BitMode=x64
UninstallDisplayIcon={app}\QuickInput.exe

[Files]
; 主程序文件
Source: "bin\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs
Source: "models\*"; DestDir: "{app}\models"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "resources\*"; DestDir: "{app}\resources"; Flags: ignoreversion recursesubdirs
Source: "README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "LICENSE"; DestDir: "{app}"; Flags: ignoreversion

; 卸载程序
Source: "uninstall.exe"; DestDir: "{app}"; Flags: deleteafterinstall

[Icons]
Name: "{group}\QuickInput 配置面板"; Filename: "{app}\config_panel.exe"
Name: "{group}\卸载 QuickInput"; Filename: "{uninstallexe}"

[Registry]
; 注册输入法COM服务器
Root: HKCR; Subkey: "CLSID\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}"; ValueType: string; ValueName: ""; ValueData: "QuickInput Text Service"; Flags: uninsdeletekey
Root: HKCR; Subkey: "CLSID\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}\LocalServer32"; ValueType: string; ValueName: ""; ValueData: "{app}\quickinput_ime.dll"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Microsoft\CTF\SystemShared\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}"; ValueType: dword; ValueName: "ThreadMgrEx"; ValueData: "1"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Microsoft\CTF\SystemShared\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}"; ValueType: string; ValueName: "TextService"; ValueData: "{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}"; Flags: uninsdeletekey

; 注册表项
Root: HKCU; Subkey: "SOFTWARE\QuickInput"; Flags: uninsdeletekey
Root: HKCU; Subkey: "SOFTWARE\QuickInput\Config"; Flags: uninsdeletekey

[Run]
; 注册输入法
Filename: "{app}\register_ime.bat"; Parameters: "/silent"; StatusMsg: "正在注册输入法..."; Flags: runhidden waituntilterminated
Filename: "{sys}\regsvr32.exe"; Parameters: "/s ""{app}\quickinput_ime.dll"""; StatusMsg: "注册COM组件..."; Flags: runhidden waituntilterminated

[UninstallDelete]
; 删除用户数据
Type: filesandordirs; Name: "{userappdata}\QuickInput"
Type: dirifempty; Name: "{userappdata}"

[UninstallRun]
; 注销输入法
Filename: "{sys}\regsvr32.exe"; Parameters: "/u /s ""{app}\quickinput_ie.dll"""; StatusMsg: "注销COM组件..."; Flags: runhidden
Filename: "{app}\unregister_ime.bat"; Parameters: "/silent"; StatusMsg: "注销输入法..."; Flags: runhidden

[Messages]
WelcomeLabel1=欢迎使用 QuickInput 智能五笔输入法
WelcomeLabel2=此向导将帮助您在计算机上安装 QuickInput 输入法。
ReadyLabel1=现在将安装 QuickInput 智能五笔输入法。
ReadyLabel2=单击“下一步”继续。
FinishedHeadingLabel=完成 QuickInput 安装向导
FinishedLabelNoIcons=安装 QuickInput 已完成。
FinishedLabel=安装 QuickInput 已完成，已创建桌面图标。
ClickNext=单击“下一步”继续，或单击“取消”退出。
BeveledLabel=QuickInput 智能输入法
LicenseLabel=请阅读下面的许可协议：
InfoBeforeLabel=在继续之前，请查看以下信息：
InfoAfterLabel=安装完成后，请查看以下信息：
WizardFileLabel=文件:
PageFileInfoLabel=详细信息:
LangCode=2052
LanguageName=简体中文