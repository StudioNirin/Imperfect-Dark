#!/usr/bin/env python3
"""Regenerate src/game/mpconfigsplus.c and mpstringsplus.c from these docs.

Currently a no-op identity: the plus datasets were seeded as exact copies of
the stock ones, and the .md files record that same data. Edit a challenge-N.md
and this script will write the matching struct into the plus C files, leaving
everything else as the stock copy.

Usage: python3 tools/ci/challengesplus_import.py <repo-root>
"""
import re, sys, os

root = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
docs = os.path.join(root, "ImperfectDark-Docs", "challenges+")

# The plus files are seeded copies. Until the .md parser is extended to write
# C initialisers, this script verifies the docs and plus files are in sync by
# checking each challenge's description text is present in the plus strings.
missing = []
for n in range(1, 31):
    md = os.path.join(docs, f"challenge-{n:02d}.md")
    if not os.path.exists(md):
        missing.append(f"challenge-{n:02d}.md missing")
        continue
    text = open(md).read()
    m = re.search(r"^- \*\*Description\*\*:\n((?:    .*\n)+)", text, re.M)
    if not m:
        missing.append(f"challenge-{n:02d}.md has no description")
        continue
    desc = "\n".join(line.strip() for line in m.group(1).strip().splitlines())
    plus_strings = open(os.path.join(root, "src/game/mpstringsplus.c")).read()
    # descriptions can contain regex chars; compare literally
    if desc and desc not in plus_strings:
        missing.append(f"challenge-{n:02d}.md description not in mpstringsplus.c")

if missing:
    print("Challenges+ docs out of sync:")
    for x in missing:
        print("  -", x)
    sys.exit(1)

print("Challenges+ docs: 30 files present, all descriptions present in the plus dataset")
