# Orbit logo provenance

Credit: **EntropyLab — Team Ooga Booga**,
[OogaBoogaX/entropylab](https://github.com/OogaBoogaX/entropylab), revision
`6e1f39cc7da25942c7a1f51ea5837f4ac7ef8f51`. This native adaptation is unofficial;
no affiliation, endorsement, individual logo authorship or security certification
is claimed.

## Origin and conversion

The tracked upstream [src/assets/logo-dark.svg](https://github.com/OogaBoogaX/entropylab/blob/6e1f39cc7da25942c7a1f51ea5837f4ac7ef8f51/src/assets/logo-dark.svg)
is the same artwork as the retained website copy `site-logo-dark.svg`. The bounded
XML comparison found identical child tags, attributes, text, geometry and viewBox;
only root class/accessibility attributes differ. The upstream SVG is 5,100 bytes,
SHA-256 `9125034467a970bc8fbac4d735f42d00410fdb4fa81a0b28c80dae17f4fbb745`.

`logo-dark-180x270.png` is the retained transparent Chromium raster of that artwork.
`logo_rgb565a8.inc` packs that raster into a little-endian RGB565 plane followed by
an A8 plane, without premultiplication. The payload is 145,800 bytes; it is derived
artwork, not original art because represented in C. The include records the
historical converter name; a converter is not required for normal builds and no
fresh rasterization/reproducibility claim is made here. Runtime uses the packed
180×270 image, not PNG/SVG decoding. Orbit ring/trail/marker decoration is native
work distinct from the reused illustration. Static payload size is not measured
target memory headroom or performance.

[logo-provenance.json](logo-provenance.json) uses paths relative to this directory.
File sizes/hashes and the extracted C pixel payload hash were recomputed for this
publication copy. The historical raster pipeline was not rerun.

## License coverage interpretation

The exact custom [THE OOGA BOOGA LICENSE](LICENSE-OOGA-BOOGA) is retained unchanged.
The pinned upstream [README lines 570–575](https://github.com/OogaBoogaX/entropylab/blob/6e1f39cc7da25942c7a1f51ea5837f4ac7ef8f51/README.md#L570-L575)
describes dedication of the software and permission to copy, modify, publish,
compile and distribute in source or binary form. The logo is a tracked, integrated
software asset; no separate restriction was found in the bounded review.

Publication retains the authentic asset under this repository-wide coverage
interpretation. The custom license's code-oriented wording leaves residual scope
uncertainty: this is neither a legal guarantee nor an explicit separate artwork or
trademark grant, and does not establish an unknown artist's authorship chain.
No owner contact or endorsement is implied. Upstream's comparison to the Unlicense
does not replace its exact custom license. See the repository
[third-party notices](../../../../THIRD_PARTY_NOTICES.md).
