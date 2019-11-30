/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * Portions of this file were taken from the NQ project.
 * Credit goes to core
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
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file g_strparse.h
 *
 * @brief Changes have to be in sync with input_token.gperf and fresh generated g_match_tokens.c
 */

#ifndef INCLUDE_G_STRPARSE_H
#define INCLUDE_G_STRPARSE_H

/**
 * @enum g_StringToken_t
 * @brief
 */
typedef enum
{
	TOK_g_currentRound = 0,             ///< 0
	TOK_g_nextTimeLimit,                ///< 1
	TOK_gamestate,                      ///< 2
	TOK_g_currentCampaign,              ///< 3
	TOK_g_currentCampaignMap,           ///< 4
	TOK_ip,                             ///< 5
	TOK_name,                           ///< 6
	TOK_cl_guid,                        ///< 7
	TOK_password,                       ///< 8
	TOK_scriptName,                     ///< 9
	TOK_respawn,                        ///< 10
	TOK_cg_uinfo,                       ///< 11
	TOK_pmove_fixed,                    ///< 12
	TOK_pmove_msec,                     ///< 13
	TOK_ch,                             ///< 14
	TOK_skill,                          ///< 15 obsolete/unused (botskill)
	TOK_rate,                           ///< 16
	TOK_n,                              ///< 17
	TOK_t,                              ///< 18
	TOK_c,                              ///< 19
	TOK_r,                              ///< 20
	TOK_f,                              ///< 21
	TOK_m,                              ///< 22
	TOK_s,                              ///< 23
	TOK_xp,                             ///< 24
	TOK_dn,                             ///< 25 now disguised client num (change to 'd' to safe a char per client)
	TOK_dc,                             ///< 26 obsolete/unused (disguised class)
	TOK_dr,                             ///< 27 unused
	TOK_w,                              ///< 28
	TOK_lw,                             ///< 29
	TOK_lc,                             ///< 30
	TOK_sw,                             ///< 31
	TOK_ref,                            ///< 32
	TOK_rn,                             ///< 33
	TOK_bd,                             ///< 34
	TOK_hd,                             ///< 35
	TOK_l,                              ///< 36
	TOK_bc,                             ///< 37
	TOK_bcs,                            ///< 38
	TOK_bl,                             ///< 39
	TOK_bls,                            ///< 40
	TOK_br,                             ///< 41
	TOK_brs,                            ///< 42
	TOK_he,                             ///< 43
	TOK_hs,                             ///< 44
	TOK_g,                              ///< 45
	TOK_gs,                             ///< 46
	TOK_mu,                             ///< 47
	TOK_lsw,                            ///< 48

	//// Don't add anything below here
	TOK_UNKNOWN                         ///< 49
} g_StringToken_t;

extern g_StringToken_t G_GetTokenForString(char const *str);

#endif  // #ifndef INCLUDE_G_STRPARSE_H
