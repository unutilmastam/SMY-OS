# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# Design assets (design/, docs/BRAND_PROMPT.md): validation, exports and
# preview sheets.

.PHONY: brand brand-check brand-preview

# Pure Python, no renderer needed: generated SVGs are up to date, every SVG
# under design/ passes the asset rules, the manifest matches the files and
# the documented colour pairs meet their contrast minimums.
brand-check:
	$(Q)python3 tools/design/brand_concepts.py --check
	$(Q)python3 tools/design/brand_logo.py --check
	$(Q)python3 tools/design/validate_svg.py
	$(Q)python3 tools/design/validate_brand.py

# Needs rsvg-convert (librsvg) and ImageMagick; writes build/brand/:
# concept sheet, logo family sheet and PNG exports (16-1024 px), then checks
# every expected export exists.
brand-preview brand: brand-check
	$(Q)tools/design/brand_preview.sh
	$(Q)tools/design/brand_logo_preview.sh
	$(Q)python3 tools/design/validate_brand.py --exports
