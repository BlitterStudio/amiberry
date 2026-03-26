const
  SHCNE_MKDIR = $00000008;
  SHCNE_UPDATEDIR = $00001000;
  SHCNE_ASSOCCHANGED = $08000000;
  SHCNF_IDLIST = $0000;
  SHCNF_PATH = $0005;

procedure SHChangeNotifyPath(wEventId, uFlags: Integer; dwItem1, dwItem2: String);
  external 'SHChangeNotifyW@shell32.dll stdcall';
procedure SHChangeNotifyNil(wEventId, uFlags: Integer; dwItem1, dwItem2: Integer);
  external 'SHChangeNotify@shell32.dll stdcall';

procedure RefreshProgramsShell(GroupDir: String);
begin
  SHChangeNotifyPath(SHCNE_UPDATEDIR, SHCNF_PATH, ExpandConstant('{autoprograms}'), '');
  SHChangeNotifyPath(SHCNE_UPDATEDIR, SHCNF_PATH, GroupDir, '');
  SHChangeNotifyNil(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);
end;

<event('CurStepChanged')>
procedure AmiberryRefreshShellAfterInstall(CurStep: TSetupStep);
var
  GroupDir: String;
begin
  if CurStep <> ssPostInstall then
    exit;

  GroupDir := ExpandConstant('{group}');

  SHChangeNotifyPath(SHCNE_MKDIR, SHCNF_PATH, GroupDir, '');
  RefreshProgramsShell(GroupDir);
end;

<event('CurUninstallStepChanged')>
procedure AmiberryRefreshShellAfterUninstall(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
    RefreshProgramsShell(ExpandConstant('{group}'));
end;
