## Asset formats required for Enemy Territory 2.60b compatibility

This document lists safe asset settings when content must work in ET 2.60b.

### `.tga` textures

Use these settings:

- Extension: `.tga`
- Color map: none (indexed/color-mapped TGA is not supported)
- Image type:
  - `2` (uncompressed true-color), or
  - `10` (RLE true-color), or
  - `3` (grayscale)
- Pixel depth:
  - `24` bpp for RGB, or
  - `32` bpp for RGBA
  - `8` bpp is valid only for type `3` grayscale
- Dimensions: non-zero width and height
- Orientation for cross-version safety: bottom-left origin (clear top-origin bit in TGA header attribute bit 5)

Notes:

- For type `2` and `10`, only 24/32-bit data is safe for 2.60b compatibility.
- Top-origin TGA files can behave differently across engine versions; use bottom-origin for predictable results.

### `.wav` sounds

Use these settings:

- Container: RIFF/WAVE with a `fmt ` chunk and a `data` chunk
- Codec: PCM (format tag `1`)
- Sample format:
  - `8-bit` unsigned PCM, or
  - `16-bit` little-endian PCM
- Channels: `1` (mono) or `2` (stereo)
- Sample rate (anything else will produce an engine warning):
  - For ordinary sounds: `11025` Hz or `22050` Hz or `44100` Hz
  - For streaming sounds (e.g. music): `22050` Hz
- Avoid ADPCM/float/compressed WAV variants if you need strict 2.60b compatibility

### `.pk3` Archives

Standard ZIP archives with the following constraints:

- **Compression**
  - Stored (ZIP method 0): No compression
  - Deflate (ZIP method 8): Standard DEFLATE, compression levels 1–9

- **ZIP64:** Unsupported, ZIP 32-bit only, which means:
  - Maximum archive/member size: engine support up to 2 GiB
  - Maximum entries: 65,535

- **Encryption:** Unsupported.

- **Paths and Filenames**
  - Keep the full path (including directories and filename) below 64 bytes for
    maximum compatibility
  - Case-insensitive filesystem lookups (files are lowercased, duplicate names
    within a PK3 resolve to the last entry in ZIP central-directory order, last
    one wins)
