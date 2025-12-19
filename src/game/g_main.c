	/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * Jump in into the game.so and support functions.
 *
 * =======================================================================
 */

#include "header/local.h"

game_locals_t game;
level_locals_t level;
game_import_t gi;
game_export_t globals;
spawn_temp_t st;

int sm_meat_index;
int snd_fry;
int meansOfDeath;

edict_t *g_edicts;

cvar_t *deathmatch;
cvar_t *coop;
cvar_t *coop_pickup_weapons;
cvar_t *coop_elevator_delay;
cvar_t *dmflags;
cvar_t *skill;
cvar_t *fraglimit;
cvar_t *timelimit;
cvar_t *password;
cvar_t *spectator_password;
cvar_t *needpass;
cvar_t *maxclients;
cvar_t *maxspectators;
cvar_t *maxentities;
cvar_t *g_select_empty;
cvar_t *dedicated;
cvar_t *g_footsteps;
cvar_t *g_monsterfootsteps;
cvar_t *g_fix_triggered;
cvar_t *g_commanderbody_nogod;

cvar_t *filterban;

cvar_t *sv_maxvelocity;
cvar_t *sv_gravity;

cvar_t *sv_rollspeed;
cvar_t *sv_rollangle;
cvar_t *gun_x;
cvar_t *gun_y;
cvar_t *gun_z;

cvar_t *run_pitch;
cvar_t *run_roll;
cvar_t *bob_up;
cvar_t *bob_pitch;
cvar_t *bob_roll;

cvar_t *sv_cheats;

cvar_t *flood_msgs;
cvar_t *flood_persecond;
cvar_t *flood_waitdelay;

cvar_t *sv_maplist;

cvar_t *gib_on;

cvar_t *aimfix;
cvar_t *g_machinegun_norecoil;
cvar_t *g_swap_speed;

e_events_t ESO_NOTHING = { 0, "Nothing", "", "", 0, 0};
e_events_t ESO_POSESSED = { 444, "Possessed","Comply will all \n audits for the \n duration\n", "Audits", 3, 0.5};
e_events_t ESO_BIRTHDAY = { 365, "Birthday", "Collect your presents\n from the bowels\n of the enemy\n", "Presents", 10, 2};
e_events_t ESO_AUDITDAY = { 373, "Auditing Day", "Get touched by\n as few ghosts as possible,\n more watcher spawns\n", "Times Hit", 3, 0.25 };
e_events_t ESO_BORING = { 111, "Boring", "No killing\n", "", 0, 0.40 };
e_events_t ESO_GHOSTBUSTERS = { 800, "Ghostbusters", "Repulse Watchers", "Hit", 3, 1 };

int repulseCooldown = 0;
qboolean repulseDebounce = false;
qboolean repulsing = false;
int eventReq = 0;
int auditTimer = 0;
qboolean spawnDebounce = false;
qboolean initDebounce = false;
int lastPulse = 0;
int lastPulse2 = 0;
qboolean eventIgnore = false;
e_audit_t AUD_CURR = {0, 0, -1};



e_audit_t AUD_NONE = {0, 0, -1};
e_audit_t AUD_TYPING = {1, 0, 0};
e_audit_t AUD_ACTION = {5, 0, 1};
e_audit_t AUD_QUIZ = {1, 0, 2};

int buffLevel = 0;
int debuffLevel = 0;
int timeSlowDuration = 0;
int timeSlowCooldown = 0;


e_events_t eEve[3];
e_manager_t eMan = {eEve, 0, -1, false, &AUD_CURR};

const char* quiz[5] = {"Orange Juice or Apple Juice?", "What game is this?", "What grade will this mod get?", "What are the flying ghosts called?", "Linux or Windows?"};
const char* qAnswer[5] = {"Cranberry", "Quake2", "A+" , "Watchers", "Whateveryouarepaidtouse"};
const char* spelling[7] = {"supercalifragilisticexpialidocious", "mississipi", "sequoia", "honorificabilitudinitatibus", "incomprehensibilities", "llIIIIllIlIlIlIllIlIlllII", "antidisestablishmentarianism"};

char* auditPhrase = "";
qboolean isAuditing;

void e_eventUpdate(void);
void e_eventSelect(void);
void G_RunFrame(void);

/* =================================================================== */

void
ShutdownGame(void)
{
	gi.dprintf("==== ShutdownGame ====\n");

	gi.FreeTags(TAG_LEVEL);
	gi.FreeTags(TAG_GAME);
}

/*
 * convert function declarations to correct one
 * (warning like from incompatible pointer type)
 * little bit better than cast function before set
 */
static void
ReadLevel_f(char *filename)
{
	ReadLevel(filename);
}

static void
WriteLevel_f(char *filename)
{
	WriteLevel(filename);
}

static void
ReadGame_f(char *filename)
{
	ReadGame(filename);
}

static void
WriteGame_f(char *filename, qboolean autosave)
{
	WriteGame(filename, autosave);
}

static void
SpawnEntities_f(char *mapname, char *entities, char *spawnpoint)
{
	SpawnEntities(mapname, entities, spawnpoint);
}

/*
 * Returns a pointer to the structure
 * with all entry points and global
 * variables
 */
Q2_DLL_EXPORTED game_export_t *
GetGameAPI(game_import_t *import)
{
	gi = *import;

	globals.apiversion = GAME_API_VERSION;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities_f;

	globals.WriteGame = WriteGame_f;
	globals.ReadGame = ReadGame_f;
	globals.WriteLevel = WriteLevel_f;
	globals.ReadLevel = ReadLevel_f;

	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;

	globals.RunFrame = G_RunFrame;

	globals.ServerCommand = ServerCommand;

	globals.edict_size = sizeof(edict_t);

	/* Initalize the PRNG */
	randk_seed();

	return &globals;
}

/*
 * this is only here so the functions
 * in shared source files can link
 */
void
Sys_Error(const char *error, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, error);
	vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	gi.error("%s", text);
}

void
Com_Printf(const char *msg, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	gi.dprintf("%s", text);
}

/* ====================================================================== */

void
ClientEndServerFrames(void)
{
	int i;
	edict_t *ent;

	/* calc the player views now that all
	   pushing  and damage has been added */
	for (i = 0; i < maxclients->value; i++)
	{
		ent = g_edicts + 1 + i;

		if (!ent->inuse || !ent->client)
		{
			continue;
		}

		ClientEndServerFrame(ent);
	}
}

/*
 * Returns the created target changelevel
 */
edict_t *
CreateTargetChangeLevel(char *map)
{
	edict_t *ent;

	if (!map)
	{
		return NULL;
	}

	ent = G_Spawn();
	ent->classname = "target_changelevel";
	Com_sprintf(level.nextmap, sizeof(level.nextmap), "%s", map);
	ent->map = level.nextmap;
	return ent;
}

/*
 * The timelimit or fraglimit has been exceeded
 */
void
EndDMLevel(void)
{
	edict_t *ent;
	char *s, *t, *f;
	static const char *seps = " ,\n\r";

	/* stay on same level flag */
	if ((int)dmflags->value & DF_SAME_LEVEL)
	{
		BeginIntermission(CreateTargetChangeLevel(level.mapname));
		return;
	}

	/* see if it's in the map list */
	if (*sv_maplist->string)
	{
		s = strdup(sv_maplist->string);
		f = NULL;
		t = strtok(s, seps);

		while (t != NULL)
		{
			if (Q_stricmp(t, level.mapname) == 0)
			{
				/* it's in the list, go to the next one */
				t = strtok(NULL, seps);

				if (t == NULL) /* end of list, go to first one */
				{
					if (f == NULL) /* there isn't a first one, same level */
					{
						BeginIntermission(CreateTargetChangeLevel(level.mapname));
					}
					else
					{
						BeginIntermission(CreateTargetChangeLevel(f));
					}
				}
				else
				{
					BeginIntermission(CreateTargetChangeLevel(t));
				}

				free(s);
				return;
			}

			if (!f)
			{
				f = t;
			}

			t = strtok(NULL, seps);
		}

		free(s);
	}

	if (level.nextmap[0]) /* go to a specific map */
	{
		BeginIntermission(CreateTargetChangeLevel(level.nextmap));
	}
	else    /* search for a changelevel */
	{
		ent = G_Find(NULL, FOFS(classname), "target_changelevel");

		if (!ent)
		{   /* the map designer didn't include a changelevel,
			   so create a fake ent that goes back to the same level */
			BeginIntermission(CreateTargetChangeLevel(level.mapname));
			return;
		}

		BeginIntermission(ent);
	}
}

void
CheckNeedPass(void)
{
	int need;

	/* if password or spectator_password has
	   changed, update needpass as needed */
	if (password->modified || spectator_password->modified)
	{
		password->modified = spectator_password->modified = false;

		need = 0;

		if (*password->string && Q_stricmp(password->string, "none"))
		{
			need |= 1;
		}

		if (*spectator_password->string &&
			Q_stricmp(spectator_password->string, "none"))
		{
			need |= 2;
		}

		gi.cvar_set("needpass", va("%d", need));
	}
}

void
CheckDMRules(void)
{
	int i;
	gclient_t *cl;

	if (level.intermissiontime)
	{
		return;
	}

	if (!deathmatch->value)
	{
		return;
	}

	if (timelimit->value)
	{
		if (level.time >= timelimit->value * 60)
		{
			gi.bprintf(PRINT_HIGH, "Timelimit hit.\n");
			EndDMLevel();
			return;
		}
	}

	if (fraglimit->value)
	{
		for (i = 0; i < maxclients->value; i++)
		{
			cl = game.clients + i;

			if (!g_edicts[i + 1].inuse)
			{
				continue;
			}

			if (cl->resp.score >= fraglimit->value)
			{
				gi.bprintf(PRINT_HIGH, "Fraglimit hit.\n");
				EndDMLevel();
				return;
			}
		}
	}
}

void
ExitLevel(void)
{
	int i;
	edict_t *ent;
	char command[256];

	Com_sprintf(command, sizeof(command), "gamemap \"%s\"\n", level.changemap);
	gi.AddCommandString(command);
	level.changemap = NULL;
	level.exitintermission = 0;
	level.intermissiontime = 0;
	ClientEndServerFrames();

	/* clear some things before going to next level */
	for (i = 0; i < maxclients->value; i++)
	{
		ent = g_edicts + 1 + i;

		if (!ent->inuse)
		{
			continue;
		}

		if (ent->health > ent->max_health)
		{
			ent->health = ent->max_health;
		}
	}

	debristhisframe = 0;
	gibsthisframe = 0;
}

/*
 * Advances the world by 0.1 seconds
 */

int e_eventTimer(){

	if(eMan.intermission){

		return (eMan.nextEvent - level.framenum)/10;

	} else {

		return (eMan.endFrame - level.framenum)/10;
	}

}

char* e_eventName(void){

	return eMan.eventArray[EVE_CURR].eventName;
}

int e_eventID(){

	return eMan.eventArray[EVE_CURR].eventID;
}

char* e_eventDesc(void){

	return eMan.eventArray[EVE_CURR].eventDesc;
}

char* e_objectiveName(void){

	return eMan.eventArray[EVE_CURR].objectiveType;
}

int e_objectiveNumber(void){

	return eventReq;
}

int e_objectiveTotal(void){

	return eMan.eventArray[EVE_CURR].objectiveTotal;
}

char* getAuditPhrase(){

	return auditPhrase;
}

void e_buff(){

	switch (buffLevel){

		case 0:
			gi.AddCommandString("safesay Arsenal Increased\n");
			gi.AddCommandString("give all\n");

			break;
		case 1:
			gi.AddCommandString("safesay You are blessed by David Goggin\n");
			gi.AddCommandString("cl_forwardspeed 250\n");
			break;
		case 2:
			gi.AddCommandString("safesay Anti Gravity Boots Installed\n");
			gi.AddCommandString("sv_gravity 300\n");
			break;
		case 3:
			gi.AddCommandString("safesay You can control time: (Press G)\n");
			gi.AddCommandString("bind g \"timeslow\" \n");
			break;
		case 4:
			gi.AddCommandString("safesay Events dont matter anymore. Complete the Level!\n");
			eventIgnore = true;

			break;

		default:

			break;
	}

	buffLevel++;

}

void e_debuff(){

	switch (debuffLevel){

		case 0:
			gi.AddCommandString("safesay Stop Panicking\n");
			gi.AddCommandString("timescale 1.5\n");

			break;
		case 1:
			gi.AddCommandString("safesay You are left handed\n");
			gi.AddCommandString("hand 1\n");
			break;
		case 2:
			gi.AddCommandString("safesay You broke a leg\n");
			gi.AddCommandString("sv_gravity 9999\n");
			gi.AddCommandString("cl_forwardspeed 100\n");
			break;
		case 3:
			gi.AddCommandString("safesay You might need glasses\n");
			gi.AddCommandString("fov 45\n");
			gi.AddCommandString("cl_entities 45\n");
			break;
		case 4:
			gi.AddCommandString("safesay You are a conscientious objector\n");
			gi.AddCommandString("unbind mouse1\n");

			break;

		default:

			break;
	}

	debuffLevel++;

}




void e_changeNextEvent(int n){

	switch(n){

		case 800:
			eMan.eventArray[EVE_NEXT] = ESO_GHOSTBUSTERS;
			break;
		case 373:
			eMan.eventArray[EVE_NEXT] = ESO_POSESSED;
			break;
		case 365:
			eMan.eventArray[EVE_NEXT] = ESO_BIRTHDAY;
			break;
		case 444:
			eMan.eventArray[EVE_NEXT] = ESO_AUDITDAY;
			break;
		case 111:
			eMan.eventArray[EVE_NEXT] = ESO_BORING;
			break;
		default:
			eMan.eventArray[EVE_NEXT] = ESO_NOTHING;
			break;
	}

}



void e_boringFail(){

	eventReq++;
	gi.AddCommandString("skip \n");

}

void winLoss(){

	// Boring, AuditDay
	if(e_eventID() == 111 || e_eventID() == 800){
		if(eMan.eventArray[EVE_CURR].objectiveTotal > eventReq){

		gi.AddCommandString("safesay Event Complete \n");
		e_buff();

	} else {
		gi.AddCommandString("safesay Event Failed \n");
		if(!eventIgnore){
			gi.AddCommandString("kill\n");


		}
	}

	return;
	}
	// Presents, Ghostbuster, Possessed
	if(eMan.eventArray[EVE_CURR].objectiveTotal <= eventReq) {

		gi.AddCommandString("safesay Event Complete \n");
		e_buff();
	} else {

		gi.AddCommandString("safesay Event Failed \n");
		if(!eventIgnore){
			gi.AddCommandString("kill\n");

		}
	}

}


void e_eventUpdate(){

	e_eventSelect();

	if (isAuditing == true) {

		if(auditTimer < level.framenum){

			gi.AddCommandString("say Audit FAILED! \n");
			e_debuff();
			isAuditing = false;
		}
		if(AUD_CURR.curr >= AUD_CURR.req){
			gi.AddCommandString("say Audit PASSED! \n");
			if (e_eventID() == 444){
				eventReq++;
			}
			isAuditing = false;
		}

	}

	if(timeSlowDuration < level.framenum && level.framenum - timeSlowDuration >= 10){

		gi.AddCommandString("timescale 1\n");
	}

	if (level.framenum%3==0){
		if (timeSlowCooldown < 100 && (timeSlowDuration < level.framenum) && lastPulse2 != level.framenum){
			lastPulse2 = level.framenum;
			timeSlowCooldown+=2;
		}
		if (repulseCooldown < 100 && !repulseDebounce) {
			repulseCooldown+=2;
			repulseDebounce = true;
		}
	} else {

		if(level.framenum < lastPulse && repulseDebounce){
			repulsing = false;
		}

		repulseDebounce = false;
	}

	if (eMan.intermission == false && level.framenum % 150 == 0 && !spawnDebounce){

		gi.AddCommandString("spawnwatcher\n");
		spawnDebounce = true;
		return;
	}
	if (level.framenum % 150 == 10){
	spawnDebounce = false;
	}

	return;
}

void e_presentDrop(edict_t *e, vec3_t loc){

	char s[70] = {0};
	char x[32] = {0};
	char y[32] = {0};
	char z[32] = {0};

	sprintf(x, "%f", loc[0]);
	sprintf(y, "%f", loc[1]);
	sprintf(z, "%f", loc[2]);

	strcat(s, "spawnpresent ");
	strcat(s, x);
	strcat(s, " ");
	strcat(s, y);
	strcat(s, " ");
	strcat(s, z);
	strcat(s, "\n");

	if (e_eventID() == 365){
		eventReq++;
	}

	gi.AddCommandString("safesay Ouch! \n");
	gi.AddCommandString(s);
}

int e_minigameCheck(){

	return eMan.currAudit->type;
}



qboolean e_wordCheck(char* s){

	char str[256] = {0};

	strcat(str, auditPhrase);


if (strcmp(s, str)==0){

	gi.AddCommandString("toggleconsole\n");

	AUD_CURR.curr++;

	return true;

} else {


	gi.AddCommandString("toggleconsole\n");


return false;
}

}


void e_jumpCheck(){
	if(AUD_CURR.type == 1){
		AUD_CURR.curr++;
	}
}

void e_flyerRepulsed(edict_t *e){

	if (!e) {
		return;
	}
	if (e_eventID() == 800){
		eventReq++;
	}

	gi.AddCommandString("safesay Watcher Killed \n");

	G_FreeEdict(e);

	lastPulse = level.framenum + 10;
}

void e_giveAudit(edict_t *e){

	char s[256] = {0};
	int randomNum;
	isAuditing = false;

	if(e_eventID() ==373 || e_eventID() == 444){
		eventReq++;
	}


	switch (AUD_CURR.type)	{

		case 0:
			AUD_CURR = AUD_ACTION;
			break;
		case 1:
			AUD_CURR = AUD_QUIZ;
			break;
		case 2:
			AUD_CURR = AUD_TYPING;
			break;
		default:
			AUD_CURR = AUD_TYPING;
			break;
	}

	switch (AUD_CURR.type){

		case 2:

			randomNum = (int)(random()*5);

			auditPhrase = quiz[randomNum];

			gi.AddCommandString("toggleconsole\n");

			strcat(s,"safesay ");
			strcat(s, auditPhrase);
			auditPhrase = qAnswer[randomNum];
			strcat(s, "\n");
			gi.AddCommandString(s);


			auditTimer = level.framenum + 2;

			isAuditing = true;



		break;

		case 1:

		gi.AddCommandString("safesay Jump 5 Times\n");

			auditTimer = level.framenum + 60;

			isAuditing = true;


		break;

		case 0:

			randomNum = (int) (random()*7);

			auditPhrase = spelling[randomNum];

			gi.AddCommandString("toggleconsole\n");

			strcat(s,"safesay ");
			strcat(s,"Spell: ");
			strcat(s,auditPhrase);
			strcat(s,"\n");
			gi.AddCommandString(s);

			auditTimer = level.framenum + 2 ;

			isAuditing = true;

		break;
	}

	G_FreeEdict(e);
}


void e_cycleEvent(int eventslot){

	switch(eMan.eventArray[eventslot-1].eventID){

		case 365:
			eMan.eventArray[eventslot] = ESO_GHOSTBUSTERS;
			break;
		case 444:
			eMan.eventArray[eventslot] = ESO_POSESSED;
			break;
		case 373:
			eMan.eventArray[eventslot] = ESO_BIRTHDAY;
			break;
		case 111:
			eMan.eventArray[eventslot] = ESO_AUDITDAY;
			break;
		case 800:
			eMan.eventArray[eventslot] = ESO_BORING;
			break;
		default:
			eMan.eventArray[eventslot] = ESO_BIRTHDAY;
			break;
	}
}
qboolean e_isAuditing(){
	return isAuditing;
}

void e_skipEvent(){

	if (eMan.intermission){
		eMan.nextEvent = level.framenum + 10;
	} else {

		eMan.endFrame = level.framenum + 10;
		eMan.nextEvent = level.framenum + 100;
	}
	return;
}

void e_eventSelect(){

	// intiial event
	if(eMan.nextEvent <= 0){
		e_cycleEvent(EVE_CURR);
		e_cycleEvent(EVE_NEXT);
		eMan.endFrame = level.framenum + 600*eMan.eventArray[EVE_CURR].weight + 10;
		eMan.nextEvent = eMan.endFrame + 100;
	} else if (level.framenum > eMan.endFrame && level.framenum < eMan.nextEvent)
	// event transition
	{
		if(!eMan.intermission){
			winLoss();
			eMan.intermission = true;
		}
	} else if (level.framenum >= eMan.nextEvent){
		eventReq = 0;
		eMan.intermission = false;
		eMan.eventArray[EVE_PREV] = eMan.eventArray[EVE_CURR];
		eMan.eventArray[EVE_CURR] = eMan.eventArray[EVE_NEXT];
		e_cycleEvent(EVE_NEXT);
		eMan.endFrame = level.framenum + 600*eMan.eventArray[EVE_CURR].weight + 10;
		eMan.nextEvent = eMan.endFrame + 100;
	}

}
int e_chargeAmount(){

	return repulseCooldown;
}

qboolean e_isRepulsing(){

	return repulsing;
}




void e_repulseCheck(){
	// get player
	int j;

	edict_t *ent;

	for (j = 1; j <= game.maxclients; j++)
	{
		ent = &g_edicts[j];
	}

	if(repulseCooldown >= 100){
		repulsing = true;
		repulseCooldown = 0;
		gi.sound(ent, CHAN_VOICE, gi.soundindex(
			"weapons/railgf1a.wav"), 1, ATTN_NORM, 0);
		return;
	} else {
		gi.sound(ent, CHAN_VOICE, gi.soundindex(
			"weapons/noammo.wav"), 1, ATTN_NORM, 0);
		return;
	}
}

int e_slowtimeCD(){

	return timeSlowCooldown;
}

void e_slowtime(){
	// get player
	int j;

	edict_t *ent;

	for (j = 1; j <= game.maxclients; j++)
	{
		ent = &g_edicts[j];
	}

	if(timeSlowCooldown >= 100){

		timeSlowCooldown = 0;
		timeSlowDuration = level.framenum + 5;
		gi.AddCommandString("timescale 0.65\n");
		gi.sound(ent, CHAN_VOICE, gi.soundindex(
			"weapons/plasshot.wav"), 1, ATTN_NORM, 0);
		return;
	} else if (timeSlowDuration > level.framenum){
		gi.AddCommandString("timescale 1\n");
		timeSlowDuration = level.framenum - 11;
		gi.sound(ent, CHAN_VOICE, gi.soundindex(
			"weapons/disint2.wav"), 1, ATTN_NORM, 0);
		return;
	} else {
		gi.sound(ent, CHAN_VOICE, gi.soundindex(
			"weapons/noammo.wav"), 1, ATTN_NORM, 0);
		return;
	}
}


void
G_RunFrame(void)
{
	int i;
	edict_t *ent;



	level.framenum++;
	level.time = level.framenum * FRAMETIME;

	if (!initDebounce){
	gi.AddCommandString("bind h \"repulse\" \n");
	gi.AddCommandString("god \n");
	gi.AddCommandString("bind mouse1 +attack \n");
	gi.AddCommandString("fov 90\n");
	gi.AddCommandString("cl_forwardspeed 200\n");
	gi.AddCommandString("hand 0\n");
	gi.AddCommandString("sv_gravity 800\n");
		gi.AddCommandString("unbind g \n");
		gi.AddCommandString("timescale 1\n");
		gi.AddCommandString("sv_cheats 1\n");
	initDebounce = true;
	}

	gibsthisframe = 0;
	debristhisframe = 0;

	/* choose a client for monsters to target this frame */
	AI_SetSightClient();

	/* exit intermissions */
	if (level.exitintermission)
	{
		ExitLevel();
		return;
	}

	/* treat each object in turn
	   even the world gets a chance
	   to think */
	ent = &g_edicts[0];

	for (i = 0; i < globals.num_edicts; i++, ent++)
	{
		if (!ent->inuse)
		{
			continue;
		}

		level.current_entity = ent;

		VectorCopy(ent->s.origin, ent->s.old_origin);

		e_eventUpdate();


		/* if the ground entity moved, make sure we are still on it */
		if ((ent->groundentity) &&
			(ent->groundentity->linkcount != ent->groundentity_linkcount))
		{
			ent->groundentity = NULL;

			if (!(ent->flags & (FL_SWIM | FL_FLY)) &&
				(ent->svflags & SVF_MONSTER))
			{
				M_CheckGround(ent);
			}
		}
		if ((i > 0) && (i <= maxclients->value))
		{
			ClientBeginServerFrame(ent);
			continue;
		}

		G_RunEntity(ent);
	}

	/* see if it is time to end a deathmatch */
	CheckDMRules();
	/* see if needpass needs updated */
	CheckNeedPass();

	/* build the playerstate_t structures for all players */
	ClientEndServerFrames();

}
