#ifndef G_STRPARSE_H
#define G_STRPARSE_H

typedef enum
{
	TOK_g_currentRound,                 // 0
	TOK_g_nextTimeLimit,                // 1
	TOK_gamestate,                      // 2
	TOK_g_currentCampaign,              // 3
	TOK_g_currentCampaignMap,           // 4
	TOK_ip,                             // 5
	TOK_name,                           // 6
	TOK_cl_guid,                        // 7
	TOK_password,                       // 8
	TOK_scriptName,                     // 9
	TOK_respawn,                        // 10
	TOK_cg_uinfo,                       // 11
	TOK_pmove_fixed,                    // 12
	TOK_pmove_msec,                     // 13
	TOK_ch,                             // 14
	TOK_skill,                          // 15
	TOK_rate,                           // 16
	TOK_n,                              // 17
	TOK_t,                              // 18
	TOK_c,                              // 19
	TOK_r,                              // 20
	TOK_f,                              // 21
	TOK_m,                              // 22
	TOK_s,                              // 23
	TOK_xp,                             // 24
	TOK_dn,                             // 25
	TOK_dc,                             // 26
	TOK_dr,                             // 27
	TOK_w,                              // 28
	TOK_lw,                             // 29
	TOK_lc,                             // 30
	TOK_sw,                             // 31
	TOK_ref,                            // 32
	TOK_rn,                             // 33
	TOK_bd,                             // 34
	TOK_hd,                             // 35
	TOK_l,                              // 36
	TOK_bc,                             // 37
	TOK_bcs,                            // 38
	TOK_bl,                             // 39
	TOK_bls,                            // 40
	TOK_br,                             // 41
	TOK_brs,                            // 42
	TOK_he,                             // 43
	TOK_hs,                             // 44
	TOK_g,                              // 45
	TOK_gs,                             // 46
	TOK_mu,

	// Don't add anything below here
	TOK_UNKNOWN
} g_StringToken_t;

extern g_StringToken_t G_GetTokenForString(char const *str);

#endif  // G_STRPARSE_H
