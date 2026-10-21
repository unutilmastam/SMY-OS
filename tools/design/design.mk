# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# Design assets (design/, docs/BRAND_PROMPT.md): validation, exports and
# preview sheets.

.PHONY: brand brand-check brand-preview design-check fonts

DESIGN_GEN := $(BUILD)/generated/design
FONT_DIR   := third_party/downloads/fonts

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
brand-preview brand: brand-check fonts
	$(Q)tools/design/brand_preview.sh
	$(Q)tools/design/brand_logo_preview.sh
	$(Q)python3 tools/design/validate_brand.py --exports
	$(Q)tools/design/theme_preview.sh

# Pinned, SHA-256-verified fonts (Inter, IBM Plex Mono) in $(FONT_DIR).
fonts:
	$(Q)tools/fetch-fonts.sh

# Design system (DESIGN_SYSTEM.md): base ramps up to date, WCAG contrast of
# every themed pair, token code generated and compiling, fonts covering
# Uzbek Latin and Cyrillic.
design-check: fonts
	$(Q)python3 tools/design/gen_palette.py --check
	$(Q)python3 tools/design/check_contrast.py
	$(Q)python3 tools/design/gen_tokens.py $(DESIGN_GEN)
	$(Q)printf '#include "smy_tokens.h"\n_Static_assert(SMY_COLOR_DARK_TEXT_PRIMARY == 0xFFF4F7FFu, "tokens");\n' | \
		$(CC) -std=c17 $(WARN_FLAGS) -I$(DESIGN_GEN) -fsyntax-only -x c -
	$(Q)python3 tools/design/check_font_coverage.py $(FONT_DIR)/inter/*.ttf $(FONT_DIR)/plex-mono/*.ttf
