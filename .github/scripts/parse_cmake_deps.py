#!/usr/bin/env python3
"""
Parse FetchContent_Declare blocks in a CMakeLists.txt and output JSON.
Usage: parse_cmake_deps.py [path]
If path omitted, reads ./CMakeLists.txt
Outputs stable JSON array of {name, repo, tag}
"""
import sys
import re
import json
from pathlib import Path

def parse_cmake(text):
    deps = []
    for m in re.finditer(r'FetchContent_Declare\((.*?)\)', text, re.S):
        block = m.group(1)
        if 'GIT_REPOSITORY' in block and 'GIT_TAG' in block:
            # find name
            name_match = re.search(r'^\s*([A-Za-z0-9_\-]+)', block)
            name = name_match.group(1) if name_match else 'unknown'
            repo_match = re.search(r'GIT_REPOSITORY\s+([^\n\r]+)', block)
            tag_match = re.search(r'GIT_TAG\s+([^\n\r]+)', block)
            if repo_match and tag_match:
                repo = repo_match.group(1).strip()
                tag = tag_match.group(1).strip()
                deps.append({'name': name, 'repo': repo, 'tag': tag})
    # stable ordering
    deps.sort(key=lambda d: d['name'])
    return deps


def main():
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('CMakeLists.txt')
    if not path.exists():
        print(f"Error: {path} not found", file=sys.stderr)
        sys.exit(2)
    text = path.read_text(encoding='utf-8')
    deps = parse_cmake(text)
    json.dump(deps, sys.stdout, ensure_ascii=False, indent=2)

if __name__ == '__main__':
    main()
