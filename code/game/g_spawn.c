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

#include "g_local.h"
#include "bg_version.h"

qboolean	G_SpawnString( const char *key, const char *defaultString, char **out ) {
	int		i;

	if ( !level.spawning ) {
		*out = (char *)defaultString;
//		G_Error( "G_SpawnString() called while not spawning" );
	}

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		if ( !Q_stricmp( key, level.spawnVars[i][0] ) ) {
			*out = level.spawnVars[i][1];
			return qtrue;
		}
	}

	*out = (char *)defaultString;
	return qfalse;
}

qboolean	G_SpawnFloat( const char *key, const char *defaultString, float *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atof( s );
	return present;
}

qboolean	G_SpawnInt( const char *key, const char *defaultString, int *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atoi( s );
	return present;
}

qboolean	G_SpawnVector( const char *key, const char *defaultString, float *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f", &out[0], &out[1], &out[2] );
	return present;
}



//
// fields are needed for spawning from the entity string
//
typedef enum {
	F_INT,
	F_FLOAT,
	F_LSTRING,			// string on disk, pointer in memory, TAG_LEVEL
	F_GSTRING,			// string on disk, pointer in memory, TAG_GAME
	F_VECTOR,
	F_ANGLEHACK,
	F_ENTITY,			// index on disk, pointer in memory
	F_ITEM,				// index on disk, pointer in memory
	F_CLIENT,			// index on disk, pointer in memory
	F_IGNORE
} fieldtype_t;

typedef struct
{
	const char	*name;
	int			ofs;
	fieldtype_t	type;
	int			flags;
} field_t;

static const field_t fields[] = {
	{"classname", FOFS(classname), F_LSTRING},
	{"teamnodmg", FOFS(teamnodmg), F_INT},
	{"roffname", FOFS(roffname), F_LSTRING},
	{"rofftarget", FOFS(rofftarget), F_LSTRING},
	{"origin", FOFS(s.origin), F_VECTOR},
	{"model", FOFS(model), F_LSTRING},
	{"model2", FOFS(model2), F_LSTRING},
	{"spawnflags", FOFS(spawnflags), F_INT},
	{"speed", FOFS(speed), F_FLOAT},
	{"target", FOFS(target), F_LSTRING},
	{"targetname", FOFS(targetname), F_LSTRING},
	{"message", FOFS(message), F_LSTRING},
	{"team", FOFS(team), F_LSTRING},
	{"wait", FOFS(wait), F_FLOAT},
	{"random", FOFS(random), F_FLOAT},
	{"count", FOFS(count), F_INT},
	{"health", FOFS(health), F_INT},
	{"light", 0, F_IGNORE},
	{"dmg", FOFS(damage), F_INT},
	{"angles", FOFS(s.angles), F_VECTOR},
	{"angle", FOFS(s.angles), F_ANGLEHACK},
	{"targetShaderName", FOFS(targetShaderName), F_LSTRING},
	{"targetShaderNewName", FOFS(targetShaderNewName), F_LSTRING},

	{NULL}
};


typedef struct {
	const char	*name;
	void		(*spawn)(gentity_t *ent);
} spawn_t;

void SP_info_player_start (gentity_t *ent);
void SP_info_player_deathmatch (gentity_t *ent);
void SP_info_player_imperial (gentity_t *ent);
void SP_info_player_rebel (gentity_t *ent);
void SP_info_player_intermission (gentity_t *ent);
void SP_info_jedimaster_start (gentity_t *ent);
void SP_info_firstplace(gentity_t *ent);
void SP_info_secondplace(gentity_t *ent);
void SP_info_thirdplace(gentity_t *ent);
void SP_info_podium(gentity_t *ent);

void SP_info_saga_objective (gentity_t *ent);

void SP_func_plat (gentity_t *ent);
void SP_func_static (gentity_t *ent);
void SP_func_rotating (gentity_t *ent);
void SP_func_bobbing (gentity_t *ent);
void SP_func_pendulum( gentity_t *ent );
void SP_func_button (gentity_t *ent);
void SP_func_door (gentity_t *ent);
void SP_func_train (gentity_t *ent);
void SP_func_timer (gentity_t *self);
void SP_func_breakable (gentity_t *ent);
void SP_func_glass (gentity_t *ent);
void SP_func_usable( gentity_t *ent);

void SP_trigger_always (gentity_t *ent);
void SP_trigger_multiple (gentity_t *ent);
void SP_trigger_push (gentity_t *ent);
void SP_trigger_teleport (gentity_t *ent);
void SP_trigger_hurt (gentity_t *ent);

void SP_target_remove_powerups( gentity_t *ent );
void SP_target_give (gentity_t *ent);
void SP_target_delay (gentity_t *ent);
void SP_target_speaker (gentity_t *ent);
void SP_target_print (gentity_t *ent);
void SP_target_laser (gentity_t *self);
void SP_target_character (gentity_t *ent);
void SP_target_score( gentity_t *ent );
void SP_target_teleporter( gentity_t *ent );
void SP_target_relay (gentity_t *ent);
void SP_target_kill (gentity_t *ent);
void SP_target_position (gentity_t *ent);
void SP_target_location (gentity_t *ent);
void SP_target_push (gentity_t *ent);

void SP_light (gentity_t *self);
void SP_info_null (gentity_t *self);
void SP_info_notnull (gentity_t *self);
void SP_info_camp (gentity_t *self);
void SP_path_corner (gentity_t *self);

void SP_misc_teleporter_dest (gentity_t *self);
void SP_misc_model(gentity_t *ent);
void SP_misc_G2model(gentity_t *ent);
void SP_misc_portal_camera(gentity_t *ent);
void SP_misc_portal_surface(gentity_t *ent);

void SP_misc_shield_floor_unit( gentity_t *ent );
void SP_misc_model_shield_power_converter( gentity_t *ent );
void SP_misc_model_ammo_power_converter( gentity_t *ent );
void SP_misc_model_health_power_converter( gentity_t *ent );

void SP_fx_runner( gentity_t *ent );

#ifdef ANIMENT_SPAWNER
void SP_misc_animent_spawner(gentity_t *ent);
void SP_target_screenshake(gentity_t *ent);
void SP_target_escapetrig(gentity_t *ent);
#endif

void SP_misc_holocron(gentity_t *ent);

void SP_shooter_blaster( gentity_t *ent );

void SP_team_CTF_redplayer( gentity_t *ent );
void SP_team_CTF_blueplayer( gentity_t *ent );

void SP_team_CTF_redspawn( gentity_t *ent );
void SP_team_CTF_bluespawn( gentity_t *ent );

void SP_item_botroam( gentity_t *ent )
{
}

void SP_emplaced_gun( gentity_t *ent );

static const spawn_t	spawns[] = {
	// info entities don't do anything at all, but provide positional
	// information for things controlled by other processes
	{"info_player_start", SP_info_player_start},
	{"info_player_deathmatch", SP_info_player_deathmatch},
	{"info_player_imperial", SP_info_player_imperial},
	{"info_player_rebel", SP_info_player_rebel},
	{"info_player_intermission", SP_info_player_intermission},
	{"info_jedimaster_start", SP_info_jedimaster_start},
	{"info_null", SP_info_null},
	{"info_notnull", SP_info_notnull},		// use target_position instead
	{"info_camp", SP_info_camp},

	{"info_saga_objective", SP_info_saga_objective},

	{"func_plat", SP_func_plat},
	{"func_button", SP_func_button},
	{"func_door", SP_func_door},
	{"func_static", SP_func_static},
	{"func_rotating", SP_func_rotating},
	{"func_bobbing", SP_func_bobbing},
	{"func_pendulum", SP_func_pendulum},
	{"func_train", SP_func_train},
	{"func_group", SP_info_null},
	{"func_timer", SP_func_timer},			// rename trigger_timer?
	{"func_breakable", SP_func_breakable},
	{"func_glass", SP_func_glass},
	{"func_usable", SP_func_usable},

	// Triggers are brush objects that cause an effect when contacted
	// by a living player, usually involving firing targets.
	// While almost everything could be done with
	// a single trigger class and different targets, triggered effects
	// could not be client side predicted (push and teleport).
	{"trigger_always", SP_trigger_always},
	{"trigger_multiple", SP_trigger_multiple},
	{"trigger_push", SP_trigger_push},
	{"trigger_teleport", SP_trigger_teleport},
	{"trigger_hurt", SP_trigger_hurt},

	// targets perform no action by themselves, but must be triggered
	// by another entity
	{"target_give", SP_target_give},
	{"target_remove_powerups", SP_target_remove_powerups},
	{"target_delay", SP_target_delay},
	{"target_speaker", SP_target_speaker},
	{"target_print", SP_target_print},
	{"target_laser", SP_target_laser},
	{"target_score", SP_target_score},
	{"target_teleporter", SP_target_teleporter},
	{"target_relay", SP_target_relay},
	{"target_kill", SP_target_kill},
	{"target_position", SP_target_position},
	{"target_location", SP_target_location},
	{"target_push", SP_target_push},

	{"light", SP_light},
	{"path_corner", SP_path_corner},

	{"misc_teleporter_dest", SP_misc_teleporter_dest},
	{"misc_model", SP_misc_model},
	{"misc_G2model", SP_misc_G2model},
	{"misc_portal_surface", SP_misc_portal_surface},
	{"misc_portal_camera", SP_misc_portal_camera},

	{"misc_shield_floor_unit", SP_misc_shield_floor_unit},
	{"misc_model_shield_power_converter", SP_misc_model_shield_power_converter},
	{"misc_model_ammo_power_converter", SP_misc_model_ammo_power_converter},
	{"misc_model_health_power_converter", SP_misc_model_health_power_converter},

	{"fx_runner", SP_fx_runner},
#ifdef ANIMENT_SPAWNER
	{"misc_animent_spawner", SP_misc_animent_spawner},
	{"target_screenshake", SP_target_screenshake},
	{"target_escapetrig", SP_target_escapetrig},
#endif

	{"misc_holocron", SP_misc_holocron},

	{"shooter_blaster", SP_shooter_blaster},

	{"team_CTF_redplayer", SP_team_CTF_redplayer},
	{"team_CTF_blueplayer", SP_team_CTF_blueplayer},

	{"team_CTF_redspawn", SP_team_CTF_redspawn},
	{"team_CTF_bluespawn", SP_team_CTF_bluespawn},

	{"item_botroam", SP_item_botroam},

	{"emplaced_gun", SP_emplaced_gun},

	{0, 0}
};

/*
===============
G_IsSpawnEntity
===============
*/
qboolean G_IsSpawnEntity( const gentity_t *ent ) {
	const spawn_t	*s;

	for (s = spawns; s->name; s++) {
		if (!strcmp(s->name, ent->classname)) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
===============
G_CallSpawn

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/
qboolean G_CallSpawn( gentity_t *ent ) {
	const spawn_t	*s;
	gitem_t			*item;

	if ( !ent->classname ) {
		G_Printf ("G_CallSpawn: NULL classname\n");
		return qfalse;
	}

	// check item spawn functions
	for ( item=bg_itemlist+1 ; item->classname ; item++ ) {
		if ( !strcmp(item->classname, ent->classname) ) {
			if ( GT_Round(level.gametype) )
				return qfalse;
			G_SpawnItem( ent, item );
			return qtrue;
		}
	}

	// check normal spawn functions
	for ( s=spawns ; s->name ; s++ ) {
		if ( !strcmp(s->name, ent->classname) ) {
			// found it
			s->spawn(ent);
			return qtrue;
		}
	}
	G_Printf ("%s doesn't have a spawn function\n", ent->classname);
	return qfalse;
}

/*
=============
G_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
char *G_NewString( const char *string ) {
	char	*newb, *new_p;
	size_t	i,l;

	l = strlen(string) + 1;

	newb = (char *)G_Alloc( l );

	new_p = newb;

	// turn \n into a real linefeed
	for ( i=0 ; i< l ; i++ ) {
		if (string[i] == '\\' && i < l-1) {
			i++;
			if (string[i] == 'n') {
				*new_p++ = '\n';
			} else {
				*new_p++ = '\\';
			}
		} else {
			*new_p++ = string[i];
		}
	}

	return newb;
}




/*
===============
G_ParseField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
void G_ParseField( const char *key, const char *value, gentity_t *ent ) {
	const field_t	*f;
	byte	*b;
	float	v;
	vec3_t	vec;

	for ( f=fields ; f->name ; f++ ) {
		if ( !Q_stricmp(f->name, key) ) {
			// found it
			b = (byte *)ent;

			switch( f->type ) {
			case F_LSTRING:
				*(char **)(b+f->ofs) = G_NewString (value);
				break;
			case F_VECTOR:
				sscanf (value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
				((float *)(b+f->ofs))[0] = vec[0];
				((float *)(b+f->ofs))[1] = vec[1];
				((float *)(b+f->ofs))[2] = vec[2];
				break;
			case F_INT:
				*(int *)(b+f->ofs) = atoi(value);
				break;
			case F_FLOAT:
				*(float *)(b+f->ofs) = atof(value);
				break;
			case F_ANGLEHACK:
				v = atof(value);
				((float *)(b+f->ofs))[0] = 0;
				((float *)(b+f->ofs))[1] = v;
				((float *)(b+f->ofs))[2] = 0;
				break;
			default:
			case F_IGNORE:
				break;
			}
			return;
		}
	}
}




/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/
void G_SpawnGEntityFromSpawnVars( void ) {
	int			i;
	gentity_t	*ent;
	char		*s, *value;

	// get the next free entity
	ent = G_Spawn( ENTITYNUM_WORLD );

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		G_ParseField( level.spawnVars[i][0], level.spawnVars[i][1], ent );
	}

	// check for "notsingle" flag
	if ( level.gametype == GT_SINGLE_PLAYER ) {
		G_SpawnInt( "notsingle", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	}
	// check for "notteam" flag (GT_FFA, GT_TOURNAMENT, GT_SINGLE_PLAYER)
	if ( GT_Team(level.gametype) ) {
		G_SpawnInt( "notteam", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	} else {
		G_SpawnInt( "notfree", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	}

	G_SpawnInt( "notta", "0", &i );
	if ( i ) {
		G_FreeEntity( ent );
		return;
	}

	if( G_SpawnString( "gametype", NULL, &value ) ) {
		s = Q_stristr( value, gametypeShort[level.gametype] );
		if( !s ) {
			G_FreeEntity( ent );
			return;
		}
	}

	// move editor origin to pos
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	// if we didn't get a classname, don't bother spawning anything
	if ( !G_CallSpawn( ent ) ) {
		G_FreeEntity( ent );
	}
}



/*
====================
G_AddSpawnVarToken
====================
*/
char *G_AddSpawnVarToken( const char *string ) {
	int		l;
	char	*dest;

	l = strlen( string );
	if ( level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		G_Error( "G_AddSpawnVarToken: MAX_SPAWN_CHARS" );
	}

	dest = level.spawnVarChars + level.numSpawnVarChars;
	memcpy( dest, string, l+1 );

	level.numSpawnVarChars += l + 1;

	return dest;
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean G_ParseSpawnVars( void ) {
	char		keyname[MAX_TOKEN_CHARS];
	char		com_token[MAX_TOKEN_CHARS];

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	// parse the opening brace
	if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
		// end of spawn string
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		G_Error( "G_ParseSpawnVars: found %s when expecting {",com_token );
	}

	// go through all the key / value pairs
	while ( 1 ) {
		// parse key
		if ( !trap_GetEntityToken( keyname, sizeof( keyname ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( keyname[0] == '}' ) {
			break;
		}

		// parse value
		if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( com_token[0] == '}' ) {
			G_Error( "G_ParseSpawnVars: closing brace without data" );
		}
		if ( level.numSpawnVars == MAX_SPAWN_VARS ) {
			G_Error( "G_ParseSpawnVars: MAX_SPAWN_VARS" );
		}
		level.spawnVars[ level.numSpawnVars ][0] = G_AddSpawnVarToken( keyname );
		level.spawnVars[ level.numSpawnVars ][1] = G_AddSpawnVarToken( com_token );
		level.numSpawnVars++;
	}

	return qtrue;
}

static int QDECL SortStrcmp( const void *a, const void *b )
{
	return strcmp(*(const char **)a, *(const char **) b);
}

static int G_AddModes( char *cs, int nummodes, const char *dir ) {
	char	*modes[MAX_MODES];
	char	filelist[MAX_INFO_STRING * 3];
	char	*file;
	char	realDir[MAX_QPATH];
	int		numfiles;
	int		i;

	Com_sprintf(realDir, sizeof(realDir), "modes/%s", dir);

	numfiles = trap_FS_GetFileList( realDir, "/", filelist, sizeof(filelist) );
	file = filelist;

	for (i = 0; i < numfiles; i++) {
		modes[i] = file;
		file += strlen(file) + 1;
	}

	qsort( modes, numfiles, sizeof( modes[0] ), SortStrcmp );

	for (i = 0; i < numfiles; i++) {
		if (strcmp(modes[i], "") && strcmp(modes[i], ".") && strcmp(modes[i], "..")) {
			char	subDir[MAX_QPATH];

			Q_strncpyz(subDir, dir, sizeof(subDir));
			Q_strcat(subDir, sizeof(subDir), modes[i]);
			Q_strcat(subDir, sizeof(subDir), "/");
			nummodes = G_AddModes(cs, nummodes, subDir);
		}
	}

	numfiles = trap_FS_GetFileList( realDir, ".cfg", filelist, sizeof(filelist) );
	file = filelist;

	if (numfiles > MAX_MODES - nummodes) {
		numfiles = MAX_MODES - nummodes;
	}

	for ( i = 0; i < numfiles; i++ ) {
		int len = strlen( file );

		file[len - 4] = '\\'; // strip extension and use '\\' as separator
		file[len - 3] = '\0';
		modes[i] = file;
		file += len + 1;
	}

	qsort( modes, numfiles, sizeof( modes[0] ), SortStrcmp );

	for (i = 0; i < numfiles; i++) {
		Q_strcat(cs, MAX_INFO_STRING, dir);
		Q_strcat(cs, MAX_INFO_STRING, modes[i]);
	}

	return nummodes + numfiles;
}

static const char * const defaultStyles[32][3] =
{
	{	// 0 normal
		"z",
		"z",
		"z"
	},
	{	// 1 FLICKER (first variety)
		"mmnmmommommnonmmonqnmmo",
		"mmnmmommommnonmmonqnmmo",
		"mmnmmommommnonmmonqnmmo"
	},
	{	// 2 SLOW STRONG PULSE
		"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcb",
		"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcb",
		"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcb"
	},
	{	// 3 CANDLE (first variety)
		"mmmmmaaaaammmmmaaaaaabcdefgabcdefg",
		"mmmmmaaaaammmmmaaaaaabcdefgabcdefg",
		"mmmmmaaaaammmmmaaaaaabcdefgabcdefg"
	},
	{	// 4 FAST STROBE
		"mamamamamama",
		"mamamamamama",
		"mamamamamama"
	},
	{	// 5 GENTLE PULSE 1
		"jklmnopqrstuvwxyzyxwvutsrqponmlkj",
		"jklmnopqrstuvwxyzyxwvutsrqponmlkj",
		"jklmnopqrstuvwxyzyxwvutsrqponmlkj"
	},
	{	// 6 FLICKER (second variety)
		"nmonqnmomnmomomno",
		"nmonqnmomnmomomno",
		"nmonqnmomnmomomno"
	},
	{	// 7 CANDLE (second variety)
		"mmmaaaabcdefgmmmmaaaammmaamm",
		"mmmaaaabcdefgmmmmaaaammmaamm",
		"mmmaaaabcdefgmmmmaaaammmaamm"
	},
	{	// 8 CANDLE (third variety)
		"mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa",
		"mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa",
		"mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa"
	},
	{	// 9 SLOW STROBE (fourth variety)
		"aaaaaaaazzzzzzzz",
		"aaaaaaaazzzzzzzz",
		"aaaaaaaazzzzzzzz"
	},
	{	// 10 FLUORESCENT FLICKER
		"mmamammmmammamamaaamammma",
		"mmamammmmammamamaaamammma",
		"mmamammmmammamamaaamammma"
	},
	{	// 11 SLOW PULSE NOT FADE TO BLACK
		"abcdefghijklmnopqrrqponmlkjihgfedcba",
		"abcdefghijklmnopqrrqponmlkjihgfedcba",
		"abcdefghijklmnopqrrqponmlkjihgfedcba"
	},
	{	// 12 FAST PULSE FOR JEREMY
		"mkigegik",
		"mkigegik",
		"mkigegik"
	},
	{	// 13 Test Blending
		"abcdefghijklmqrstuvwxyz",
		"zyxwvutsrqmlkjihgfedcba",
		"aammbbzzccllcckkffyyggp"
	},
	{	// 14
		"",
		"",
		""
	},
	{	// 15
		"",
		"",
		""
	},
	{	// 16
		"",
		"",
		""
	},
	{	// 17
		"",
		"",
		""
	},
	{	// 18
		"",
		"",
		""
	},
	{	// 19
		"",
		"",
		""
	},
	{	// 20
		"",
		"",
		""
	},
	{	// 21
		"",
		"",
		""
	},
	{	// 22
		"",
		"",
		""
	},
	{	// 23
		"",
		"",
		""
	},
	{	// 24
		"",
		"",
		""
	},
	{	// 25
		"",
		"",
		""
	},
	{	// 26
		"",
		"",
		""
	},
	{	// 27
		"",
		"",
		""
	},
	{	// 28
		"",
		"",
		""
	},
	{	// 29
		"",
		"",
		""
	},
	{	// 30
		"",
		"",
		""
	},
	{	// 31
		"",
		"",
		""
	}
};

void *precachedKyle = 0;

/*QUAKED worldspawn (0 0 0) ?

Every map should have exactly one worldspawn.
"music"		music wav file
"gravity"	800 is default gravity
"message"	Text to print during connection process

BSP Options
"gridsize"     size of lighting grid to "X Y Z". default="64 64 128"
"ambient"      scale of global light (from _color)
"fog"          shader name of the global fog texture - must include the full path, such as "textures/rj/fog1"
"distancecull" value for vis for the maximum viewing distance
"chopsize"     value for bsp on the maximum polygon / portal size
"ls_Xr"	override lightstyle X with this pattern for Red.
"ls_Xg"	green (valid patterns are "a-z")
"ls_Xb"	blue (a is OFF, z is ON)
*/
void SP_worldspawn( void )
{
	char		*text, temp[32];
	int			i;
	int			lengthRed, lengthBlue, lengthGreen;

	G_SpawnString( "classname", "", &text );
	if ( Q_stricmp( text, "worldspawn" ) ) {
		G_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}

	//The server will precache the standard model and animations, so that there is no hit
	//when the first client connnects.
	if (!BGPAFtextLoaded)
	{
		BG_ParseAnimationFile("models/players/_humanoid/animation.cfg");
	}

	if (!precachedKyle)
	{
		trap_G2API_InitGhoul2Model(&precachedKyle, "models/players/kyle/model.glm", 0, 0, -20, 0, 0);
	}

	if (!g2SaberInstance)
	{
		trap_G2API_InitGhoul2Model(&g2SaberInstance, "models/weapons2/saber/saber_w.glm", 0, 0, -20, 0, 0);

		if (g2SaberInstance)
		{
			// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
			trap_G2API_SetBoltInfo(g2SaberInstance, 0, 0);
			// now set up the gun bolt on it
			trap_G2API_AddBolt(g2SaberInstance, 0, "*flash");
		}
	}

	// make some data visible to connecting client
	trap_SetConfigstring( CS_GAME_VERSION, "basejk-1" );

	trap_SetConfigstring( CS_LEVEL_START_TIME, va("%i", level.startTime ) );

	G_SpawnString( "music", "", &text );
	trap_SetConfigstring( CS_MUSIC, text );

	G_SpawnString( "message", "", &text );
	trap_SetConfigstring( CS_MESSAGE, text );				// map specific message

	trap_SetConfigstring( CS_MOTD, g_motd.string );		// message of the day

	trap_SetConfigstring( CS_ROUND, "0");

	G_SpawnString( "gravity", "800", &text );
	trap_Cvar_Set( "g_gravity", text );

	G_SpawnString( "enableDust", "0", &text );
	trap_Cvar_Set( "g_enableDust", text );

	G_SpawnString( "enableBreath", "0", &text );
	trap_Cvar_Set( "g_enableBreath", text );

	g_entities[ENTITYNUM_WORLD].s.number = ENTITYNUM_WORLD;
	g_entities[ENTITYNUM_WORLD].classname = "worldspawn";

	// see if we want a warmup time
	trap_SetConfigstring( CS_WARMUP, "" );
	if ( g_restarted.integer ) {
		trap_Cvar_Set( "g_restarted", "0" );
		level.warmupTime = 0;

		if ( g_doWarmup.integer && level.gametype != GT_TOURNAMENT ) {
			trap_Cvar_Set( "g_status", GAMESTATUS_MATCH );
		} else {
			trap_Cvar_Set( "g_status", GAMESTATUS_DEFAULT );
		}
	} else if ( g_doWarmup.integer && level.gametype != GT_TOURNAMENT ) { // Turn it on
		level.warmupTime = -1;
		trap_Cvar_Set( "g_status", GAMESTATUS_WARMUP );
		trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
		G_LogPrintf( LOG_GAME, "Warmup:\n" );
	}

	trap_SetConfigstring( CS_SCORES1, "" );
	trap_SetConfigstring( CS_SCORES2, "" );

	trap_SetConfigstring( CS_VOTE_TIME, "" );
	trap_SetConfigstring( CS_VOTE_STRING, "" );
	trap_SetConfigstring( CS_VOTE_YES, "" );
	trap_SetConfigstring( CS_VOTE_NO, "" );

	for (i = 0; i < 2; i++)
	{
		trap_SetConfigstring( CS_TEAMVOTE_TIME + i, "" );
		trap_SetConfigstring( CS_TEAMVOTE_STRING + i, "" );
		trap_SetConfigstring( CS_TEAMVOTE_YES + i, "" );
		trap_SetConfigstring( CS_TEAMVOTE_NO + i, "" );
	}

	trap_SetConfigstring( CS_INTERMISSION, "" );
	trap_SetConfigstring( CS_FLAGSTATUS, "" );
	trap_SetConfigstring( CS_SHADERSTATE, "" );

	trap_SetConfigstring(CS_LIGHT_STYLES+(LS_STYLES_START*3)+0, defaultStyles[0][0]);
	trap_SetConfigstring(CS_LIGHT_STYLES+(LS_STYLES_START*3)+1, defaultStyles[0][1]);
	trap_SetConfigstring(CS_LIGHT_STYLES+(LS_STYLES_START*3)+2, defaultStyles[0][2]);

	for(i=1;i<LS_NUM_STYLES;i++)
	{
		Com_sprintf(temp, sizeof(temp), "ls_%dr", i);
		G_SpawnString(temp, defaultStyles[i][0], &text);
		lengthRed = strlen(text);
		trap_SetConfigstring(CS_LIGHT_STYLES+((i+LS_STYLES_START)*3)+0, text);

		Com_sprintf(temp, sizeof(temp), "ls_%dg", i);
		G_SpawnString(temp, defaultStyles[i][1], &text);
		lengthGreen = strlen(text);
		trap_SetConfigstring(CS_LIGHT_STYLES+((i+LS_STYLES_START)*3)+1, text);

		Com_sprintf(temp, sizeof(temp), "ls_%db", i);
		G_SpawnString(temp, defaultStyles[i][2], &text);
		lengthBlue = strlen(text);
		trap_SetConfigstring(CS_LIGHT_STYLES+((i+LS_STYLES_START)*3)+2, text);

		if (lengthRed != lengthGreen || lengthGreen != lengthBlue)
		{
			Com_Error(ERR_DROP, "Style %d has inconsistent lengths: R %d, G %d, B %d",
				i, lengthRed, lengthGreen, lengthBlue);
		}
	}

	trap_SetConfigstring( CS_READY, "" );
	trap_SetConfigstring( CS_UNPAUSE, "" );

	// write available modes to CS_MODES
	{

		char	cs[MAX_INFO_STRING];

		cs[0] = '\0';

		G_AddModes( cs, 0, "" );

		if ( strlen( cs ) + 1 >= sizeof( cs ) )
			Com_Printf( S_COLOR_YELLOW "WARNING: Too many modes. Delete some or use shorter names.\n" );

		trap_SetConfigstring( CS_MODES, cs );
	}
}


/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void G_SpawnEntitiesFromString( void ) {
	// allow calls to G_Spawn*()
	level.spawning = qtrue;
	level.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !G_ParseSpawnVars() ) {
		G_Error( "SpawnEntities: no entities" );
	}
	SP_worldspawn();

	// parse ents
	while( G_ParseSpawnVars() ) {
		G_SpawnGEntityFromSpawnVars();
	}

	level.spawning = qfalse;			// any future calls to G_Spawn*() will be errors
}

