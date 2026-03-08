#!/usr/bin/env python3
"""
Analyze WinUAE commits or code for platform-specific patterns that need attention
when merging to Amiberry.

Note: Most core emulation code (CPU, chipset, etc.) is identical between WinUAE
and Amiberry. This script focuses on platform-specific code that requires adaptation:
- Graphics (Direct3D → OpenGL)
- Audio (WASAPI → SDL2)
- Input (DirectInput → SDL2)
- GUI (Win32 → ImGui)
- File I/O (Windows APIs → POSIX)
- Threading (Win32 → SDL2/pthreads)
"""

import re
import sys
from pathlib import Path
from typing import List, Dict, Set

# Patterns that indicate Windows-specific code
WINDOWS_PATTERNS = {
    'win32_api': [
        r'\bCreateWindow[AW]?\b',
        r'\bRegisterClass[AW]?\b',
        r'\bGetMessage[AW]?\b',
        r'\bDispatchMessage[AW]?\b',
        r'\bDefWindowProc[AW]?\b',
        r'\bGetDC\b',
        r'\bReleaseDC\b',
        r'\bBeginPaint\b',
        r'\bEndPaint\b',
        r'\bInvalidateRect\b',
        r'\bGetSystemMetrics\b',
        r'\bSetWindowPos\b',
    ],
    'win32_input': [
        r'\bGetAsyncKeyState\b',
        r'\bGetKeyState\b',
        r'\bSetCursor\b',
        r'\bShowCursor\b',
    ],
    'directx': [
        r'\bDirect3D',
        r'\bIDirect3D',
        r'\bD3D[0-9]',
        r'\bDirectInput',
        r'\bIDirectInput',
        r'\bDirectSound',  # Legacy, WASAPI is more common now
        r'\bIDirectSound',
        r'\bWASAPI',
        r'\bIAudioClient',
        r'\bIMMDevice',
        r'\bCoCreateInstance.*Audio',
    ],
    'threading': [
        r'\bCreateThread\b',
        r'\bWaitForSingleObject\b',
        r'\bWaitForMultipleObjects\b',
        r'\bCRITICAL_SECTION\b',
        r'\bEnterCriticalSection\b',
        r'\bLeaveCriticalSection\b',
        r'\bCreateEvent[AW]?\b',
        r'\bSetEvent\b',
        r'\bResetEvent\b',
    ],
    'file_system': [
        r'\bCreateFile[AW]?\b',
        r'\bReadFile\b',
        r'\bWriteFile\b',
        r'\bFindFirstFile[AW]?\b',
        r'\bFindNextFile[AW]?\b',
        r'\bGetFileAttributes[AW]?\b',
        r'\bSetFileAttributes[AW]?\b',
        r'\bDeleteFile[AW]?\b',
        r'\bMAX_PATH\b',
        r'\\\\',  # Backslash path separators
    ],
    'registry': [
        r'\bRegOpenKey[AW]?\b',
        r'\bRegQueryValue[AW]?\b',
        r'\bRegSetValue[AW]?\b',
        r'\bRegCloseKey\b',
        r'\bHKEY_',
    ],
    'timing': [
        r'\bQueryPerformanceCounter\b',
        r'\bQueryPerformanceFrequency\b',
        r'\bGetTickCount\b',
        r'\bSleep\b',
    ],
    'gdi': [
        r'\bBitBlt\b',
        r'\bStretchBlt\b',
        r'\bCreateDIBSection\b',
        r'\bSelectObject\b',
        r'\bDeleteObject\b',
        r'\bCreateCompatibleDC\b',
    ],
}

# File patterns that are typically Windows-only
WINDOWS_FILE_PATTERNS = [
    r'.*\.rc$',  # Resource files
    r'.*_win\.cpp$',
    r'.*_win\.h$',
    r'.*win32.*\.(cpp|h)$',
    r'.*windows.*\.(cpp|h)$',
]

# Exclude patterns (files that should be ignored)
EXCLUDE_PATTERNS = [
    r'\.git/',
    r'\.vs/',
    r'build/',
    r'cmake-build-.*/',
]


def should_exclude(filepath: str) -> bool:
    """Check if file should be excluded from analysis."""
    for pattern in EXCLUDE_PATTERNS:
        if re.search(pattern, filepath):
            return True
    return False


def is_windows_file(filepath: str) -> bool:
    """Check if file is Windows-specific by name."""
    for pattern in WINDOWS_FILE_PATTERNS:
        if re.match(pattern, filepath, re.IGNORECASE):
            return True
    return False


def analyze_code(code: str, filepath: str = "") -> Dict[str, List[Dict[str, any]]]:
    """Analyze code for Windows-specific patterns."""
    results = {category: [] for category in WINDOWS_PATTERNS.keys()}
    
    lines = code.split('\n')
    
    for category, patterns in WINDOWS_PATTERNS.items():
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


def format_results(results: Dict[str, List[Dict[str, any]]]) -> str:
    """Format analysis results for display."""
    output = []
    
    total_matches = sum(len(matches) for matches in results.values())
    
    if total_matches == 0:
        return "✓ No Windows-specific patterns detected."
    
    output.append(f"⚠ Found {total_matches} potential Windows-specific code patterns:\n")
    
    for category, matches in results.items():
        if matches:
            output.append(f"\n## {category.upper().replace('_', ' ')} ({len(matches)} matches)")
            
            # Group by file if multiple files
            by_file = {}
            for match in matches:
                file = match.get('file', 'unknown')
                if file not in by_file:
                    by_file[file] = []
                by_file[file].append(match)
            
            for file, file_matches in by_file.items():
                if file:
                    output.append(f"\n  File: {file}")
                
                for match in file_matches[:10]:  # Limit to first 10 per category per file
                    line_num = match['line']
                    text = match['text']
                    matched = match['match']
                    output.append(f"    Line {line_num}: {matched}")
                    output.append(f"      {text}")
                
                if len(file_matches) > 10:
                    output.append(f"    ... and {len(file_matches) - 10} more matches")
    
    return '\n'.join(output)


def analyze_file(filepath: Path) -> Dict[str, List[Dict[str, any]]]:
    """Analyze a single file."""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            code = f.read()
        return analyze_code(code, str(filepath))
    except Exception as e:
        print(f"Error reading {filepath}: {e}", file=sys.stderr)
        return {category: [] for category in WINDOWS_PATTERNS.keys()}


def analyze_directory(directory: Path) -> Dict[str, List[Dict[str, any]]]:
    """Recursively analyze all source files in a directory."""
    combined_results = {category: [] for category in WINDOWS_PATTERNS.keys()}
    
    source_extensions = {'.c', '.cpp', '.cc', '.h', '.hpp'}
    
    for file in directory.rglob('*'):
        if not file.is_file():
            continue
        
        if file.suffix not in source_extensions:
            continue
        
        rel_path = str(file.relative_to(directory))
        
        if should_exclude(rel_path):
            continue
        
        if is_windows_file(rel_path):
            print(f"ℹ Skipping Windows-specific file: {rel_path}", file=sys.stderr)
            continue
        
        results = analyze_file(file)
        
        for category in combined_results:
            combined_results[category].extend(results[category])
    
    return combined_results


def main():
    if len(sys.argv) < 2:
        print("Usage: python analyze_windows_code.py <file_or_directory> [file_or_directory ...]")
        print("\nAnalyzes source code for Windows-specific patterns that need attention")
        print("when merging WinUAE code to Amiberry.")
        sys.exit(1)
    
    all_results = {category: [] for category in WINDOWS_PATTERNS.keys()}
    
    for arg in sys.argv[1:]:
        path = Path(arg)
        
        if not path.exists():
            print(f"Error: Path does not exist: {path}", file=sys.stderr)
            continue
        
        print(f"Analyzing: {path}...", file=sys.stderr)
        
        if path.is_file():
            results = analyze_file(path)
        else:
            results = analyze_directory(path)
        
        for category in all_results:
            all_results[category].extend(results[category])
    
    print("\n" + "="*80)
    print(format_results(all_results))
    print("="*80)


if __name__ == '__main__':
    main()
