/*
================================================================================
This file is part of SaberMod - Star Wars Jedi Knight II: Jedi Outcast mod.

Copyright (C) 1999-2000 Id Software, Inc.
Copyright (C) 1999-2002 Activision
Copyright (C) 2015-2019 Witold Pilat <witold.pilat@gmail.com>

This program is free software; you can redistribute it and/or modify it
under the terms and conditions of the GNU General Public License,
version 2, as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
================================================================================
*/

// BG_PAnimate.c

#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "anims.h"
#include "../cgame/animtable.h"

// for debuggers
animNumber_t BG_Anim( int anim ) {
	return ANIM(anim);
}

/*
==============================================================================
BEGIN: Animation utility functions (sequence checking)
==============================================================================
*/
//Called regardless of pm validity:
qboolean BG_InSpecialJump( animNumber_t anim )
{
	switch ( anim )
	{
	case BOTH_WALL_RUN_RIGHT:
	case BOTH_WALL_RUN_RIGHT_FLIP:
	case BOTH_WALL_RUN_LEFT:
	case BOTH_WALL_RUN_LEFT_FLIP:
	case BOTH_WALL_FLIP_RIGHT:
	case BOTH_WALL_FLIP_LEFT:
	case BOTH_FLIP_BACK1:
	case BOTH_FLIP_BACK2:
	case BOTH_FLIP_BACK3:
	case BOTH_WALL_FLIP_BACK1:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean BG_InSaberStandAnim( animNumber_t anim )
{
	switch ( anim )
	{
	case BOTH_SABERFAST_STANCE:
	case BOTH_STAND2:
	case BOTH_SABERSLOW_STANCE:
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean BG_DirectFlippingAnim( animNumber_t anim )
{
	switch ( anim )
	{
	case BOTH_FLIP_F:			//# Flip forward
	case BOTH_FLIP_B:			//# Flip backwards
	case BOTH_FLIP_L:			//# Flip left
	case BOTH_FLIP_R:			//# Flip right
		return qtrue;
	default:
		return qfalse;
	}

}

qboolean BG_SaberInAttack( saberMoveName_t move )
{
	if ( move >= LS_A_TL2BR && move <= LS_A_T2B )
	{
		return qtrue;
	}
	switch ( move )
	{
	case LS_A_BACK:
	case LS_A_BACK_CR:
	case LS_A_BACKSTAB:
	case LS_A_LUNGE:
	case LS_A_JUMP_T__B_:
	case LS_A_FLIP_STAB:
	case LS_A_FLIP_SLASH:
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean BG_SaberInSpecial( saberMoveName_t move )
{
	switch( move )
	{
	case LS_A_BACK:
	case LS_A_BACK_CR:
	case LS_A_BACKSTAB:
	case LS_A_LUNGE:
	case LS_A_JUMP_T__B_:
	case LS_A_FLIP_STAB:
	case LS_A_FLIP_SLASH:
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean BG_SaberInIdle( saberMoveName_t move )
{
	switch ( move )
	{
	case LS_NONE:
	case LS_READY:
	case LS_DRAW:
	case LS_PUTAWAY:
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean BG_FlippingAnim( animNumber_t anim )
{
	switch ( anim )
	{
	case BOTH_FLIP_F:			//# Flip forward
	case BOTH_FLIP_B:			//# Flip backwards
	case BOTH_FLIP_L:			//# Flip left
	case BOTH_FLIP_R:			//# Flip right
	case BOTH_WALL_RUN_RIGHT_FLIP:
	case BOTH_WALL_RUN_LEFT_FLIP:
	case BOTH_WALL_FLIP_RIGHT:
	case BOTH_WALL_FLIP_LEFT:
	case BOTH_FLIP_BACK1:
	case BOTH_FLIP_BACK2:
	case BOTH_FLIP_BACK3:
	case BOTH_WALL_FLIP_BACK1:
	//Not really flips, but...
	case BOTH_WALL_RUN_RIGHT:
	case BOTH_WALL_RUN_LEFT:
	case BOTH_WALL_RUN_RIGHT_STOP:
	case BOTH_WALL_RUN_LEFT_STOP:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	//
	case BOTH_ARIAL_LEFT:
	case BOTH_ARIAL_RIGHT:
	case BOTH_ARIAL_F1:
	case BOTH_CARTWHEEL_LEFT:
	case BOTH_CARTWHEEL_RIGHT:
	case BOTH_JUMPFLIPSLASHDOWN1:
	case BOTH_JUMPFLIPSTABDOWN:
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean BG_SpinningSaberAnim( animNumber_t anim )
{
	switch ( anim )
	{
	//level 1 - FIXME: level 1 will have *no* spins
	case BOTH_T1_BR_BL:
	case BOTH_T1__R__L:
	case BOTH_T1__R_BL:
	case BOTH_T1_TR_BL:
	case BOTH_T1_BR_TL:
	case BOTH_T1_BR__L:
	case BOTH_T1_TL_BR:
	case BOTH_T1__L_BR:
	case BOTH_T1__L__R:
	case BOTH_T1_BL_BR:
	case BOTH_T1_BL__R:
	case BOTH_T1_BL_TR:
	//level 2
	case BOTH_T2_BR__L:
	case BOTH_T2_BR_BL:
	case BOTH_T2__R_BL:
	case BOTH_T2__L_BR:
	case BOTH_T2_BL_BR:
	case BOTH_T2_BL__R:
	//level 3
	case BOTH_T3_BR__L:
	case BOTH_T3_BR_BL:
	case BOTH_T3__R_BL:
	case BOTH_T3__L_BR:
	case BOTH_T3_BL_BR:
	case BOTH_T3_BL__R:
	//level 4
	case BOTH_T4_BR__L:
	case BOTH_T4_BR_BL:
	case BOTH_T4__R_BL:
	case BOTH_T4__L_BR:
	case BOTH_T4_BL_BR:
	case BOTH_T4_BL__R:
	//level 5
	case BOTH_T5_BR_BL:
	case BOTH_T5__R__L:
	case BOTH_T5__R_BL:
	case BOTH_T5_TR_BL:
	case BOTH_T5_BR_TL:
	case BOTH_T5_BR__L:
	case BOTH_T5_TL_BR:
	case BOTH_T5__L_BR:
	case BOTH_T5__L__R:
	case BOTH_T5_BL_BR:
	case BOTH_T5_BL__R:
	case BOTH_T5_BL_TR:
	//special
	//case BOTH_A2_STABBACK1:
	case BOTH_ATTACK_BACK:
	case BOTH_CROUCHATTACKBACK1:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_JUMPFLIPSLASHDOWN1:
	case BOTH_JUMPFLIPSTABDOWN:
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean BG_SaberInSpecialAttack( animNumber_t anim )
{
	switch ( anim )
	{
	case BOTH_A2_STABBACK1:
	case BOTH_ATTACK_BACK:
	case BOTH_CROUCHATTACKBACK1:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_LUNGE2_B__T_:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_JUMPFLIPSLASHDOWN1://#
	case BOTH_JUMPFLIPSTABDOWN://#
		return qtrue;
	default:
		return qfalse;
	}
}

saberMoveName_t BG_BrokenParryForAttack( saberMoveName_t move )
{
	//Our attack was knocked away by a knockaway parry
	//FIXME: need actual anims for this
	//FIXME: need to know which side of the saber was hit!  For now, we presume the saber gets knocked away from the center
	switch ( saberMoveData[move].startQuad )
	{
	case Q_B:  return LS_V1_B_;
	case Q_BR: return LS_V1_BR;
	case Q_R:  return LS_V1__R;
	case Q_TR: return LS_V1_TR;
	case Q_T:  return LS_V1_T_;
	case Q_TL: return LS_V1_TL;
	case Q_L:  return LS_V1__L;
	case Q_BL: return LS_V1_BL;
	default:   return LS_NONE;
	}
}

saberMoveName_t BG_BrokenParryForParry( saberMoveName_t move )
{
	//FIXME: need actual anims for this
	//FIXME: need to know which side of the saber was hit!  For now, we presume the saber gets knocked away from the center
	switch ( move )
	{
	case LS_PARRY_UP:
		//Hmm... since we don't know what dir the hit came from, randomly pick knock down or knock back
		if ( Q_irand( 0, 1 ) )
		{
			return LS_H1_B_;
		}
		else
		{
			return LS_H1_T_;
		}
	case LS_PARRY_UR: return LS_H1_TR;
	case LS_PARRY_UL: return LS_H1_TL;
	case LS_PARRY_LR: return LS_H1_BR;
	case LS_PARRY_LL: return LS_H1_BL;
	case LS_READY:    return LS_H1_B_;//???
	default:          return LS_NONE;
	}
}

saberMoveName_t BG_KnockawayForParry( saberBlockedType_t move )
{
	//FIXME: need actual anims for this
	//FIXME: need to know which side of the saber was hit!  For now, we presume the saber gets knocked away from the center
	switch ( move )
	{
	case BLOCKED_TOP://LS_PARRY_UP:
		return LS_K1_T_;//push up
	case BLOCKED_UPPER_RIGHT://LS_PARRY_UR:
	default://case LS_READY:
		return LS_K1_TR;//push up, slightly to right
	case BLOCKED_UPPER_LEFT://LS_PARRY_UL:
		return LS_K1_TL;//push up and to left
	case BLOCKED_LOWER_RIGHT://LS_PARRY_LR:
		return LS_K1_BR;//push down and to left
	case BLOCKED_LOWER_LEFT://LS_PARRY_LL:
		return LS_K1_BL;//push down and to right
	}
	//return LS_NONE;
}

qboolean BG_InRoll( const playerState_t *ps, animNumber_t anim )
{
	switch ( anim )
	{
	case BOTH_ROLL_F:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		if ( ps->legsTimer > 0 )
		{
			return qtrue;
		}
		return qfalse;
	default:
		return qfalse;
	}
}

qboolean BG_InDeathAnim( animNumber_t anim )
{
	switch ( anim )
	{
	case BOTH_DIVE1:
	case BOTH_DEATHBACKWARD1:
	case BOTH_DEATHBACKWARD2:
	case BOTH_DEATHFORWARD1:
	case BOTH_DEATHFORWARD2:
	case BOTH_DEATH1:
	case BOTH_DEATH2:
	case BOTH_DEATH3:
	case BOTH_DEATH4:
	case BOTH_DEATH5:
	case BOTH_DEATH6:
	case BOTH_DEATH7:

	case BOTH_DEATH1IDLE:
	case BOTH_LYINGDEATH1:
	case BOTH_STUMBLEDEATH1:
	case BOTH_FALLDEATH1:
	case BOTH_FALLDEATH1INAIR:
	case BOTH_FALLDEATH1LAND:
		return qtrue;
	default:
		return qfalse;
	}
}

//Called only where pm is valid (not all require pm, but some do):
saberMoveName_t PM_SaberBounceForAttack( saberMoveName_t move )
{
	switch ( saberMoveData[move].startQuad )
	{
	case Q_B:
	case Q_BR: return LS_B1_BR;
	case Q_R:  return LS_B1__R;
	case Q_TR: return LS_B1_TR;
	case Q_T:  return LS_B1_T_;
	case Q_TL: return LS_B1_TL;
	case Q_L:  return LS_B1__L;
	case Q_BL: return LS_B1_BL;
	default:   return LS_NONE;
	}
}

saberMoveName_t PM_SaberDeflectionForQuad( saberQuadrant_t quad )
{
	switch ( quad )
	{
	case Q_B:  return LS_D1_B_;
	case Q_BR: return LS_D1_BR;
	case Q_R:  return LS_D1__R;
	case Q_TR: return LS_D1_TR;
	case Q_T:  return LS_D1_T_;
	case Q_TL: return LS_D1_TL;
	case Q_L:  return LS_D1__L;
	case Q_BL: return LS_D1_BL;
	default:   return LS_NONE;
	}
}

qboolean PM_SaberInDeflect( saberMoveName_t move )
{
	if ( move >= LS_D1_BR && move <= LS_D1_B_ )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInParry( saberMoveName_t move )
{
	if ( move >= LS_PARRY_UP && move <= LS_PARRY_LL )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInKnockaway( saberMoveName_t move )
{
	if ( move >= LS_K1_T_ && move <= LS_K1_BL )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInReflect( saberMoveName_t move )
{
	if ( move >= LS_REFLECT_UP && move <= LS_REFLECT_LL )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInStart( saberMoveName_t move )
{
	if ( move >= LS_S_TL2BR && move <= LS_S_T2B )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInReturn( saberMoveName_t move )
{
	if ( move >= LS_R_TL2BR && move <= LS_R_TL2BR )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_InSaberAnim( animNumber_t anim )
{
	if ( anim >= BOTH_A1_T__B_ && anim <= BOTH_H1_S1_BR )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_InKnockDown( playerState_t *ps )
{
	animNumber_t animN = ANIM(ps->legsAnim);

	switch ( animN )
	{
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN4:
	case BOTH_KNOCKDOWN5:
		return qtrue;
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
		if ( ps->legsTimer )
		{
			return qtrue;
		}
		return qfalse;
	default:
		return qfalse;
	}
}

qboolean PM_PainAnim( animNumber_t anim )
{
	switch ( anim )
	{
	case BOTH_PAIN1:				//# First take pain anim
	case BOTH_PAIN2:				//# Second take pain anim
	case BOTH_PAIN3:				//# Third take pain anim
	case BOTH_PAIN4:				//# Fourth take pain anim
	case BOTH_PAIN5:				//# Fifth take pain anim - from behind
	case BOTH_PAIN6:				//# Sixth take pain anim - from behind
	case BOTH_PAIN7:				//# Seventh take pain anim - from behind
	case BOTH_PAIN8:				//# Eigth take pain anim - from behind
	case BOTH_PAIN9:				//#
	case BOTH_PAIN10:			//#
	case BOTH_PAIN11:			//#
	case BOTH_PAIN12:			//#
	case BOTH_PAIN13:			//#
	case BOTH_PAIN14:			//#
	case BOTH_PAIN15:			//#
	case BOTH_PAIN16:			//#
	case BOTH_PAIN17:			//#
	case BOTH_PAIN18:			//#
	case BOTH_PAIN19:			//#
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean PM_JumpingAnim( animNumber_t anim )
{
	switch ( anim )
	{
	case BOTH_JUMP1:				//# Jump - wind-up and leave ground
	case BOTH_INAIR1:			//# In air loop (from jump)
	case BOTH_LAND1:				//# Landing (from in air loop)
	case BOTH_LAND2:				//# Landing Hard (from a great height)
	case BOTH_JUMPBACK1:			//# Jump backwards - wind-up and leave ground
	case BOTH_INAIRBACK1:		//# In air loop (from jump back)
	case BOTH_LANDBACK1:			//# Landing backwards(from in air loop)
	case BOTH_JUMPLEFT1:			//# Jump left - wind-up and leave ground
	case BOTH_INAIRLEFT1:		//# In air loop (from jump left)
	case BOTH_LANDLEFT1:			//# Landing left(from in air loop)
	case BOTH_JUMPRIGHT1:		//# Jump right - wind-up and leave ground
	case BOTH_INAIRRIGHT1:		//# In air loop (from jump right)
	case BOTH_LANDRIGHT1:		//# Landing right(from in air loop)
	case BOTH_FORCEJUMP1:				//# Jump - wind-up and leave ground
	case BOTH_FORCEINAIR1:			//# In air loop (from jump)
	case BOTH_FORCELAND1:				//# Landing (from in air loop)
	case BOTH_FORCEJUMPBACK1:			//# Jump backwards - wind-up and leave ground
	case BOTH_FORCEINAIRBACK1:		//# In air loop (from jump back)
	case BOTH_FORCELANDBACK1:			//# Landing backwards(from in air loop)
	case BOTH_FORCEJUMPLEFT1:			//# Jump left - wind-up and leave ground
	case BOTH_FORCEINAIRLEFT1:		//# In air loop (from jump left)
	case BOTH_FORCELANDLEFT1:			//# Landing left(from in air loop)
	case BOTH_FORCEJUMPRIGHT1:		//# Jump right - wind-up and leave ground
	case BOTH_FORCEINAIRRIGHT1:		//# In air loop (from jump right)
	case BOTH_FORCELANDRIGHT1:		//# Landing right(from in air loop)
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean PM_LandingAnim( animNumber_t anim )
{
	switch ( anim )
	{
	case BOTH_LAND1:				//# Landing (from in air loop)
	case BOTH_LAND2:				//# Landing Hard (from a great height)
	case BOTH_LANDBACK1:			//# Landing backwards(from in air loop)
	case BOTH_LANDLEFT1:			//# Landing left(from in air loop)
	case BOTH_LANDRIGHT1:		//# Landing right(from in air loop)
	case BOTH_FORCELAND1:		//# Landing (from in air loop)
	case BOTH_FORCELANDBACK1:	//# Landing backwards(from in air loop)
	case BOTH_FORCELANDLEFT1:	//# Landing left(from in air loop)
	case BOTH_FORCELANDRIGHT1:	//# Landing right(from in air loop)
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean PM_SpinningAnim( animNumber_t anim )
{
	/*
	switch ( anim )
	{
	//FIXME: list any other spinning anims
	default:
		break;
	}
	*/
	return BG_SpinningSaberAnim( anim );
}

qboolean PM_InOnGroundAnim ( animNumber_t anim )
{
	switch ( anim )
	{
	case BOTH_DEAD1:
	case BOTH_DEAD2:
	case BOTH_DEAD3:
	case BOTH_DEAD4:
	case BOTH_DEAD5:
	case BOTH_DEADFORWARD1:
	case BOTH_DEADBACKWARD1:
	case BOTH_DEADFORWARD2:
	case BOTH_DEADBACKWARD2:
	case BOTH_LYINGDEATH1:
	case BOTH_LYINGDEAD1:
	case BOTH_PAIN2WRITHE1:		//# Transition from upright position to writhing on ground anim
	case BOTH_WRITHING1:			//# Lying on ground writhing in pain
	case BOTH_WRITHING1RLEG:		//# Lying on ground writhing in pain: holding right leg
	case BOTH_WRITHING1LLEG:		//# Lying on ground writhing in pain: holding left leg
	case BOTH_WRITHING2:			//# Lying on stomache writhing in pain
	case BOTH_INJURED1:			//# Lying down: against wall - can also be sleeping
	case BOTH_CRAWLBACK1:			//# Lying on back: crawling backwards with elbows
	case BOTH_INJURED2:			//# Injured pose 2
	case BOTH_INJURED3:			//# Injured pose 3
	case BOTH_INJURED6:			//# Injured pose 6
	case BOTH_INJURED6ATTACKSTART:	//# Start attack while in injured 6 pose
	case BOTH_INJURED6ATTACKSTOP:	//# End attack while in injured 6 pose
	case BOTH_INJURED6COMBADGE:	//# Hit combadge while in injured 6 pose
	case BOTH_INJURED6POINT:		//# Chang points to door while in injured state
	case BOTH_KNOCKDOWN1:		//#
	case BOTH_KNOCKDOWN2:		//#
		return qtrue;
	default:
		return qfalse;
	}

}

qboolean PM_InRollComplete( playerState_t *ps, animNumber_t anim )
{
	switch ( anim )
	{
	case BOTH_ROLL_F:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		if ( ps->legsTimer < 1 )
		{
			return qtrue;
		}
		return qfalse;
	default:
		return qfalse;
	}
}

int PM_AnimLength( animNumber_t anim )
{
	if (anim >= MAX_ANIMATIONS)
	{
		return -1;
	}
	return pm->animations[anim].numFrames * abs(pm->animations[anim].frameLerp);
}

void PM_DebugLegsAnim(int anim)
{
	animNumber_t oldAnim = ANIM(pm->ps->legsAnim);
	animNumber_t newAnim = ANIM(anim);

	if (oldAnim < MAX_TOTALANIMATIONS && oldAnim >= BOTH_DEATH1 &&
		newAnim < MAX_TOTALANIMATIONS && newAnim >= BOTH_DEATH1)
	{
		Com_Printf("OLD: %s\n", animTable[oldAnim].name);
		Com_Printf("NEW: %s\n", animTable[newAnim].name);
	}
}
/*
==============================================================================
END: Animation utility functions (sequence checking)
==============================================================================
*/

/*
======================
BG_ParseAnimationFile

Read a configuration file containing animation coutns and rates
models/players/visor/animation.cfg, etc

======================
*/
static char		BGPAFtext[40000];
qboolean		BGPAFtextLoaded = qfalse;
animation_t		bgGlobalAnimations[MAX_TOTALANIMATIONS];

//#define CONVENIENT_ANIMATION_FILE_DEBUG_THING

#ifdef CONVENIENT_ANIMATION_FILE_DEBUG_THING
void SpewDebugStuffToFile()
{
	fileHandle_t f;
	int i = 0;

	trap_FS_FOpenFile("file_of_debug_stuff_MP.txt", &f, FS_WRITE);

	if (!f)
	{
		return;
	}

	BGPAFtext[0] = 0;

	while (i < MAX_ANIMATIONS)
	{
		Q_strcat(BGPAFtext, sizeof(BGPAFtext), va("%i %i\n", i, bgGlobalAnimations[i].frameLerp));
		i++;
	}

	trap_FS_Write(BGPAFtext, strlen(BGPAFtext), f);
	trap_FS_FCloseFile(f);
}
#endif

#define HASH_TABLE_SIZE		4096

qboolean BG_ParseAnimationFile(const char *filename)
{
	char		*text_p;
	int			len;
	unsigned	i;
	char		*token;
	float		fps;

	fileHandle_t	f;
	// int because values correspond to animTable indexes, not ids
	int			 	animHashTable[HASH_TABLE_SIZE];
	unsigned		hash;
	int				index;
	int				animNum;

	// load the file
	if (!BGPAFtextLoaded)
	{ //rww - We are always using the same animation config now. So only load it once.
		len = trap_FS_FOpenFile( filename, &f, FS_READ );
		if ( len <= 0 )
		{
			return qfalse;
		}
		if ( len >= (int)sizeof( BGPAFtext ) - 1 )
		{
			//Com_Printf( "File %s too long\n", filename );
			return qfalse;
		}

		trap_FS_Read( BGPAFtext, len, f );
		BGPAFtext[len] = 0;
		trap_FS_FCloseFile( f );
	}
	else
	{
		return qtrue;
	}

	// -1 marks empty slot
	for ( i = 0; i < HASH_TABLE_SIZE; i++ ) {
		animHashTable[i] = -1;
	}

	// fill the hash table
	for ( i = 0; i < ARRAY_LEN(animTable) - 1; i++ ) {
		hash = COM_HashForString( animTable[i].name, HASH_TABLE_SIZE );

		// open addressing with linear probing
		while ( animHashTable[hash] != -1 )
			hash = (hash + 1) & (HASH_TABLE_SIZE - 1);

		animHashTable[hash] = i;
	}

	// parse the text
	text_p = BGPAFtext;

	//FIXME: have some way of playing anims backwards... negative numFrames?

	//initialize anim array so that from 0 to MAX_ANIMATIONS, set default values of 0 1 0 100
	for(i = 0; i < MAX_ANIMATIONS; i++)
	{
		bgGlobalAnimations[i].firstFrame = 0;
		bgGlobalAnimations[i].numFrames = 0;
		bgGlobalAnimations[i].loopFrames = -1;
		bgGlobalAnimations[i].frameLerp = 100;
		bgGlobalAnimations[i].initialLerp = 100;
	}

	// read information for each frame
	while(1)
	{
		token = COM_Parse( (const char **)(&text_p) );

		if ( !token || !token[0])
		{
			break;
		}

		// find animNum in hash table
		hash = COM_HashForString( token, HASH_TABLE_SIZE );
		while ( 1 ) {
			index = animHashTable[hash];

			if ( index == -1 )
				break;

			if ( !Q_stricmp( token, animTable[index].name ) )
				break;

			hash = (hash + 1) & (HASH_TABLE_SIZE - 1);
		}

		if(index == -1)
		{
//#ifndef FINAL_BUILD
#ifdef _DEBUG
			Com_Printf(S_COLOR_RED"WARNING: Unknown token %s in %s\n", token, filename);
#endif
			continue;
		}

		animNum = animTable[index].id;

		token = COM_Parse( (const char **)(&text_p) );
		if ( !token )
		{
			break;
		}
		bgGlobalAnimations[animNum].firstFrame = atoi( token );

		token = COM_Parse( (const char **)(&text_p) );
		if ( !token )
		{
			break;
		}
		bgGlobalAnimations[animNum].numFrames = atoi( token );

		token = COM_Parse( (const char **)(&text_p) );
		if ( !token )
		{
			break;
		}
		bgGlobalAnimations[animNum].loopFrames = atoi( token );

		token = COM_Parse( (const char **)(&text_p) );
		if ( !token )
		{
			break;
		}
		fps = atof( token );
		if ( fps == 0 )
		{
			fps = 1;//Don't allow divide by zero error
		}
		if ( fps < 0 )
		{//backwards
			bgGlobalAnimations[animNum].frameLerp = floorf(1000.0f / fps);
		}
		else
		{
			bgGlobalAnimations[animNum].frameLerp = ceilf(1000.0f / fps);
		}

		bgGlobalAnimations[animNum].initialLerp = ceilf(1000.0f / fabsf(fps));
	}

#ifdef _DEBUG
	//Check the array, and print the ones that have nothing in them.
	for(i = 0; i < MAX_ANIMATIONS; i++)
	{
		if (animTable[i].name != NULL)		// This animation reference exists.
		{
			if (bgGlobalAnimations[i].firstFrame <= 0 && bgGlobalAnimations[i].numFrames <=0)
			{	// This is an empty animation reference.
				Com_Printf("***ANIMTABLE reference #%d (%s) is empty!\n", i, animTable[i].name);
			}
		}
	}
#endif // _DEBUG

#ifdef CONVENIENT_ANIMATION_FILE_DEBUG_THING
	SpewDebugStuffToFile();
#endif
	BGPAFtextLoaded = qtrue;
	return qtrue;
}



/*
===================
LEGS Animations
Base animation for overall body
===================
*/
static void PM_StartLegsAnim( animNumber_t anim ) {
	switch ( pm->ps->pm_type ) {
	case PM_DEAD:
	case PM_FREEZE:
	case PM_INTERMISSION:
	case PM_SPINTERMISSION:
		return;
	default:
		break;
	}
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}

	if (pm->ps->usingATST)
	{ //animation is handled mostly client-side with only a few exceptions
		return;
	}

	if (BG_InSaberStandAnim(anim) && pm->ps->weapon == WP_SABER && pm->ps->dualBlade)
	{ //a bit of a hack, but dualblade is cheat-only anyway
		anim = BOTH_STAND1;
	}

	pm->ps->legsAnim = ( ( pm->ps->legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
		| anim;

	if ( pm->debugLevel ) {
		Com_Printf("%d:  StartLegsAnim %d, on client#%d\n", pm->cmd.serverTime, anim, pm->ps->clientNum);
	}
}

void PM_ContinueLegsAnim( animNumber_t anim ) {
	if ( ANIM( pm->ps->legsAnim ) == anim ) {
		return;
	}
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}

	PM_StartLegsAnim( anim );
}

void PM_ForceLegsAnim( animNumber_t anim) {
	if (BG_InSpecialJump(ANIM(pm->ps->legsAnim)) &&
		pm->ps->legsTimer > 0 &&
		!BG_InSpecialJump(anim))
	{
		return;
	}

	if (BG_InRoll(pm->ps, ANIM(pm->ps->legsAnim)) &&
		pm->ps->legsTimer > 0 &&
		!BG_InRoll(pm->ps, anim))
	{
		return;
	}

	pm->ps->legsTimer = 0;
	PM_StartLegsAnim( anim );
}



/*
===================
TORSO Animations
Override animations for upper body
===================
*/
void PM_StartTorsoAnim( animNumber_t anim ) {
	switch ( pm->ps->pm_type ) {
	case PM_DEAD:
	case PM_FREEZE:
	case PM_INTERMISSION:
	case PM_SPINTERMISSION:
		return;
	default:
		break;
	}

	if (pm->ps->usingATST)
	{ //animation is handled mostly client-side with only a few exceptions
		return;
	}

	if (BG_InSaberStandAnim(anim) && pm->ps->weapon == WP_SABER && pm->ps->dualBlade)
	{ //a bit of a hack, but dualblade is cheat-only anyway
		anim = BOTH_STAND1;
	}

	pm->ps->torsoAnim = ( ( pm->ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
		| anim;
}


/*
-------------------------
PM_SetLegsAnimTimer
-------------------------
*/

void PM_SetLegsAnimTimer( int time )
{
	pm->ps->legsTimer = time;

	if (pm->ps->legsTimer < 0 && time != -1 )
	{//Cap timer to 0 if was counting down, but let it be -1 if that was intentional.  NOTENOTE Yeah this seems dumb, but it mirrors SP.
		pm->ps->legsTimer = 0;
	}
}

/*
-------------------------
PM_SetTorsoAnimTimer
-------------------------
*/

void PM_SetTorsoAnimTimer( int time )
{
	pm->ps->torsoTimer = time;

	if (pm->ps->torsoTimer < 0 && time != -1 )
	{//Cap timer to 0 if was counting down, but let it be -1 if that was intentional.  NOTENOTE Yeah this seems dumb, but it mirrors SP.
		pm->ps->torsoTimer = 0;
	}
}

void BG_SaberStartTransAnim( animNumber_t anim, float *animSpeed )
{
	assert( anim < MAX_TOTALANIMATIONS);

	// fau - prevent esp saber style cheat from working
	if (anim >= BOTH_T1_BR__R && anim <= BOTH_T1_BL_TL)
	{
		*animSpeed *= 1.5f;
	}
	else if (anim >= BOTH_T3_BR__R && anim <= BOTH_T3_BL_TL)
	{
		*animSpeed *= 0.75f;
	}
}

/*
-------------------------
PM_SetAnimFinal
-------------------------
*/
void PM_SetAnimFinal(int setAnimParts, animNumber_t anim, int setAnimFlags, int blendTime)		// default blendTime=350
{
	const animation_t *animations = pm->animations;

	float editAnimSpeed = 1;

	if (!animations)
	{
		return;
	}

	//NOTE: Setting blendTime here breaks actual blending..
	//blendTime = 0;

	BG_SaberStartTransAnim(anim, &editAnimSpeed);

	// Set torso anim
	if (setAnimParts & SETANIM_TORSO)
	{
		// Don't reset if it's already running the anim
		if( !(setAnimFlags & SETANIM_FLAG_RESTART) && ANIM(pm->ps->torsoAnim) == anim )
		{
			goto setAnimLegs;
		}
		// or if a more important anim is running
		if( !(setAnimFlags & SETANIM_FLAG_OVERRIDE) && ((pm->ps->torsoTimer > 0)||(pm->ps->torsoTimer == -1)) )
		{
			goto setAnimLegs;
		}

		PM_StartTorsoAnim( anim );

		if (setAnimFlags & SETANIM_FLAG_HOLD)
		{
			if (setAnimFlags & SETANIM_FLAG_HOLDLESS)
			{	// Make sure to only wait in full 1/20 sec server frame intervals.
				int dur;
				int speedDif;

				dur = (animations[anim].numFrames-1) * abs(animations[anim].frameLerp);
				speedDif = dur - (dur * editAnimSpeed);
				dur += speedDif;
				if (dur > 1)
				{
					pm->ps->torsoTimer = dur-1;
				}
				else
				{
					pm->ps->torsoTimer = abs(animations[anim].frameLerp);
				}
			}
			else
			{
				pm->ps->torsoTimer = ((animations[anim].numFrames ) * abs(animations[anim].frameLerp));
			}

			if (pm->ps->fd.forcePowersActive & (1 << FP_RAGE))
			{
				pm->ps->torsoTimer /= 1.7f;
			}
		}
	}

setAnimLegs:
	// Set legs anim
	if (setAnimParts & SETANIM_LEGS)
	{
		// Don't reset if it's already running the anim
		if( !(setAnimFlags & SETANIM_FLAG_RESTART) && ANIM(pm->ps->legsAnim) == anim )
		{
			goto setAnimDone;
		}
		// or if a more important anim is running
		if( !(setAnimFlags & SETANIM_FLAG_OVERRIDE) && ((pm->ps->legsTimer > 0)||(pm->ps->legsTimer == -1)) )
		{
			goto setAnimDone;
		}

		PM_StartLegsAnim(anim);

		if (setAnimFlags & SETANIM_FLAG_HOLD)
		{
			if (setAnimFlags & SETANIM_FLAG_HOLDLESS)
			{	// Make sure to only wait in full 1/20 sec server frame intervals.
				int dur;
				int speedDif;

				dur = (animations[anim].numFrames-1) * abs(animations[anim].frameLerp);
				speedDif = dur - (dur * editAnimSpeed);
				dur += speedDif;
				if (dur > 1)
				{
					pm->ps->legsTimer = dur-1;
				}
				else
				{
					pm->ps->legsTimer = abs(animations[anim].frameLerp);
				}
			}
			else
			{
				pm->ps->legsTimer = ((animations[anim].numFrames ) * abs(animations[anim].frameLerp));
			}

			if (pm->ps->fd.forcePowersActive & (1 << FP_RAGE))
			{
				pm->ps->legsTimer /= 1.3f;
			}
			else if (pm->ps->fd.forcePowersActive & (1 << FP_SPEED))
			{
				pm->ps->legsTimer /= 1.7f;
			}
		}
	}

setAnimDone:
	return;
}



// Imported from single-player, this function is mainly intended to make porting from SP easier.
void PM_SetAnim(int setAnimParts, animNumber_t anim, unsigned setAnimFlags, int blendTime)
{
	assert( anim < MAX_TOTALANIMATIONS);
	assert(	bgGlobalAnimations[anim].firstFrame != 0 ||
			bgGlobalAnimations[anim].numFrames != 0);

	if (BG_InSpecialJump(anim))
	{
		setAnimFlags |= SETANIM_FLAG_RESTART;
	}

	if (BG_InRoll(pm->ps, ANIM(pm->ps->legsAnim)))
	{ //never interrupt a roll
		return;
	}

	if (setAnimFlags&SETANIM_FLAG_OVERRIDE)
	{
		if (setAnimParts & SETANIM_TORSO)
		{
			if( (setAnimFlags & SETANIM_FLAG_RESTART) || ANIM(pm->ps->torsoAnim) != anim )
			{
				PM_SetTorsoAnimTimer(0);
			}
		}
		if (setAnimParts & SETANIM_LEGS)
		{
			if( (setAnimFlags & SETANIM_FLAG_RESTART) || ANIM(pm->ps->legsAnim) != anim )
			{
				PM_SetLegsAnimTimer(0);
			}
		}
	}

	PM_SetAnimFinal(setAnimParts, anim, setAnimFlags, blendTime);
}
