/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing to id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file discord_log2_shim.c
 * @brief Provide log2() for the vendored /MD discord-rpc.lib under the engine's /MT static CRT
 *
 * The vendored discord-rpc.lib (src/discord/win32-static/lib/) was built against
 * the dynamic CRT (/MD), so its C-runtime references go through the dllimport
 * __imp_ thunks. The engine builds with the static CRT (/MT — FORCE_STATIC_VCRT
 * is ON by default, cmake/ETLPlatform.cmake replaces /MD with /MT), so the linker
 * synthesizes __imp_ thunks that bridge those references into the static CRT for
 * most symbols (free/malloc/time/memmove/... → LNK4217, benign). log2() has no
 * local definition in this toolset's static UCRT (it is inline-only in <math.h>),
 * so __imp__log2 is left unresolved (LNK2019) — the single link failure.
 *
 * Providing a log2() definition here lets the linker synthesize __imp__log2 the
 * same way it already does for the other CRT symbols. log() (natural log) IS
 * provided by the static CRT, so log2(x) = ln(x)/ln(2).
 *
 * @note Built only under the MSVC AND FEATURE_DISCORD gate (same as the lib link).
 */

#include <math.h>

/**
 * @brief log2 — C99 base-2 logarithm; absent from this toolset's static UCRT.
 * @param[in] x
 * @return log base 2 of x
 */
double __cdecl log2(double x)
{
	/* ln() (C log()) is provided by the static CRT; log2(x) = ln(x) / ln(2) */
	return log(x) / 0.693147180559945309417232121458;
}
