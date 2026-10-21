# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# Design assets (design/, docs/BRAND_PROMPT.md): validation and previews.

.PHONY: brand-check brand-preview

# Pure Python, no renderer needed: generated SVGs are up to date and every
# SVG under design/ passes the asset rules.
brand-check:
	$(Q)python3 tools/design/brand_concepts.py --check
	$(Q)python3 tools/design/validate_svg.py

# Needs rsvg-convert (librsvg) and ImageMagick; writes build/brand/.
brand-preview:
	$(Q)tools/design/brand_preview.sh
