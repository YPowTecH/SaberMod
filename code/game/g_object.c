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

// leave this line at the top for all g_xxxx.cpp files...
#include "g_local.h"

extern void G_MoverTouchPushTriggers( gentity_t *ent, const vec3_t oldOrg );
void G_StopObjectMoving( gentity_t *object );

/*
====================================================================
void pitch_roll_for_slope (edict_t *forwhom, vec3_t *slope)

MG

This will adjust the pitch and roll of a monster to match
a given slope - if a non-'0 0 0' slope is passed, it will
use that value, otherwise it will use the ground underneath
the monster.  If it doesn't find a surface, it does nothinh\g
and returns.
====================================================================
*/

void pitch_roll_for_slope( gentity_t *forwhom, vec3_t pass_slope )
{
	vec3_t	slope;
	vec3_t	nvf, ovf, ovr, startspot, endspot, new_angles;
	float	pitch, mod, dot;
	float	oldmins2;

	//if we don't have a slope, get one
	if( !pass_slope || VectorCompare( vec3_origin, pass_slope ) )
	{
		trace_t trace;

		VectorCopy( forwhom->r.currentOrigin, startspot );
		startspot[2] += forwhom->r.mins[2] + 4;
		VectorCopy( startspot, endspot );
		endspot[2] -= 300;
		G_Trace( &trace, forwhom->r.currentOrigin, vec3_origin, vec3_origin, endspot, forwhom->s.number, MASK_SOLID );
//		if(trace_fraction>0.05&&forwhom.movetype==MOVETYPE_STEP)
//			forwhom.flags(-)FL_ONGROUND;

		if ( trace.fraction >= 1.0f )
			return;

		if ( trace.allsolid )
			return;

		VectorCopy( trace.plane.normal, slope );
	}
	else
	{
		VectorCopy( pass_slope, slope );
	}


	AngleVectors( forwhom->r.currentAngles, ovf, ovr, NULL );

	vectoangles( slope, new_angles );
	pitch = new_angles[PITCH] + 90;
	new_angles[ROLL] = new_angles[PITCH] = 0;

	AngleVectors( new_angles, nvf, NULL, NULL );

	mod = DotProduct( nvf, ovr );

	if ( mod<0 )
		mod = -1;
	else
		mod = 1;

	dot = DotProduct( nvf, ovf );

	if ( forwhom->client )
	{
		forwhom->client->ps.viewangles[PITCH] = dot * pitch;
		forwhom->client->ps.viewangles[ROLL] = ((1-Q_fabs(dot)) * pitch * mod);
		oldmins2 = forwhom->r.mins[2];
		forwhom->r.mins[2] = -24 + 12 * fabsf(forwhom->client->ps.viewangles[PITCH])/180.0f;
		//FIXME: if it gets bigger, move up
		if ( oldmins2 > forwhom->r.mins[2] )
		{//our mins is now lower, need to move up
			//FIXME: trace?
			forwhom->client->ps.origin[2] += (oldmins2 - forwhom->r.mins[2]);
			forwhom->r.currentOrigin[2] = forwhom->client->ps.origin[2];
			trap_LinkEntity( forwhom );
		}
	}
	else
	{
		forwhom->r.currentAngles[PITCH] = dot * pitch;
		forwhom->r.currentAngles[ROLL] = ((1-Q_fabs(dot)) * pitch * mod);
	}
}

/*
================
G_BounceObject

================
*/
void G_BounceObject( gentity_t *ent, trace_t *trace )
{
	vec3_t	velocity;
	float	dot, bounceFactor;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + (int)( ( level.time - level.previousTime ) * trace->fraction );
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	// bounceFactor = 60/ent->mass;		// NOTENOTE Mass is not yet implemented
	// bounceFactor = MIN(bounceFactor, 1);
	bounceFactor = 1.0f;

	VectorMA( velocity, -2*dot*bounceFactor, trace->plane.normal, ent->s.pos.trDelta );

	//FIXME: customized or material-based impact/bounce sounds
	if ( ent->s.eFlags & EF_BOUNCE_HALF )
	{
		VectorScale( ent->s.pos.trDelta, 0.5f, ent->s.pos.trDelta );

		// check for stop
		if (((trace->plane.normal[2] > 0.7f && g_gravity.value > 0) || (trace->plane.normal[2] < -0.7f && g_gravity.value < 0)) &&
			((ent->s.pos.trDelta[2] < 40 && g_gravity.value > 0) || (ent->s.pos.trDelta[2] > -40 && g_gravity.value < 0))) //this can happen even on very slightly sloped walls, so changed it from > 0 to > 0.7
		{
			//G_SetOrigin( ent, trace->endpos );
			//ent->nextthink = level.time + 500;
			ent->s.apos.trType = TR_STATIONARY;
			VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );
			VectorCopy( trace->endpos, ent->r.currentOrigin );
			VectorCopy( trace->endpos, ent->s.pos.trBase );
			ent->s.pos.trTime = level.time;
			return;
		}
	}

	// NEW--It would seem that we want to set our trBase to the trace endpos
	//	and set the trTime to the actual time of impact....
	//	FIXME: Should we still consider adding the normal though??
	VectorCopy( trace->endpos, ent->r.currentOrigin );
	ent->s.pos.trTime = hitTime;

	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	VectorCopy( trace->plane.normal, ent->pos1 );//???
}


/*
================
G_RunObject

  TODO:  When transition to 0 grav, push away from surface you were resting on
  TODO:  When free-floating in air, apply some friction to your trDelta (based on mass?)
================
*/
extern void DoImpact( gentity_t *self, gentity_t *other, qboolean damageSelf );
void G_RunObject( gentity_t *ent )
{
	vec3_t		origin, oldOrg;
	trace_t		tr;
	gentity_t	*traceEnt = NULL;

	//FIXME: floaters need to stop floating up after a while, even if gravity stays negative?
	if ( ent->s.pos.trType == TR_STATIONARY )//g_gravity.value <= 0 &&
	{
		ent->s.pos.trType = TR_GRAVITY;
		VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
		ent->s.pos.trTime = level.previousTime;//?necc?
		if ( !g_gravity.value )
		{
			ent->s.pos.trDelta[2] += 100;
		}
	}

	ent->nextthink = level.time + FRAMETIME;

	VectorCopy( ent->r.currentOrigin, oldOrg );
	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	//Get current angles?
	BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->r.currentAngles );

	if ( VectorCompare( ent->r.currentOrigin, origin ) )
	{//error - didn't move at all!
		return;
	}
	// trace a line from the previous position to the current position,
	// ignoring interactions with the missile owner
	G_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin,
		ent->parent ? ent->parent->s.number : ent->s.number, ent->clipmask );

	if ( !tr.startsolid && !tr.allsolid && tr.fraction )
	{
		VectorCopy( tr.endpos, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}
	else
	//if ( tr.startsolid )
	{
		tr.fraction = 0;
	}

	G_MoverTouchPushTriggers( ent, oldOrg );
	/*
	if ( !(ent->s.eFlags & EF_TELEPORT_BIT) && !(ent->svFlags & SVF_NO_TELEPORT) )
	{
		G_MoverTouchTeleportTriggers( ent, oldOrg );
		if ( ent->s.eFlags & EF_TELEPORT_BIT )
		{//was teleported
			return;
		}
	}
	else
	{
		ent->s.eFlags &= ~EF_TELEPORT_BIT;
	}
	*/

	if ( tr.fraction == 1 )
	{
		if ( g_gravity.value <= 0 )
		{
			if ( ent->s.apos.trType == TR_STATIONARY )
			{
				VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );
				ent->s.apos.trType = TR_LINEAR;
				ent->s.apos.trDelta[1] = flrand( -300, 300 );
				ent->s.apos.trDelta[0] = flrand( -10, 10 );
				ent->s.apos.trDelta[2] = flrand( -10, 10 );
				ent->s.apos.trTime = level.time;
			}
		}
		//friction in zero-G
		if ( !g_gravity.value )
		{
			float friction = 0.975f;
			// friction -= ent->mass/1000.0f;
			// friction = MAX(friction, 0.1f);

			VectorScale( ent->s.pos.trDelta, friction, ent->s.pos.trDelta );
			VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
			ent->s.pos.trTime = level.time;
		}
		return;
	}

	//hit something

	//Do impact damage
	traceEnt = &g_entities[tr.entityNum];
	if ( tr.fraction || (traceEnt && traceEnt->takedamage) )
	{
		if ( !VectorCompare( ent->r.currentOrigin, oldOrg ) )
		{//moved and impacted
			if ( (traceEnt && traceEnt->takedamage) )
			{//hurt someone
//				G_Sound( ent, G_SoundIndex( "sound/movers/objects/objectHurt.wav" ) );
			}
//			G_Sound( ent, G_SoundIndex( "sound/movers/objects/objectHit.wav" ) );
		}

		if (ent->s.weapon != WP_SABER)
		{
			DoImpact( ent, traceEnt, qtrue );
		}
	}

	if ( !ent || (ent->takedamage&&ent->health <= 0) )
	{//been destroyed by impact
		//chunks?
//		G_Sound( ent, G_SoundIndex( "sound/movers/objects/objectBreak.wav" ) );
		return;
	}

	//do impact physics
	if ( ent->s.pos.trType == TR_GRAVITY )//tr.fraction < 1.0 &&
	{//FIXME: only do this if no trDelta
		if ( g_gravity.value <= 0 || tr.plane.normal[2] < 0.7f )
		{
			if ( ent->s.eFlags&(EF_BOUNCE|EF_BOUNCE_HALF) )
			{
				if ( tr.fraction <= 0.0f )
				{
					VectorCopy( tr.endpos, ent->r.currentOrigin );
					VectorCopy( tr.endpos, ent->s.pos.trBase );
					VectorClear( ent->s.pos.trDelta );
					ent->s.pos.trTime = level.time;
				}
				else
				{
					G_BounceObject( ent, &tr );
				}
			}
			else
			{//slide down?
				//FIXME: slide off the slope
			}
		}
		else
		{
			ent->s.apos.trType = TR_STATIONARY;
			pitch_roll_for_slope( ent, tr.plane.normal );
			//ent->r.currentAngles[0] = 0;//FIXME: match to slope
			//ent->r.currentAngles[2] = 0;//FIXME: match to slope
			VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );
			//okay, we hit the floor, might as well stop or prediction will
			//make us go through the floor!
			//FIXME: this means we can't fall if something is pulled out from under us...
			G_StopObjectMoving( ent );
		}
	}
	else if (ent->s.weapon != WP_SABER)
	{
		ent->s.apos.trType = TR_STATIONARY;
		pitch_roll_for_slope( ent, tr.plane.normal );
		//ent->r.currentAngles[0] = 0;//FIXME: match to slope
		//ent->r.currentAngles[2] = 0;//FIXME: match to slope
		VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );
	}

	//call touch func
	ent->touch( ent, &g_entities[tr.entityNum], &tr );
}


void G_StopObjectMoving( gentity_t *object )
{
	object->s.pos.trType = TR_STATIONARY;
	VectorCopy( object->r.currentOrigin, object->s.origin );
	VectorCopy( object->r.currentOrigin, object->s.pos.trBase );
	VectorClear( object->s.pos.trDelta );

	/*
	//Stop spinning
	VectorClear( self->s.apos.trDelta );
	vectoangles(trace->plane.normal, self->s.angles);
	VectorCopy(self->s.angles, self->r.currentAngles );
	VectorCopy(self->s.angles, self->s.apos.trBase);
	*/
}

#ifdef UNUSED
void G_StartObjectMoving( gentity_t *object, vec3_t dir, float speed, trType_t trType )
{
	VectorNormalize (dir);

	//object->s.eType = ET_GENERAL;
	object->s.pos.trType = trType;
	VectorCopy( object->r.currentOrigin, object->s.pos.trBase );
	VectorScale(dir, speed, object->s.pos.trDelta );
	object->s.pos.trTime = level.time;

	/*
	//FIXME: incorporate spin?
	vectoangles(dir, object->s.angles);
	VectorCopy(object->s.angles, object->s.apos.trBase);
	VectorSet(object->s.apos.trDelta, 300, 0, 0 );
	object->s.apos.trTime = level.time;
	*/

	//FIXME: make these objects go through G_RunObject automatically, like missiles do
	if ( object->think == NULL )
	{
		object->nextthink = level.time + FRAMETIME;
		object->think = G_RunObject;
	}
	else
	{//You're responsible for calling RunObject
	}
}
#endif // UNUSED
