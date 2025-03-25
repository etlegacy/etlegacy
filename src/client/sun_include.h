// SunLight - Demo viewer include file
#ifndef SUN_INCLUDE_H
#define SUN_INCLUDE_H

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

// Demo viewer constants
#define WARPOMETER_BACKUP 256
#define WARPOMETER_MASK 0xff

// Demo viewer variables
extern int demo_playernames;
extern int demo_follow_enabled;
extern int demo_follow_clientnum;
extern int demo_follow_original_player;
extern int demo_follow_attacker;
extern int demo_follow_validview;
extern int demo_warpometer_enabled;
extern int demo_warpometer_clientnum;
extern int demo_unlag_value;
extern int demo_force_zoom_shift;
extern int demo_force_zoom_enabled;
extern int demo_force_zoom_x;
extern int demo_force_zoom_y;
extern int demo_shift_fov;
extern float demo_force_timescale;
extern float demo_old_timescale;
extern float demo_force_timescale_alt;
extern float demo_old_timescale_alt;
extern int no_damage_kick;
extern int demo_is_seeking;
extern int first_serverCommandSequence;
extern int last_requested_snapshot;
extern char etr_ver[];
extern char last_demo_played[];
extern char last_mapname[];
extern char last_servername[];
extern int demo_demooffset;
extern int demo_total_length;
extern snapshot_t last_stored_snap;

// Function declarations
void GetPlayerName(int clientnum, char *str, int maxsize);
int get_player_team(int num);
entityState_t *FindPlayerEntityInSnap(snapshot_t *snap, int clientnum);
void Check_DemoSeek(void);

#endif // SUN_INCLUDE_H
