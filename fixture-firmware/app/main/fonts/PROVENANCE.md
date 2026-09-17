# Generated font provenance

These five `el_*` fonts are modified font software under SIL OFL 1.1, not MIT.
The copyright and reserved-name notices and full upstream license are preserved
byte-for-byte in `LICENSE-OFL-1.1`, retrieved from:
https://raw.githubusercontent.com/liberationfonts/liberation-fonts/2.1.5/LICENSE
The source milestone's Debian copyright document is also retained unchanged in
`LICENSE-Liberation.txt`; its separate debian/* packaging clause is not the font license.
Upstream: https://github.com/liberationfonts/liberation-fonts/tree/2.1.5

Source fonts: Debian `fonts-liberation` version `1:2.1.5-3`.
SHA-256:

| Source | SHA-256 |
| --- | --- |
| LiberationSans-Regular.ttf | bade59d822652f76e6941aa87b40a87c13d1cc70db98ededb5011127efafd1d3 |
| LiberationMono-Regular.ttf | 5883330d94debd992952cd8f0571b225f478c2d797d3f36c7521b0a5c9bde0f2 |
| LiberationSerif-Bold.ttf | 443a4dbd694b6e4689925947b456e0532e10b19c90fa32eb39ea6c2cd9db9ebd |

Converter: `lv_font_conv` 1.5.3. Each C file records its exact font, size and
conversion flags: 4bpp, ASCII 0x20–0x7e, LVGL format, no compression.
Sans is 14/16px, mono 16/20px, serif **bold** 24px. All five were regenerated
from the source files above and compared byte-for-byte after the first generated
comment (which contains the local invocation path); every generated payload matched.
The checked-in source milestone files remain unchanged. No converter is needed
for normal builds. `el_*` derivative names avoid the reserved font names.
No upstream endorsement is implied.

For the bounded native selector-content correction, the 14px and 16px sans
derivatives retain ASCII 0x20–0x7e and additionally include U+00B7 MIDDLE DOT,
U+2013 EN DASH, and U+2192 RIGHTWARDS ARROW from the same pinned Liberation Sans
source. This makes the contract's exact public selector labels render as glyphs
rather than missing-character boxes. Size, bpp, format, compression setting,
license, source family, and all non-sans generated fonts are unchanged.
