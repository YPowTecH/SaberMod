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

// g_bot.c

#include "g_local.h"


static int		g_numBots;
static char		*g_botInfos[MAX_BOTS];


static int		g_numArenas;
static char		*g_arenaInfos[MAX_ARENAS];


#define BOT_BEGIN_DELAY_BASE		2000
#define BOT_BEGIN_DELAY_INCREMENT	1500

#define BOT_SPAWN_QUEUE_DEPTH	16

typedef struct {
	int		clientNum;
	int		spawnTime;
} botSpawnQueue_t;

//static int			botBeginDelay = 0;  // bk001206 - unused, init
static botSpawnQueue_t	botSpawnQueue[BOT_SPAWN_QUEUE_DEPTH];

vmCvar_t bot_minplayers;

extern gentity_t	*podium1;
extern gentity_t	*podium2;
extern gentity_t	*podium3;

float trap_Cvar_VariableValue( const char *var_name ) {
	char buf[128];

	trap_Cvar_VariableStringBuffer(var_name, buf, sizeof(buf));
	return atof(buf);
}



/*
===============
G_ParseInfos
===============
*/
int G_ParseInfos( char *buf, int max, char *infos[] ) {
	char	*token;
	int		count;
	char	key[MAX_TOKEN_CHARS];
	char	info[MAX_INFO_STRING];

	count = 0;

	while ( 1 ) {
		token = COM_Parse( (const char **)(&buf) );
		if ( !token[0] ) {
			break;
		}
		if ( strcmp( token, "{" ) != 0 ) {
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if ( count == max ) {
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		info[0] = '\0';
		while ( 1 ) {
			token = COM_ParseExt( (const char **)(&buf), qtrue );
			if ( !token[0] ) {
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}
			if ( !strcmp( token, "}" ) ) {
				break;
			}
			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( (const char **)(&buf), qfalse );
			if ( !token[0] ) {
				strcpy( token, "<NULL>" );
			}
			Info_SetValueForKey( info, key, token );
		}
		//NOTE: extra space for arena number
		infos[count] = (char *)G_Alloc(strlen(info) + strlen("\\num\\") + strlen(va("%d", MAX_ARENAS)) + 1);
		if (infos[count]) {
			strcpy(infos[count], info);
			count++;
		}
	}
	return count;
}

/*
===============
G_LoadArenasFromFile
===============
*/
static void G_LoadArenasFromFile( const char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[MAX_ARENAS_TEXT];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_ARENAS_TEXT ) {
		trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_ARENAS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	g_numArenas += G_ParseInfos( buf, MAX_ARENAS - g_numArenas, &g_arenaInfos[g_numArenas] );
}

static int G_GetMapTypeBits(const char *type)
{
	int typeBits = 0;

	if( *type ) {
		if( strstr( type, "ffa" ) ) {
			typeBits |= (1 << GT_FFA);
			typeBits |= (1 << GT_TEAM);
			typeBits |= (1 << GT_REDROVER);
			typeBits |= (1 << GT_CLANARENA);
		}
		if( strstr( type, "holocron" ) ) {
			typeBits |= (1 << GT_HOLOCRON);
		}
		if( strstr( type, "jedimaster" ) ) {
			typeBits |= (1 << GT_JEDIMASTER);
		}
		if( strstr( type, "duel" ) ) {
			typeBits |= (1 << GT_TOURNAMENT);
		}
		if( strstr( type, "saga" ) ) {
			typeBits |= (1 << GT_SAGA);
		}
		if( strstr( type, "ctf" ) ) {
			typeBits |= (1 << GT_CTF);
		}
		if( strstr( type, "cty" ) ) {
			typeBits |= (1 << GT_CTY);
		}
	} else {
		typeBits |= (1 << GT_FFA);
	}

	return typeBits;
}

qboolean G_DoesMapSupportGametype(const char *mapname, gametype_t gametype)
{
	arena_t		arena;

	arena = G_GetArenaByMap( mapname );
	return G_DoesArenaSupportGametype( arena, gametype );
}

qboolean G_DoesArenaSupportGametype(arena_t arena, gametype_t gametype)
{
	int			typeBits = 0;
	const char	*type;

	if (arena < 0 || arena >= g_numArenas)
	{
		return qfalse;
	}

	type = Info_ValueForKey(g_arenaInfos[arena], "type");

	typeBits = G_GetMapTypeBits(type);
	if (typeBits & (1 << gametype))
	{ //the map in question supports the gametype in question, so..
		return qtrue;
	}

	return qfalse;
}

//rww - auto-obtain nextmap. I could've sworn Q3 had something like this, but I guess not.
const char *G_RefreshNextMap(int gametype, qboolean forced)
{
	int			typeBits = 0;
	int			thisLevel = 0;
	int			desiredMap = 0;
	int			n = 0;
	const char		*type;
	qboolean	loopingUp = qfalse;
	char		mapname[MAX_CVAR_VALUE_STRING];

	if (!g_autoMapCycle.integer && !forced)
	{
		return NULL;
	}

	if (!g_arenaInfos[0])
	{
		return NULL;
	}

	trap_Cvar_VariableStringBuffer( "mapname", mapname, sizeof( mapname ) );
	for( n = 0; n < g_numArenas; n++ )
	{
		type = Info_ValueForKey( g_arenaInfos[n], "map" );

		if (Q_stricmp(mapname, type) == 0)
		{
			thisLevel = n;
			break;
		}
	}

	desiredMap = thisLevel;

	n = thisLevel+1;
	while (n != thisLevel)
	{ //now cycle through the arena list and find the next map that matches the gametype we're in
		if (!g_arenaInfos[n] || n >= g_numArenas)
		{
			if (loopingUp)
			{ //this shouldn't happen, but if it does we have a null entry break in the arena file
			  //if this is the case just break out of the loop instead of sticking in an infinite loop
				break;
			}
			n = 0;
			loopingUp = qtrue;
		}

		type = Info_ValueForKey(g_arenaInfos[n], "type");

		typeBits = G_GetMapTypeBits(type);
		if (typeBits & (1 << gametype))
		{
			desiredMap = n;
			break;
		}

		n++;
	}

	if (desiredMap == thisLevel)
	{ //If this is the only level for this game mode or we just can't find a map for this game mode, then nextmap
	  //will always restart.
		trap_Cvar_Set( "nextmap", "map_restart 0");
	}
	else
	{ //otherwise we have a valid nextmap to cycle to, so use it.
		type = Info_ValueForKey( g_arenaInfos[desiredMap], "map" );
		trap_Cvar_Set( "nextmap", va("map %s", type));
	}

	return Info_ValueForKey( g_arenaInfos[desiredMap], "map" );
}

/*
=================
G_GetMapsConfigstring
=================
*/
static int G_GetMapsConfigstrings( char cs[MAX_CS_MAPS][MAX_INFO_STRING], int max ) {
	char	token[MAX_INFO_VALUE * 2 + 1];
	int		numMaps;
	int		i;
	int		n = 0;
	int		pos = 0;

	for ( i = 0; i < MAX_CS_MAPS; i++ ) {
		cs[i][0] = '\0';
	}

	for ( i = 0, numMaps = 0; i < g_numArenas && numMaps < max; i++ ) {
		const char	*type;
		int			typeBits;

		type = Info_ValueForKey( g_arenaInfos[i], "type" );
		typeBits = G_GetMapTypeBits( type );

		if ( typeBits & (1 << level.gametype) ) {
			const char	*name = Info_ValueForKey( g_arenaInfos[i], "map" );
			const char	*longName = Info_ValueForKey( g_arenaInfos[i], "longname" );
			int			len;

			len = Com_sprintf( token, sizeof( token ), "%s\\%s\\", name, longName );

			while ( pos + len + 1 >= MAX_INFO_STRING ) {
				cs[n][MAX_INFO_STRING - 1] = '\0'; // safe
				pos = 0;
				n++;
				if ( n >= MAX_CS_MAPS ) {
					return -1;
				}
			}

			strncpy( &cs[n][pos], token, len + 1 );
			pos += len;

			numMaps++;
		}
	}

	return numMaps;
}

/*
===============
G_LoadArenas
===============
*/
static void G_LoadArenas( void ) {
	int			numdirs;
	vmCvar_t	arenasFile;
	char		filename[MAX_QPATH];
	char		dirlist[1024];
	char*		dirptr;
	int			i, n;
	int			dirlen;

	g_numArenas = 0;

	trap_Cvar_Register( &arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM );
	if( *arenasFile.string ) {
		G_LoadArenasFromFile(arenasFile.string);
	}
	else {
		G_LoadArenasFromFile("scripts/arenas.txt");
	}

	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList("scripts", ".arena", dirlist, 1024 );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		Q_strcat(filename, sizeof(filename), dirptr);
		G_LoadArenasFromFile(filename);
	}
	trap_Print( va( "%i arenas parsed\n", g_numArenas ) );

	for( n = 0; n < g_numArenas; n++ ) {
		Info_SetValueForKey( g_arenaInfos[n], "num", va( "%i", n ) );
	}

	G_RefreshNextMap(level.gametype, qfalse);

	// write available maps for current gametype to CS_SERVER_MAPS
	{
		char	cs[MAX_CS_MAPS][MAX_INFO_STRING];
		int		numMaps;

		numMaps = G_GetMapsConfigstrings( cs, MAX_SERVER_MAPS );

		if ( numMaps >= MAX_SERVER_MAPS || numMaps == -1 ) {
			Com_Printf( S_COLOR_YELLOW "WARNING: Too many maps for callvote menu.\n" );
		}

		for ( n = 0; n < MAX_CS_MAPS; n++ ) {
			trap_SetConfigstring( CS_MAPS + n, cs[n] );
		}
	}
}

/*
===============
G_GetArenaByMap
===============
*/
arena_t G_GetArenaByMap( const char *map ) {
	int			n;

	if (!map || !map[0]) {
		return ARENA_INVALID;
	}

	for( n = 0; n < g_numArenas; n++ ) {
		if( Q_stricmp( Info_ValueForKey( g_arenaInfos[n], "map" ), map ) == 0 ) {
			return n;
		}
	}

	return ARENA_INVALID;
}

const char *G_GetArenaInfo( arena_t arena ) {
	if ( arena < 0 || arena >= g_numArenas ) {
		return NULL;
	}

	return g_arenaInfos[arena];
}

#ifdef UNUSED
/*
===============
G_GetArenaInfoByMap
===============
*/
const char *G_GetArenaInfoByMap( const char *map ) {
	int			n;

	for( n = 0; n < g_numArenas; n++ ) {
		if( Q_stricmp( Info_ValueForKey( g_arenaInfos[n], "map" ), map ) == 0 ) {
			return g_arenaInfos[n];
		}
	}

	return NULL;
}

/*
=================
PlayerIntroSound
=================
*/
static void PlayerIntroSound( const char *modelAndSkin ) {
	char	model[MAX_QPATH];
	char	*skin;

	Q_strncpyz( model, modelAndSkin, sizeof(model) );
	skin = strrchr( model, '/' );
	if ( skin ) {
		*skin++ = '\0';
	}
	else {
		skin = model;
	}

	if( Q_stricmp( skin, "default" ) == 0 ) {
		skin = model;
	}

	trap_SendConsoleCommand( EXEC_APPEND, va( "play sound/player/announce/%s.wav\n", skin ) );
}
#endif // UNUSED

/*
===============
G_AddRandomBot
===============
*/
void G_AddRandomBot( int team ) {
	int		i, n, num;
	float	skill;
	const char	*value;
	const char	*teamstr;
	char		netname[MAX_NETNAME];
	gclient_t	*cl;

	num = 0;
	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "name" );
		//
		for ( i=0 ; i< level.maxclients ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected == CON_DISCONNECTED ) {
				continue;
			}
			if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
				continue;
			}
			if ( team >= 0 && cl->sess.sessionTeam != (team_t)team ) {
				continue;
			}
			if ( !Q_stricmp( value, cl->info.netname ) ) {
				break;
			}
		}
		if (i >= level.maxclients) {
			num++;
		}
	}
	num = random() * num;
	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "name" );
		//
		for ( i=0 ; i< level.maxclients ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected == CON_DISCONNECTED ) {
				continue;
			}
			if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
				continue;
			}
			if ( team >= 0 && cl->sess.sessionTeam != (team_t)team ) {
				continue;
			}
			if ( !Q_stricmp( value, cl->info.netname ) ) {
				break;
			}
		}
		if (i >= level.maxclients) {
			num--;
			if (num <= 0) {
				skill = g_spSkill.value;
				if (team == TEAM_RED) teamstr = "red";
				else if (team == TEAM_BLUE) teamstr = "blue";
				else teamstr = "";
				Q_strncpyz(netname, value, sizeof(netname));
				Q_CleanStr(netname);
				trap_SendConsoleCommand( EXEC_INSERT, va("addbot %s %f %s %i", netname, skill, teamstr, 0) );
				return;
			}
		}
	}
}

/*
===============
G_RemoveRandomBot
===============
*/
int G_RemoveRandomBot( int team ) {
	int i;
	gclient_t	*cl;

	for ( i=0 ; i< level.maxclients ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != (team_t)team ) {
			continue;
		}
		trap_SendConsoleCommand( EXEC_INSERT, va("clientkick %d", i) );
		return qtrue;
	}
	return qfalse;
}

/*
===============
G_CountHumanPlayers
===============
*/
int G_CountHumanPlayers( int team ) {
	int i, num;
	gclient_t	*cl;

	num = 0;
	for ( i=0 ; i< level.maxclients ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( g_entities[i].r.svFlags & SVF_BOT ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != (team_t)team ) {
			continue;
		}
		num++;
	}
	return num;
}

/*
===============
G_CountBotPlayers
===============
*/
int G_CountBotPlayers( int team ) {
	int i, n, num;
	gclient_t	*cl;

	num = 0;
	for ( i=0 ; i< level.maxclients ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != (team_t)team ) {
			continue;
		}
		num++;
	}
	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			continue;
		}
		if ( botSpawnQueue[n].spawnTime > level.time ) {
			continue;
		}
		num++;
	}
	return num;
}

/*
===============
G_CheckMinimumPlayers
===============
*/
void G_CheckMinimumPlayers( void ) {
	int minplayers;
	int humanplayers, botplayers;
	static int checkminimumplayers_time;

	if (level.intermissiontime) return;
	//only check once each 10 seconds
	if (checkminimumplayers_time > level.time - 10000) {
		return;
	}
	checkminimumplayers_time = level.time;
	trap_Cvar_Update(&bot_minplayers);
	minplayers = bot_minplayers.integer;
	if (minplayers <= 0) return;
	if (g_teamsize.integer && minplayers >= g_teamsize.integer) {
		minplayers = g_teamsize.integer;
	}

	switch (level.gametype) {
	case GT_TEAM:
	case GT_CTF:
	case GT_CTY:
	case GT_CLANARENA:
		if (minplayers >= level.maxclients / 2) {
			minplayers = (level.maxclients / 2) -1;
		}

		humanplayers = G_CountHumanPlayers( TEAM_RED );
		botplayers = G_CountBotPlayers(	TEAM_RED );
		//
		if (humanplayers + botplayers < minplayers && !level.teamLock[TEAM_RED]) {
			G_AddRandomBot( TEAM_RED );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( TEAM_RED );
		}
		//
		humanplayers = G_CountHumanPlayers( TEAM_BLUE );
		botplayers = G_CountBotPlayers( TEAM_BLUE );
		//
		if (humanplayers + botplayers < minplayers && !level.teamLock[TEAM_BLUE]) {
			G_AddRandomBot( TEAM_BLUE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( TEAM_BLUE );
		}
		break;
	case GT_TOURNAMENT:
		if (minplayers >= level.maxclients) {
			minplayers = level.maxclients-1;
		}
		humanplayers = G_CountHumanPlayers( -1 );
		botplayers = G_CountBotPlayers( -1 );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( TEAM_FREE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			// try to remove spectators first
			if (!G_RemoveRandomBot( TEAM_SPECTATOR )) {
				// just remove the bot that is playing
				G_RemoveRandomBot( -1 );
			}
		}
		break;
	case GT_FFA:
	case GT_HOLOCRON:
	case GT_JEDIMASTER:
	default:
		if (minplayers >= level.maxclients) {
			minplayers = level.maxclients-1;
		}
		humanplayers = G_CountHumanPlayers( TEAM_FREE );
		botplayers = G_CountBotPlayers( TEAM_FREE );
		//
		if (humanplayers + botplayers < minplayers && !level.teamLock[TEAM_FREE]) {
			G_AddRandomBot( TEAM_FREE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( TEAM_FREE );
		}
		break;
	case GT_REDROVER:
		minplayers *= 2;
		if (minplayers >= level.maxclients) {
			minplayers = level.maxclients-1;
		}
		humanplayers = G_CountHumanPlayers( TEAM_RED ) + G_CountHumanPlayers( TEAM_BLUE );
		botplayers = G_CountBotPlayers( TEAM_RED ) + G_CountBotPlayers( TEAM_BLUE );
		//
		if (humanplayers + botplayers < minplayers && !level.teamLock[TEAM_RED] && !level.teamLock[TEAM_BLUE]) {
			G_AddRandomBot( -1 );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( -1 );
		}
		break;
	}
}

/*
===============
G_CheckBotSpawn
===============
*/
void G_CheckBotSpawn( void ) {
	int		n;

	G_CheckMinimumPlayers();

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			continue;
		}
		if ( botSpawnQueue[n].spawnTime > level.time ) {
			continue;
		}
		ClientBegin( botSpawnQueue[n].clientNum, qfalse );
		botSpawnQueue[n].spawnTime = 0;

		/*
		if( level.gametype == GT_SINGLE_PLAYER ) {
			trap_GetUserinfo( botSpawnQueue[n].clientNum, userinfo, sizeof(userinfo) );
			PlayerIntroSound( Info_ValueForKey (userinfo, "model") );
		}
		*/
	}
}


/*
===============
AddBotToSpawnQueue
===============
*/
static void AddBotToSpawnQueue( int clientNum, int delay ) {
	int		n;

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			botSpawnQueue[n].spawnTime = level.time + delay;
			botSpawnQueue[n].clientNum = clientNum;
			return;
		}
	}

	G_Printf( S_COLOR_YELLOW "Unable to delay spawn\n" );
	ClientBegin( clientNum, qfalse );
}


/*
===============
G_RemoveQueuedBotBegin

Called on client disconnect to make sure the delayed spawn
doesn't happen on a freed index
===============
*/
void G_RemoveQueuedBotBegin( int clientNum ) {
	int		n;

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( botSpawnQueue[n].clientNum == clientNum ) {
			botSpawnQueue[n].spawnTime = 0;
			return;
		}
	}
}


/*
===============
G_BotConnect
===============
*/
qboolean G_BotConnect( int clientNum, qboolean restart ) {
	bot_settings_t	settings;
	char			userinfo[MAX_INFO_STRING];

	trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );

	Q_strncpyz( settings.personalityfile, Info_ValueForKey( userinfo, "personality" ), sizeof(settings.personalityfile) );
	settings.skill = atof( Info_ValueForKey( userinfo, "skill" ) );
	Q_strncpyz( settings.team, Info_ValueForKey( userinfo, "team" ), sizeof(settings.team) );

	if (!BotAISetupClient( clientNum, &settings, restart )) {
		trap_DropClient( clientNum, "BotAISetupClient failed" );
		return qfalse;
	}

	return qtrue;
}


/*
===============
G_AddBot
===============
*/
static void G_AddBot( const char *name, float skill, const char *team, int delay, const char *altname) {
	int				clientNum;
	const char		*botinfo;
	const char		*key;
	const char		*s;
	const char		*botname;
	const char		*model;
//	const char		*headmodel;
	char			userinfo[MAX_INFO_STRING];

	// get the botinfo from bots.txt
	botinfo = G_GetBotInfoByName( name );
	if ( !botinfo ) {
		G_Printf( S_COLOR_RED "Error: Bot '%s' not defined\n", name );
		return;
	}

	// create the bot's userinfo
	userinfo[0] = '\0';

	botname = Info_ValueForKey( botinfo, "funname" );
	if( !botname[0] ) {
		botname = Info_ValueForKey( botinfo, "name" );
	}
	// check for an alternative name
	if (altname && altname[0]) {
		botname = altname;
	}
	Info_SetValueForKey( userinfo, "name", botname );
	Info_SetValueForKey( userinfo, "rate", "25000" );
	Info_SetValueForKey( userinfo, "snaps", "20" );
	Info_SetValueForKey( userinfo, "skill", va("%1.2f", skill) );

	/*
	if ( skill >= 1 && skill < 2 ) {
		Info_SetValueForKey( userinfo, "handicap", "50" );
	}
	else if ( skill >= 2 && skill < 3 ) {
		Info_SetValueForKey( userinfo, "handicap", "70" );
	}
	else if ( skill >= 3 && skill < 4 ) {
		Info_SetValueForKey( userinfo, "handicap", "90" );
	}
	*/

	key = "model";
	model = Info_ValueForKey( botinfo, key );
	if ( !*model ) {
		model = "visor/default";
	}
	Info_SetValueForKey( userinfo, key, model );
	key = "team_model";
	Info_SetValueForKey( userinfo, key, model );

/*	key = "headmodel";
	headmodel = Info_ValueForKey( botinfo, key );
	if ( !*headmodel ) {
		headmodel = model;
	}
	Info_SetValueForKey( userinfo, key, headmodel );
	key = "team_headmodel";
	Info_SetValueForKey( userinfo, key, headmodel );
*/
	key = "gender";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "male";
	}
	Info_SetValueForKey( userinfo, "sex", s );

	key = "color1";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "4";
	}
	Info_SetValueForKey( userinfo, key, s );

	key = "color2";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "5";
	}
	Info_SetValueForKey( userinfo, key, s );

	s = Info_ValueForKey(botinfo, "personality");
	if (!*s )
	{
		Info_SetValueForKey( userinfo, "personality", "botfiles/default.jkb" );
	}
	else
	{
		Info_SetValueForKey( userinfo, "personality", s );
	}

	// have the server allocate a client slot
	clientNum = trap_BotAllocateClient();
	if ( clientNum == -1 ) {
//		G_Printf( S_COLOR_RED "Unable to add bot.  All player slots are in use.\n" );
//		G_Printf( S_COLOR_RED "Start server with more 'open' slots.\n" );
		trap_SendServerCommand( -1, va("print \"%s\"", G_GetStripEdString("SVINGAME", "UNABLE_TO_ADD_BOT")));
		return;
	}

	// initialize the bot settings
//	Info_SetValueForKey( userinfo, "characterfile", Info_ValueForKey( botinfo, "aifile" ) );
	Info_SetValueForKey( userinfo, "skill", va( "%5.2f", skill ) );
	Info_SetValueForKey( userinfo, "team", team );

	// register the userinfo
	trap_SetUserinfo( clientNum, userinfo );

	// have it connect to the game as a normal client
	if ( ClientConnect( clientNum, qtrue, qtrue ) ) {
		return;
	}

	if( delay == 0 ) {
		ClientBegin( clientNum, qfalse );
		return;
	}

	AddBotToSpawnQueue( clientNum, delay );
}


/*
===============
Svcmd_AddBot_f
===============
*/
void Svcmd_AddBot_f( void ) {
	float			skill;
	int				delay;
	char			name[MAX_TOKEN_CHARS];
	char			altname[MAX_TOKEN_CHARS];
	char			string[MAX_TOKEN_CHARS];
	char			team[MAX_TOKEN_CHARS];

	// are bots enabled?
	if ( !trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		return;
	}

	// name
	trap_Argv( 1, name, sizeof( name ) );
	if ( !name[0] ) {
		trap_Print( "Usage: Addbot <botname> [skill 1-5] [team] [msec delay] [altname]\n" );
		return;
	}

	// skill
	trap_Argv( 2, string, sizeof( string ) );
	if ( !string[0] ) {
		skill = 4;
	}
	else {
		skill = atof( string );
	}

	// team
	trap_Argv( 3, team, sizeof( team ) );

	// delay
	trap_Argv( 4, string, sizeof( string ) );
	if ( !string[0] ) {
		delay = 0;
	}
	else {
		delay = atoi( string );
	}

	// alternative name
	trap_Argv( 5, altname, sizeof( altname ) );

	G_AddBot( name, skill, team, delay, altname );

	// if this was issued during gameplay and we are playing locally,
	// go ahead and load the bot's media immediately
	if ( level.time - level.startTime > 1000 &&
		trap_Cvar_VariableIntegerValue( "cl_running" ) ) {
		trap_SendServerCommand( -1, "loaddefered\n" );	// FIXME: spelled wrong, but not changing for demo
	}
}

/*
===============
Svcmd_BotList_f
===============
*/
void Svcmd_BotList_f( void ) {
	int i;
	char name[MAX_NETNAME];
	char funname[MAX_NETNAME];
	char model[MAX_QPATH];
	char personality[MAX_QPATH];

	trap_Print("^1name             model            personality              funname\n");
	for (i = 0; i < g_numBots; i++) {
		Q_strncpyz(name, Info_ValueForKey( g_botInfos[i], "name" ), sizeof(name));
		if ( !*name ) {
			strcpy(name, "Padawan");
		}
		Q_strncpyz(funname, Info_ValueForKey( g_botInfos[i], "funname" ), sizeof(funname));
		if ( !*funname ) {
			strcpy(funname, "");
		}
		Q_strncpyz(model, Info_ValueForKey( g_botInfos[i], "model" ), sizeof(model));
		if ( !*model ) {
			strcpy(model, "visor/default");
		}
		Q_strncpyz(personality, Info_ValueForKey( g_botInfos[i], "personality"), sizeof(personality));
		if (!*personality ) {
			strcpy(personality, "botfiles/default.jkb");
		}
		trap_Print(va("%-16s %-16s %-20s %-20s\n", name, model, personality, funname));
	}
}

#if 0
/*
===============
G_SpawnBots
===============
*/
static void G_SpawnBots( char *botList, int baseDelay ) {
	char		*bot;
	char		*p;
	float		skill;
	int			delay;
	char		bots[MAX_INFO_VALUE];

	podium1 = NULL;
	podium2 = NULL;
	podium3 = NULL;

	skill = g_spSkill.value
	if( skill < 1 ) {
		trap_Cvar_Set( "g_spSkill", "1" );
		skill = 1;
	}
	else if ( skill > 5 ) {
		trap_Cvar_Set( "g_spSkill", "5" );
		skill = 5;
	}

	Q_strncpyz( bots, botList, sizeof(bots) );
	p = &bots[0];
	delay = baseDelay;
	while( *p ) {
		//skip spaces
		while( *p && *p == ' ' ) {
			p++;
		}
		if( !p ) {
			break;
		}

		// mark start of bot name
		bot = p;

		// skip until space of null
		while( *p && *p != ' ' ) {
			p++;
		}
		if( *p ) {
			*p++ = 0;
		}

		// we must add the bot this way, calling G_AddBot directly at this stage
		// does "Bad Things"
		trap_SendConsoleCommand( EXEC_INSERT, va("addbot %s %f free %i", bot, skill, delay) );

		delay += BOT_BEGIN_DELAY_INCREMENT;
	}
}
#endif


/*
===============
G_LoadBotsFromFile
===============
*/
static void G_LoadBotsFromFile( const char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[MAX_BOTS_TEXT];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_BOTS_TEXT ) {
		trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_BOTS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	g_numBots += G_ParseInfos( buf, MAX_BOTS - g_numBots, &g_botInfos[g_numBots] );
}

/*
===============
G_LoadBots
===============
*/
static void G_LoadBots( void ) {
	vmCvar_t	botsFile;
	int			numdirs;
	char		filename[MAX_QPATH];
	char		dirlist[1024];
	char*		dirptr;
	int			i;
	int			dirlen;

	if ( !trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		return;
	}

	g_numBots = 0;

	trap_Cvar_Register( &botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM );
	if( *botsFile.string ) {
		G_LoadBotsFromFile(botsFile.string);
	}
	else {
		//G_LoadBotsFromFile("scripts/bots.txt");
		G_LoadBotsFromFile("botfiles/bots.txt");
	}

	// get all bots from .bot files
	numdirs = trap_FS_GetFileList("scripts", ".bot", dirlist, 1024 );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		Q_strncpyz(filename, "scripts/", sizeof(filename));
		Q_strcat(filename, sizeof(filename), dirptr);
		G_LoadBotsFromFile(filename);
	}
	trap_Print( va( "%i bots parsed\n", g_numBots ) );
}


#ifdef UNUSED
/*
===============
G_GetBotInfoByNumber
===============
*/
char *G_GetBotInfoByNumber( int num ) {
	if( num < 0 || num >= g_numBots ) {
		trap_Print( va( S_COLOR_RED "Invalid bot number: %i\n", num ) );
		return NULL;
	}
	return g_botInfos[num];
}
#endif // UNUSED

/*
===============
G_GetBotInfoByName
===============
*/
char *G_GetBotInfoByName( const char *name ) {
	int			n;
	const char	*value;

	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "name" );
		if ( !Q_stricmp( value, name ) ) {
			return g_botInfos[n];
		}
	}

	return NULL;
}

//rww - pd
void LoadPath_ThisLevel(void);
//end rww

/*
===============
G_InitBots
===============
*/
void G_InitBots( int restart ) {
	G_LoadBots();
	G_LoadArenas();

	trap_Cvar_Register( &bot_minplayers, "bot_minplayers", "0", CVAR_SERVERINFO );

	//rww - new bot route stuff
	LoadPath_ThisLevel();
	//end rww
}
