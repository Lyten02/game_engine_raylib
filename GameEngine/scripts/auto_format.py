#!/usr/bin/env python3

"""
Automatic code formatter for the GameEngine project.
Applies formatting rules based on .clang-format configuration.
Can work with or without clang-format installed.
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path
import argparse
import re

def find_clang_format():
    """Find clang-format executable"""
    # Try to find clang-format
    clang_format = shutil.which('clang-format')
    if clang_format:
        return clang_format
    
    # Try common locations
    common_paths = [
        '/usr/bin/clang-format',
        '/usr/local/bin/clang-format',
        'C:\\Program Files\\LLVM\\bin\\clang-format.exe',
        'C:\\Program Files (x86)\\LLVM\\bin\\clang-format.exe'
    ]
    
    for path in common_paths:
        if os.path.exists(path):
            return path
    
    return None

def apply_basic_formatting(file_path):
    """Apply basic formatting rules when clang-format is not available"""
    try:
        with open(file_path, 'r') as f:
            content = f.read()
        
        original_content = content
        
        # Basic formatting rules based on .clang-format
        # 1. Fix spacing around operators
        content = re.sub(r'(\w)\s*<<\s*', r'\1 << ', content)
        content = re.sub(r'\s*<<\s*(\w)', r' << \1', content)
        content = re.sub(r'(\w)\s*>>\s*', r'\1 >> ', content)
        content = re.sub(r'\s*>>\s*(\w)', r' >> \1', content)
        
        # 2. Fix spacing around parentheses
        content = re.sub(r'(\w)\s*\(\s*\)', r'\1()', content)
        content = re.sub(r'if\s*\(', r'if (', content)
        content = re.sub(r'while\s*\(', r'while (', content)
        content = re.sub(r'for\s*\(', r'for (', content)
        
        # 3. Fix brace placement
        content = re.sub(r'\s*{\s*$', ' {', content, flags=re.MULTILINE)
        
        # 4. Remove trailing whitespace
        content = re.sub(r'[ \t]+$', '', content, flags=re.MULTILINE)
        
        # 5. Ensure newline at end of file
        if content and not content.endswith('\n'):
            content += '\n'
        
        if content != original_content:
            with open(file_path, 'w') as f:
                f.write(content)
            return True
        
        return False
    except Exception as e:
        print(f"Error processing {file_path}: {e}")
        return False

def format_file(file_path, clang_format_path=None):
    """Format a single file"""
    if clang_format_path:
        # Use clang-format
        try:
            result = subprocess.run(
                [clang_format_path, '-i', str(file_path)],
                capture_output=True,
                text=True
            )
            return result.returncode == 0
        except Exception as e:
            print(f"Error running clang-format on {file_path}: {e}")
            return False
    else:
        # Use basic formatting
        return apply_basic_formatting(file_path)

def format_directory(directory, extensions=['.cpp', '.h', '.hpp'], clang_format_path=None):
    """Format all files in directory"""
    directory = Path(directory)
    
    if not directory.exists():
        print(f"Error: Directory {directory} does not exist")
        return False
    
    files_formatted = 0
    files_processed = 0
    
    # Find all source files
    for ext in extensions:
        for file_path in directory.rglob(f'*{ext}'):
            files_processed += 1
            if format_file(file_path, clang_format_path):
                files_formatted += 1
                print(f"Formatted: {file_path}")
    
    print(f"\nProcessed {files_processed} files, formatted {files_formatted}")
    return True

def main():
    parser = argparse.ArgumentParser(description='Auto-format C++ source files')
    parser.add_argument('path', nargs='?', default='src',
                       help='Path to format (default: src)')
    parser.add_argument('--check', action='store_true',
                       help='Check formatting without modifying files')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='Verbose output')
    
    args = parser.parse_args()
    
    # Find clang-format
    clang_format = find_clang_format()
    
    if clang_format:
        print(f"Using clang-format: {clang_format}")
    else:
        print("⚠️  clang-format not found, using basic formatting rules")
        print("For best results, install clang-format")
    
    # Format the specified path
    path = Path(args.path)
    
    if path.is_file():
        # Format single file
        success = format_file(path, clang_format)
        if success:
            print(f"✅ Formatted: {path}")
        else:
            print(f"❌ Failed to format: {path}")
        return 0 if success else 1
    elif path.is_dir():
        # Format directory
        success = format_directory(path, clang_format_path=clang_format)
        return 0 if success else 1
    else:
        print(f"Error: {path} is not a valid file or directory")
        return 1

if __name__ == "__main__":
    sys.exit(main())