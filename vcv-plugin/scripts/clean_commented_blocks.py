#!/usr/bin/env python3
"""
Script to remove commented blocks from NtEmu.cpp that have been moved to other files.

This script identifies and removes large commented blocks that are marked as:
- MOVED_TO_FILE: ... - START/END
- EXTRACTED TO ...
- Other patterns of moved/extracted code

Usage: python3 scripts/clean_commented_blocks.py [--dry-run] [--backup]
"""

import argparse
import re
import sys
from pathlib import Path
import shutil
from typing import List, Tuple

def find_commented_blocks(content: str) -> List[Tuple[int, int, str]]:
    """
    Find commented blocks that should be removed.
    Returns list of (start_line, end_line, description) tuples.
    """
    lines = content.split('\n')
    blocks_to_remove = []
    
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        
        # Pattern 1: /* MOVED_TO_FILE: ... - START */ ... /* MOVED_TO_FILE: ... - END */
        if re.match(r'/\*\s*MOVED_TO_FILE:.*- START\s*\*/', line):
            start_line = i
            # Find the corresponding END marker
            j = i + 1
            while j < len(lines):
                end_line_content = lines[j].strip()
                if re.match(r'/\*\s*MOVED_TO_FILE:.*- END\s*\*/', end_line_content):
                    blocks_to_remove.append((start_line, j, f"MOVED_TO_FILE block (lines {start_line+1}-{j+1})"))
                    i = j  # Skip to end of this block
                    break
                j += 1
        
        # Pattern 2: /* MOVED_TO_FILE: ... - START */ followed by content until */
        elif re.match(r'/\*\s*MOVED_TO_FILE:.*- START\s*\*/', line):
            start_line = i
            # Look for end of comment block
            j = i + 1
            while j < len(lines):
                if '*/' in lines[j]:
                    # Check if next line is the END marker
                    if j + 1 < len(lines) and re.match(r'/\*\s*MOVED_TO_FILE:.*- END\s*\*/', lines[j + 1].strip()):
                        blocks_to_remove.append((start_line, j + 1, f"MOVED_TO_FILE block (lines {start_line+1}-{j+2})"))
                        i = j + 1
                        break
                j += 1
        
        # Pattern 3: // EXTRACTED TO ... followed by commented code block
        elif re.match(r'//\s*EXTRACTED TO.*', line):
            start_line = i
            # Look for commented code block that follows
            j = i + 1
            found_comment_start = False
            while j < len(lines):
                stripped = lines[j].strip()
                if stripped.startswith('/*') and not found_comment_start:
                    found_comment_start = True
                elif stripped.endswith('*/') and found_comment_start:
                    blocks_to_remove.append((start_line, j, f"EXTRACTED block (lines {start_line+1}-{j+1})"))
                    i = j
                    break
                elif stripped.startswith('//') and 'END EXTRACTED' in stripped:
                    blocks_to_remove.append((start_line, j, f"EXTRACTED block (lines {start_line+1}-{j+1})"))
                    i = j
                    break
                j += 1
        
        i += 1
    
    return blocks_to_remove

def remove_blocks(content: str, blocks: List[Tuple[int, int, str]]) -> str:
    """Remove the specified blocks from content."""
    lines = content.split('\n')
    
    # Sort blocks by start line in reverse order to maintain line numbers
    blocks_sorted = sorted(blocks, key=lambda x: x[0], reverse=True)
    
    for start_line, end_line, description in blocks_sorted:
        print(f"Removing {description}")
        # Remove lines from start_line to end_line (inclusive)
        del lines[start_line:end_line + 1]
        
        # Add a replacement comment
        replacement = f"// {description.split('(')[0].strip()} - code moved to separate files"
        lines.insert(start_line, replacement)
    
    return '\n'.join(lines)

def main():
    parser = argparse.ArgumentParser(description='Remove commented blocks from NtEmu.cpp')
    parser.add_argument('--dry-run', action='store_true', 
                       help='Show what would be removed without making changes')
    parser.add_argument('--backup', action='store_true',
                       help='Create a backup of the original file')
    parser.add_argument('--file', default='src/NtEmu.cpp',
                       help='Path to the file to clean (default: src/NtEmu.cpp)')
    
    args = parser.parse_args()
    
    file_path = Path(args.file)
    if not file_path.exists():
        print(f"Error: File {file_path} does not exist")
        return 1
    
    # Read the file
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"Error reading file: {e}")
        return 1
    
    # Find blocks to remove
    blocks = find_commented_blocks(content)
    
    if not blocks:
        print("No commented blocks found to remove.")
        return 0
    
    print(f"Found {len(blocks)} commented blocks to remove:")
    for start, end, desc in blocks:
        print(f"  - {desc}")
    
    if args.dry_run:
        print("\nDry run mode - no changes made.")
        return 0
    
    # Create backup if requested
    if args.backup:
        backup_path = file_path.with_suffix('.cpp.backup')
        shutil.copy2(file_path, backup_path)
        print(f"Created backup: {backup_path}")
    
    # Remove blocks
    cleaned_content = remove_blocks(content, blocks)
    
    # Write the cleaned file
    try:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(cleaned_content)
        
        original_lines = len(content.split('\n'))
        cleaned_lines = len(cleaned_content.split('\n'))
        lines_removed = original_lines - cleaned_lines
        
        print(f"\nSuccessfully cleaned {file_path}")
        print(f"Lines removed: {lines_removed}")
        print(f"Original: {original_lines} lines, Cleaned: {cleaned_lines} lines")
        
    except Exception as e:
        print(f"Error writing file: {e}")
        return 1
    
    return 0

if __name__ == '__main__':
    sys.exit(main())