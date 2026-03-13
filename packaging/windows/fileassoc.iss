[Setup]
ChangesAssociations=yes

[Tasks]
Name: "fileassoc"; Description: "File associations:"; GroupDescription: "Associate Amiga file types with Amiberry:"; Flags: unchecked
Name: "fileassoc\uae"; Description: ".uae - Amiberry Configuration"; Flags: unchecked
Name: "fileassoc\adf"; Description: ".adf - Amiga Disk Image"; Flags: unchecked
Name: "fileassoc\ipf"; Description: ".ipf - IPF Disk Image"; Flags: unchecked
Name: "fileassoc\dms"; Description: ".dms - DMS Compressed Disk Image"; Flags: unchecked
Name: "fileassoc\lha"; Description: ".lha - WHDLoad Game Archive"; Flags: unchecked
Name: "fileassoc\cue"; Description: ".cue - CD Cue Sheet"; Flags: unchecked
Name: "fileassoc\uss"; Description: ".uss - UAE Savestate"; Flags: unchecked

[Registry]
; .uae - Amiberry Configuration
Root: HKA; Subkey: "Software\Classes\.uae\OpenWithProgids"; ValueType: string; ValueName: "Amiberry.uae"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassoc\uae
Root: HKA; Subkey: "Software\Classes\Amiberry.uae"; ValueType: string; ValueName: ""; ValueData: "Amiberry Configuration"; Flags: uninsdeletekey; Tasks: fileassoc\uae
Root: HKA; Subkey: "Software\Classes\Amiberry.uae\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\Amiberry.exe,0"; Tasks: fileassoc\uae
Root: HKA; Subkey: "Software\Classes\Amiberry.uae\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\Amiberry.exe"" ""%1"""; Tasks: fileassoc\uae

; .adf - Amiga Disk Image
Root: HKA; Subkey: "Software\Classes\.adf\OpenWithProgids"; ValueType: string; ValueName: "Amiberry.adf"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassoc\adf
Root: HKA; Subkey: "Software\Classes\Amiberry.adf"; ValueType: string; ValueName: ""; ValueData: "Amiga Disk Image"; Flags: uninsdeletekey; Tasks: fileassoc\adf
Root: HKA; Subkey: "Software\Classes\Amiberry.adf\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\Amiberry.exe,0"; Tasks: fileassoc\adf
Root: HKA; Subkey: "Software\Classes\Amiberry.adf\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\Amiberry.exe"" ""%1"""; Tasks: fileassoc\adf

; .ipf - IPF Disk Image
Root: HKA; Subkey: "Software\Classes\.ipf\OpenWithProgids"; ValueType: string; ValueName: "Amiberry.ipf"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassoc\ipf
Root: HKA; Subkey: "Software\Classes\Amiberry.ipf"; ValueType: string; ValueName: ""; ValueData: "IPF Disk Image"; Flags: uninsdeletekey; Tasks: fileassoc\ipf
Root: HKA; Subkey: "Software\Classes\Amiberry.ipf\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\Amiberry.exe,0"; Tasks: fileassoc\ipf
Root: HKA; Subkey: "Software\Classes\Amiberry.ipf\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\Amiberry.exe"" ""%1"""; Tasks: fileassoc\ipf

; .dms - DMS Compressed Disk Image
Root: HKA; Subkey: "Software\Classes\.dms\OpenWithProgids"; ValueType: string; ValueName: "Amiberry.dms"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassoc\dms
Root: HKA; Subkey: "Software\Classes\Amiberry.dms"; ValueType: string; ValueName: ""; ValueData: "DMS Compressed Disk Image"; Flags: uninsdeletekey; Tasks: fileassoc\dms
Root: HKA; Subkey: "Software\Classes\Amiberry.dms\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\Amiberry.exe,0"; Tasks: fileassoc\dms
Root: HKA; Subkey: "Software\Classes\Amiberry.dms\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\Amiberry.exe"" ""%1"""; Tasks: fileassoc\dms

; .lha - WHDLoad Game Archive
Root: HKA; Subkey: "Software\Classes\.lha\OpenWithProgids"; ValueType: string; ValueName: "Amiberry.lha"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassoc\lha
Root: HKA; Subkey: "Software\Classes\Amiberry.lha"; ValueType: string; ValueName: ""; ValueData: "WHDLoad Game Archive"; Flags: uninsdeletekey; Tasks: fileassoc\lha
Root: HKA; Subkey: "Software\Classes\Amiberry.lha\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\Amiberry.exe,0"; Tasks: fileassoc\lha
Root: HKA; Subkey: "Software\Classes\Amiberry.lha\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\Amiberry.exe"" ""%1"""; Tasks: fileassoc\lha

; .cue - CD Cue Sheet
Root: HKA; Subkey: "Software\Classes\.cue\OpenWithProgids"; ValueType: string; ValueName: "Amiberry.cue"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassoc\cue
Root: HKA; Subkey: "Software\Classes\Amiberry.cue"; ValueType: string; ValueName: ""; ValueData: "CD Cue Sheet"; Flags: uninsdeletekey; Tasks: fileassoc\cue
Root: HKA; Subkey: "Software\Classes\Amiberry.cue\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\Amiberry.exe,0"; Tasks: fileassoc\cue
Root: HKA; Subkey: "Software\Classes\Amiberry.cue\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\Amiberry.exe"" ""%1"""; Tasks: fileassoc\cue

; .uss - UAE Savestate
Root: HKA; Subkey: "Software\Classes\.uss\OpenWithProgids"; ValueType: string; ValueName: "Amiberry.uss"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassoc\uss
Root: HKA; Subkey: "Software\Classes\Amiberry.uss"; ValueType: string; ValueName: ""; ValueData: "UAE Savestate"; Flags: uninsdeletekey; Tasks: fileassoc\uss
Root: HKA; Subkey: "Software\Classes\Amiberry.uss\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\Amiberry.exe,0"; Tasks: fileassoc\uss
Root: HKA; Subkey: "Software\Classes\Amiberry.uss\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\Amiberry.exe"" ""%1"""; Tasks: fileassoc\uss
