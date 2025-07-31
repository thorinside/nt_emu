# Build and Maintenance Scripts

This directory contains scripts for building and maintaining the VCV plugin codebase.

## clean_commented_blocks.py

**Purpose**: Automatically remove large commented blocks from source files that have been moved to other files during refactoring.

**Usage**:
```bash
# Dry run - see what would be removed without making changes
python3 scripts/clean_commented_blocks.py --dry-run

# Clean with backup
python3 scripts/clean_commented_blocks.py --backup

# Clean specific file
python3 scripts/clean_commented_blocks.py --file src/SomeFile.cpp
```

**Features**:
- Detects `/* MOVED_TO_FILE: ... - START/END */` blocks
- Detects `// EXTRACTED TO ...` blocks followed by commented code
- Creates backup files when requested
- Shows detailed information about what was removed
- Safe dry-run mode for testing

**Example Output**:
```
Found 2 commented blocks to remove:
  - MOVED_TO_FILE block (lines 52-280)
  - MOVED_TO_FILE block (lines 565-567)
Created backup: src/NtEmu.cpp.backup
Successfully cleaned src/NtEmu.cpp
Lines removed: 230
Original: 2542 lines, Cleaned: 2312 lines
```

This script was used to clean up NtEmu.cpp after Phase 1 refactoring, removing 230 lines of commented code that had been moved to separate modular files.