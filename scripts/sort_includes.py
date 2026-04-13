#!/usr/bin/env python3
"""
Sort #include directives in .c and .h files.

Order for foo.c:
    #include "path/to/foo.h"   ← primary (matching header), if present

    #include "a_other.h"       ← local includes, sorted
    #include "b_sorted.h"

    #include <system.h>        ← system includes, sorted (if any)

For .h files (no primary):
    local includes sorted, then system includes sorted.
"""

import re
import sys
from pathlib import Path

INCLUDE_RE = re.compile(r'^#include\s+([<"])')
LOCAL_RE   = re.compile(r'^#include\s+"([^"]+)"')
SYS_RE     = re.compile(r'^#include\s+<([^>]+)>')


def classify(line: str):
    """Return ('local', path), ('sys', path), or None."""
    m = LOCAL_RE.match(line.rstrip())
    if m:
        return ('local', m.group(1))
    m = SYS_RE.match(line.rstrip())
    if m:
        return ('sys', m.group(1))
    return None


def find_include_block(lines: list[str]):
    """
    Return (start, end) indices of the contiguous include block
    (includes + blank lines within it).  The block ends at the first
    non-blank non-include line after the first include.
    Returns (None, None) if no includes found.
    """
    start = end = None
    for i, line in enumerate(lines):
        stripped = line.rstrip()
        if INCLUDE_RE.match(stripped):
            if start is None:
                start = i
            end = i
        elif start is not None and stripped:
            # Non-empty, non-include line — block ends here
            break
    return start, end


def sort_includes(filepath: Path) -> bool:
    text  = filepath.read_text()
    lines = text.splitlines(keepends=True)

    stem   = filepath.stem
    is_c   = filepath.suffix == '.c'

    start, end = find_include_block(lines)
    if start is None:
        return False

    block = lines[start:end + 1]
    inc_lines = [l for l in block if INCLUDE_RE.match(l.rstrip())]

    if not inc_lines:
        return False

    primary = None
    locals_ = []
    sys_    = []

    for line in inc_lines:
        kind = classify(line)
        if kind is None:
            continue
        typ, path = kind
        if is_c and typ == 'local' and Path(path).stem == stem:
            primary = line
        elif typ == 'local':
            locals_.append(line)
        else:
            sys_.append(line)

    locals_.sort(key=lambda l: l.lower())
    sys_.sort(key=lambda l: l.lower())

    # Build new block
    new: list[str] = []
    nl = '\n'

    if primary:
        new.append(primary if primary.endswith('\n') else primary + nl)
        if locals_ or sys_:
            new.append(nl)

    new.extend(l if l.endswith('\n') else l + nl for l in locals_)

    if sys_:
        if locals_ or primary:
            new.append(nl)
        new.extend(l if l.endswith('\n') else l + nl for l in sys_)

    # Strip trailing newline on last include to match original style
    if new and new[-1] == nl:
        new.pop()

    # Compare only the include lines (ignore blank lines in old block)
    old_incs = [l for l in block if INCLUDE_RE.match(l.rstrip())]
    new_incs = [l for l in new   if INCLUDE_RE.match(l.rstrip())]
    if old_incs == new_incs:
        return False

    new_lines = lines[:start] + new + lines[end + 1:]
    new_text  = ''.join(new_lines)

    if new_text == text:
        return False

    filepath.write_text(new_text)
    return True


def main():
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('.')

    skip = {'cmake-build', 'build', '.git'}

    changed = []
    skipped = []

    for path in sorted(root.rglob('*')):
        if any(part in skip for part in path.parts):
            continue
        if path.suffix not in ('.c', '.h'):
            continue
        try:
            if sort_includes(path):
                changed.append(path)
                print(f'  sorted  {path.relative_to(root)}')
        except Exception as e:
            skipped.append((path, e))
            print(f'  SKIP    {path.relative_to(root)}: {e}')

    print(f'\n{len(changed)} file(s) changed, {len(skipped)} skipped.')


if __name__ == '__main__':
    main()