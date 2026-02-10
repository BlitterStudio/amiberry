#!/usr/bin/env python3
"""
Identify GUI-related code in WinUAE that may need ImGui adaptation in Amiberry.
"""

import re
import sys
from pathlib import Path
from typing import List, Dict

# Patterns indicating GUI code
GUI_PATTERNS = {
    'dialogs': [
        r'\bDialogBox[AW]?\b',
        r'\bCreateDialog[AW]?\b',
        r'\bEndDialog\b',
        r'\bDLGTEMPLATE\b',
        r'IDD_\w+',  # Dialog resource IDs
    ],
    'controls': [
        r'\bWM_COMMAND\b',
        r'\bWM_NOTIFY\b',
        r'\bBN_CLICKED\b',
        r'\bEN_CHANGE\b',
        r'\bCBN_SELCHANGE\b',
        r'\bLBN_SELCHANGE\b',
        r'\bGetDlgItem\b',
        r'\bSetDlgItemText[AW]?\b',
        r'\bGetDlgItemText[AW]?\b',
        r'\bCheckDlgButton\b',
        r'\bIsDlgButtonChecked\b',
        r'\bSendDlgItemMessage[AW]?\b',
        r'\bSetWindowText[AW]?\b',
        r'\bGetWindowText[AW]?\b',
    ],
    'menus': [
        r'\bCreateMenu\b',
        r'\bCreatePopupMenu\b',
        r'\bAppendMenu[AW]?\b',
        r'\bInsertMenu[AW]?\b',
        r'\bTrackPopupMenu\b',
        r'\bGetMenu\b',
        r'\bSetMenu\b',
        r'IDM_\w+',  # Menu resource IDs
    ],
    'common_controls': [
        r'\bListView_',
        r'\bTreeView_',
        r'\bToolbar_',
        r'\bTab_',
        r'\bSendMessage[AW]?\b',
        r'\bLVM_\w+',  # ListView messages
        r'\bTVM_\w+',  # TreeView messages
    ],
    'property_sheets': [
        r'\bPropertySheet[AW]?\b',
        r'\bCreatePropertySheetPage[AW]?\b',
        r'\bPROPSHEETPAGE[AW]?\b',
        r'\bPSN_\w+',  # PropertySheet notifications
    ],
}

# Control type mappings for ImGui
CONTROL_MAPPINGS = {
    'BUTTON': 'ImGui::Button()',
    'EDIT': 'ImGui::InputText()',
    'STATIC': 'ImGui::Text()',
    'COMBOBOX': 'ImGui::Combo()',
    'LISTBOX': 'ImGui::ListBox()',
    'CHECKBOX': 'ImGui::Checkbox()',
    'RADIOBUTTON': 'ImGui::RadioButton()',
    'GROUPBOX': 'ImGui::BeginGroup() / ImGui::EndGroup()',
}


def analyze_gui_code(code: str, filepath: str = "") -> Dict[str, List[Dict[str, any]]]:
    """Analyze code for GUI patterns."""
    results = {category: [] for category in GUI_PATTERNS.keys()}
    
    lines = code.split('\n')
    
    for category, patterns in GUI_PATTERNS.items():
        for pattern in patterns:
            regex = re.compile(pattern, re.IGNORECASE)
            for line_num, line in enumerate(lines, 1):
                matches = regex.finditer(line)
                for match in matches:
                    results[category].append({
                        'line': line_num,
                        'text': line.strip(),
                        'match': match.group(0),
                        'file': filepath,
                    })
    
    return results


def suggest_imgui_equivalent(win32_control: str) -> str:
    """Suggest ImGui equivalent for Win32 control."""
    control_upper = win32_control.upper()
    
    for win32_type, imgui_func in CONTROL_MAPPINGS.items():
        if win32_type in control_upper:
            return imgui_func
    
    return "Check ImGui documentation"


def format_gui_results(results: Dict[str, List[Dict[str, any]]]) -> str:
    """Format GUI analysis results."""
    output = []
    
    total_matches = sum(len(matches) for matches in results.values())
    
    if total_matches == 0:
        return "✓ No Win32 GUI patterns detected."
    
    output.append(f"⚠ Found {total_matches} Win32 GUI patterns that may need ImGui adaptation:\n")
    output.append("These likely require changes in src/osdep/imgui/ to match functionality.\n")
    
    for category, matches in results.items():
        if matches:
            output.append(f"\n## {category.upper().replace('_', ' ')} ({len(matches)} matches)")
            
            by_file = {}
            for match in matches:
                file = match.get('file', 'unknown')
                if file not in by_file:
                    by_file[file] = []
                by_file[file].append(match)
            
            for file, file_matches in by_file.items():
                if file:
                    output.append(f"\n  File: {file}")
                
                for match in file_matches[:10]:
                    line_num = match['line']
                    text = match['text']
                    matched = match['match']
                    output.append(f"    Line {line_num}: {matched}")
                    output.append(f"      {text}")
                
                if len(file_matches) > 10:
                    output.append(f"    ... and {len(file_matches) - 10} more matches")
    
    output.append("\n## ImGui Implementation Notes")
    output.append("- Amiberry's ImGui code is in: src/osdep/imgui/")
    output.append("- Try to maintain similar layout and functionality to WinUAE")
    output.append("- Common mappings:")
    for win32, imgui in CONTROL_MAPPINGS.items():
        output.append(f"  {win32} → {imgui}")
    
    return '\n'.join(output)


def main():
    if len(sys.argv) < 2:
        print("Usage: python analyze_gui_code.py <file_or_directory> [file_or_directory ...]")
        print("\nIdentifies Win32 GUI code that needs ImGui adaptation in Amiberry.")
        sys.exit(1)
    
    all_results = {category: [] for category in GUI_PATTERNS.keys()}
    
    for arg in sys.argv[1:]:
        path = Path(arg)
        
        if not path.exists():
            print(f"Error: Path does not exist: {path}", file=sys.stderr)
            continue
        
        print(f"Analyzing: {path}...", file=sys.stderr)
        
        if path.is_file():
            try:
                with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                    code = f.read()
                results = analyze_gui_code(code, str(path))
                
                for category in all_results:
                    all_results[category].extend(results[category])
            except Exception as e:
                print(f"Error reading {path}: {e}", file=sys.stderr)
        
        elif path.is_dir():
            source_extensions = {'.c', '.cpp', '.cc', '.h', '.hpp'}
            
            for file in path.rglob('*'):
                if file.is_file() and file.suffix in source_extensions:
                    try:
                        with open(file, 'r', encoding='utf-8', errors='ignore') as f:
                            code = f.read()
                        results = analyze_gui_code(code, str(file.relative_to(path)))
                        
                        for category in all_results:
                            all_results[category].extend(results[category])
                    except Exception as e:
                        print(f"Error reading {file}: {e}", file=sys.stderr)
    
    print("\n" + "="*80)
    print(format_gui_results(all_results))
    print("="*80)


if __name__ == '__main__':
    main()
