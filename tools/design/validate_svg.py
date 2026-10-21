#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
"""Validate every SVG asset under design/ (docs/BRAND_PROMPT.md §14).

Language: Python 3, standard library only (ADR 0002).

Checks per file: well-formed XML with an <svg> root in the SVG namespace; a
viewBox of four numbers with positive width and height; no scripts, raster
images, foreign objects, filters or <text> (logos are drawn as paths); no
references to anything outside the file (href / url() must point to "#id");
lower-case kebab-case file names.

Usage: tools/design/validate_svg.py [directory]   (default: design/)
"""

import pathlib
import re
import sys
import xml.etree.ElementTree as ET

SVG_NS = "{http://www.w3.org/2000/svg}"
FORBIDDEN = {"script", "image", "foreignObject", "filter", "text", "style"}
NAME = re.compile(r"^[a-z0-9]+(-[a-z0-9]+)*\.svg$")


def check(path):
    errors = []
    if not NAME.match(path.name):
        errors.append("file name is not lower-case kebab-case")
    try:
        root = ET.parse(path).getroot()
    except ET.ParseError as exc:
        return [f"XML parse error: {exc}"]
    if root.tag != SVG_NS + "svg":
        errors.append(f"root element is {root.tag}, expected svg")
    box = root.get("viewBox", "").replace(",", " ").split()
    try:
        nums = [float(v) for v in box]
        if len(nums) != 4 or nums[2] <= 0 or nums[3] <= 0:
            raise ValueError
    except ValueError:
        errors.append(f"invalid viewBox {root.get('viewBox')!r}")
    for el in root.iter():
        tag = el.tag.replace(SVG_NS, "")
        if tag in FORBIDDEN:
            errors.append(f"forbidden element <{tag}>")
        for key, value in el.attrib.items():
            if key.endswith("href") and not value.startswith("#"):
                errors.append(f"external reference {key}={value!r}")
            if "url(" in value and not re.fullmatch(r".*url\(#[^)]+\).*", value):
                errors.append(f"external url() in {key}")
            if key == "filter":
                errors.append("filter attribute")
    return errors


def main(argv):
    base = pathlib.Path(argv[0]) if argv else pathlib.Path(__file__).resolve().parents[2] / "design"
    files = sorted(base.rglob("*.svg"))
    failed = 0
    for path in files:
        for err in check(path):
            print(f"{path}: {err}")
            failed += 1
    if not files:
        print(f"validate_svg: no SVG files under {base}")
        return 1
    if failed:
        print(f"validate_svg: FAIL ({failed} problem(s) in {len(files)} files)")
        return 1
    print(f"validate_svg: OK ({len(files)} files)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
