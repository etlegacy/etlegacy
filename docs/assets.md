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
- Sample rate: commonly `22050` Hz or `44100` Hz

Recommended for music/streaming tracks:

- `22050` Hz stereo PCM WAV (this is the classic ET expectation and avoids warnings in the sound streamer path)

Notes:

- Avoid ADPCM/float/compressed WAV variants if you need strict 2.60b compatibility.
