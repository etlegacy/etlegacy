/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		ai_dmnet_mp.h
 *
 * desc:		Wolf bot AI
 *
 *
 *****************************************************************************/

void AIEnter_MP_PlantMine( bot_state_t *bs );
void AIEnter_MP_Intermission( bot_state_t *bs );
void AIEnter_MP_Observer( bot_state_t *bs );
void AIEnter_MP_Respawn( bot_state_t *bs );
void AIEnter_MP_Stand( bot_state_t *bs );
void AIEnter_MP_Seek_ActivateEntity( bot_state_t *bs );
void AIEnter_MP_Seek_NBG( bot_state_t *bs );
void AIEnter_MP_Seek_LTG( bot_state_t *bs );
void AIEnter_MP_Seek_Camp( bot_state_t *bs );
void AIEnter_MP_Battle_Fight( bot_state_t *bs );
void AIEnter_MP_Battle_Chase( bot_state_t *bs );
void AIEnter_MP_Battle_Retreat( bot_state_t *bs );
void AIEnter_MP_DynamiteTarget( bot_state_t *bs );
void AIEnter_MP_SatchelChargeTarget( bot_state_t *bs );
void AIEnter_MP_ConstructibleTarget( bot_state_t *bs );
void AIEnter_MP_TouchTarget( bot_state_t *bs );
void AIEnter_MP_DefendTarget( bot_state_t *bs );
void AIEnter_MP_MedicRevive( bot_state_t *bs );
void AIEnter_MP_MedicGiveHealth( bot_state_t *bs );
void AIEnter_MP_DisarmDynamite( bot_state_t *bs );
void AIEnter_MP_AvoidDanger( bot_state_t *bs );
void AIEnter_MP_GiveAmmo( bot_state_t *bs );
void AIEnter_MP_PanzerTarget( bot_state_t *bs );
void AIEnter_MP_SniperSpot( bot_state_t *bs );
void AIEnter_MP_ScanForLandmines( bot_state_t *bs );
void AIEnter_MP_FixMG42( bot_state_t *bs );
void AIEnter_MP_MG42Scan( bot_state_t *bs );
void AIEnter_MP_MG42Mount( bot_state_t *bs );
void AIEnter_MP_Script_MoveToMarker( bot_state_t *bs );
void AIEnter_MP_MoveToAutonomyRange( bot_state_t *bs );
void AIEnter_MP_PanzerTarget( bot_state_t *bs );
void AIEnter_MP_AttackTarget( bot_state_t *bs );
void AIEnter_MP_NavigateFromVoid( bot_state_t *bs );
void AIEnter_MP_Battle_MobileMG42( bot_state_t *bs );
int AINode_MP_PlantMine( bot_state_t *bs );
int AINode_MP_Intermission( bot_state_t *bs );
int AINode_MP_Observer( bot_state_t *bs );
int AINode_MP_Respawn( bot_state_t *bs );
int AINode_MP_Stand( bot_state_t *bs );
int AINode_MP_Seek_ActivateEntity( bot_state_t *bs );
int AINode_MP_Seek_NBG( bot_state_t *bs );
int AINode_MP_Battle_Fight( bot_state_t *bs );
int AINode_MP_Battle_Chase( bot_state_t *bs );
int AINode_MP_Battle_Retreat( bot_state_t *bs );
int AINode_MP_DynamiteTarget( bot_state_t *bs );
int AINode_MP_SatchelChargeTarget( bot_state_t *bs );
int AINode_MP_ConstructibleTarget( bot_state_t *bs );
int AINode_MP_TouchTarget( bot_state_t *bs );
int AINode_MP_DefendTarget( bot_state_t *bs );
int AINode_MP_MedicRevive( bot_state_t *bs );
int AINode_MP_MedicGiveHealth( bot_state_t *bs );
int AINode_MP_DisarmDynamite( bot_state_t *bs );
int AINode_MP_AvoidDanger( bot_state_t *bs );
int AINode_MP_GiveAmmo( bot_state_t *bs );
int AINode_MP_PanzerTarget( bot_state_t *bs );
int AINode_MP_SniperSpot( bot_state_t *bs );
int AINode_MP_FixMG42( bot_state_t *bs );
int AINode_MP_MG42Scan( bot_state_t *bs );
int AINode_MP_MG42Mount( bot_state_t *bs );
int AINode_MP_ScanForLandmines( bot_state_t *bs );
int AINode_MP_Script_MoveToMarker( bot_state_t *bs );
int AINode_MP_MoveToAutonomyRange( bot_state_t *bs );
int AINode_MP_AttackTarget( bot_state_t *bs );
int AINode_MP_NavigateFromVoid( bot_state_t *bs );
int AINode_MP_Battle_MobileMG42( bot_state_t *bs );
