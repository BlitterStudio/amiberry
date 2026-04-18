const
  SHCNE_MKDIR = $00000008;
  SHCNE_UPDATEDIR = $00001000;
  SHCNE_ASSOCCHANGED = $08000000;
  SHCNF_IDLIST = $0000;
  SHCNF_PATHW = $0005;

procedure SHChangeNotify(wEventId, uFlags: Integer; dwItem1, dwItem2: String);
  external 'SHChangeNotify@shell32.dll stdcall';

procedure RefreshProgramsShell(GroupDir: String);
begin
  SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATHW, ExpandConstant('{autoprograms}'), '');
  SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATHW, GroupDir, '');
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, '', '');
end;

<event('CurStepChanged')>
procedure AmiberryRefreshShellAfterInstall(CurStep: TSetupStep);
var
  GroupDir: String;
begin
  if CurStep <> ssPostInstall then
    exit;

  GroupDir := ExpandConstant('{group}');

  SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW, GroupDir, '');
  RefreshProgramsShell(GroupDir);
end;

<event('CurUninstallStepChanged')>
procedure AmiberryRefreshShellAfterUninstall(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
    RefreshProgramsShell(ExpandConstant('{group}'));
end;

<event('CurStepChanged')>
procedure AmiberryWritePortableMarker(CurStep: TSetupStep);
var
  MarkerPath: String;
begin
  if CurStep <> ssPostInstall then
    exit;
  if not WizardIsTaskSelected('portable') then
    exit;

  MarkerPath := ExpandConstant('{app}\amiberry.portable');
  SaveStringToFile(MarkerPath, '', False);
end;
