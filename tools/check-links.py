#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
"""Check relative links in all Markdown files of the repository.

Language: Python 3, standard library only (see ADR 0002).

For every Markdown file tracked under the repository root (excluding build/
and third_party/), every inline link or image `[text](target)` and every
reference definition `[label]: target` is checked:

- external targets (http, https, mailto) are skipped (no network in checks);
- relative paths must exist, resolved from the file's directory;
- `#anchor` fragments must match a heading in the target Markdown file,
  using GitHub's heading-slug rules.

Links inside fenced code blocks and inline code spans are ignored.
Exit status: 0 if all links resolve, 1 otherwise.
"""

import os
import re
import sys

SKIP_DIRS = {".git", "build", "out", "third_party", "node_modules"}
EXTERNAL = re.compile(r"^[a-zA-Z][a-zA-Z0-9+.-]*:")
INLINE_LINK = re.compile(r"!?\[(?:[^\[\]]|\[[^\]]*\])*\]\(\s*<?([^)\s>]+)>?(?:\s+\"[^\"]*\")?\s*\)")
REF_DEF = re.compile(r"^\s{0,3}\[[^\]]+\]:\s*<?(\S+?)>?(?:\s+.*)?$")
HEADING = re.compile(r"^\s{0,3}(#{1,6})\s+(.*?)\s*#*\s*$")
FENCE = re.compile(r"^\s{0,3}(```|~~~)")
INLINE_CODE = re.compile(r"`+[^`]*`+")


def strip_code(lines):
    """Yield (line_number, text) with fenced blocks and inline code removed."""
    in_fence = None
    for number, line in enumerate(lines, start=1):
        match = FENCE.match(line)
        if match:
            marker = match.group(1)
            if in_fence is None:
                in_fence = marker
            elif in_fence == marker:
                in_fence = None
            continue
        if in_fence is None:
            yield number, INLINE_CODE.sub("", line)


def slugify(heading):
    """GitHub-style anchor for a heading text."""
    text = INLINE_CODE.sub(lambda m: m.group(0).strip("`"), heading)
    text = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", text)  # links -> text
    text = text.strip().lower()
    text = re.sub(r"[^\w\- ]", "", text)
    return text.replace(" ", "-")


def anchors_of(path, cache):
    if path not in cache:
        found = set()
        counts = {}
        with open(path, encoding="utf-8") as handle:
            for _, line in strip_code(handle.read().splitlines()):
                match = HEADING.match(line)
                if not match:
                    continue
                slug = slugify(match.group(2))
                if slug in counts:
                    counts[slug] += 1
                    found.add(f"{slug}-{counts[slug]}")
                else:
                    counts[slug] = 0
                    found.add(slug)
        cache[path] = found
    return cache[path]


def markdown_files(root):
    for directory, subdirs, files in os.walk(root):
        subdirs[:] = sorted(d for d in subdirs if d not in SKIP_DIRS)
        for name in sorted(files):
            if name.endswith(".md"):
                yield os.path.join(directory, name)


def check_file(path, root, cache):
    errors = []
    with open(path, encoding="utf-8") as handle:
        lines = handle.read().splitlines()
    for number, line in strip_code(lines):
        targets = [m.group(1) for m in INLINE_LINK.finditer(line)]
        ref = REF_DEF.match(line)
        if ref:
            targets.append(ref.group(1))
        for target in targets:
            if EXTERNAL.match(target):
                continue
            file_part, _, anchor = target.partition("#")
            if file_part:
                resolved = os.path.normpath(
                    os.path.join(os.path.dirname(path), file_part))
            else:
                resolved = path
            where = f"{os.path.relpath(path, root)}:{number}"
            if not os.path.exists(resolved):
                errors.append(f"{where}: missing target '{target}'")
                continue
            if anchor and resolved.endswith(".md"):
                if anchor.lower() not in anchors_of(resolved, cache):
                    errors.append(f"{where}: missing anchor '{target}'")
    return errors


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    cache = {}
    files = list(markdown_files(root))
    errors = []
    for path in files:
        errors.extend(check_file(path, root, cache))
    for error in errors:
        print(error)
    print(f"check-links: {len(files)} files, {len(errors)} broken link(s)")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
