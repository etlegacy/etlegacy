# Replacing stock shaders
You can replace stock shaders (from the `pak0.pk3`) by placing an identically
named shader file below `./etmain` and it will take precedence over the stock
one.

For this to work however, you must copy the entire shader file, you can not
extend or override individual shaders inside the same shader file - only whole
shader files can be replaced.

## `.pk3` resolution order
1. `fs_game` (e.g. `legacy`) takes precedence over `etmain` (stock)
2. within the same directory reversed alphabetical sorting takes precedence
   e.g.: `legacy/z.pk3` would overrule `legacy/a.pk3`

**CAVEAT**: For the case that the same shader name (e.g.
`sprites/medicrevive.tga`) is being specified in different shader files (e.g.
`pak0/scripts/foo.shader` and `legacy/scripts/bar.shader`) the resolution is
indeterminate, so avoid this case by renaming one of them to a unique shader
name instead.
