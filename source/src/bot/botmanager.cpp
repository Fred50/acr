//
// C++ Implementation: bot
//
// Description: Code for botmanager
//
// Main bot file
//
// Author:  Rick <rickhelmus@gmail.com>
//
//
//

#include "pch.h"
#include "bot.h"

extern void respawnself();

CBotManager BotManager;

// Bot manager class begin

CBotManager::~CBotManager(void){
	EndMap();
	//ClearStoredBots();
}

void CBotManager::Init()
{
	m_bBotsShoot = true;
	m_bIdleBots = false;
	m_iFrameTime = 0;
	m_iPrevTime = lastmillis;
	
	LoadBotNamesFile();
	//WaypointClass.Init();
	lsrand(time(NULL));
}

void CBotManager::Think()
{    
	if (m_bInit)
	{
		Init();
		m_bInit = false;
	}

	AddDebugText("m_sMaxAStarBots: %d", m_sMaxAStarBots);
	AddDebugText("m_sCurrentTriggerNr: %d", m_sCurrentTriggerNr);
	short x, y;
	WaypointClass.GetNodeIndexes(player1->o, &x, &y);
	AddDebugText("x: %d y: %d", x, y);
	
	m_iFrameTime = lastmillis - m_iPrevTime;
	if (m_iFrameTime > 250) m_iFrameTime = 250;
	m_iPrevTime = lastmillis;

    // Added by Victor: control multiplayer bots
    const int ourcn = getclientnum();
    if(ourcn >= 0 && m_ai(gamemode)){
	   // handle the bots
	   loopv(players){
		  if(!players[i] || players[i]->ownernum != ourcn) continue;
		  playerent *b = players[i];
		  // ensure there is a bot controller
		  if(!b->pBot){
			 b->pBot = new CACBot;
			 b->pBot->m_pMyEnt = b;

			 // create skills
			 b->pBot->m_pBotSkill = NULL;
			 b->pBot->MakeSkill(b->level);

			 // Sync waypoints
			 b->pBot->SyncWaypoints();
			 // Try spawn
			 b->pBot->Spawn();
		  }
		  b->pBot->Think();
	   }
    }
}

void CBotManager::LoadBotNamesFile()
{
    // Init bot names array first
    for (int i=0;i<200;i++)
        strcpy(m_szBotNames[i], "Bot");

    m_sBotNameCount = 0;

    // Load bot file
    char szNameFileName[256];
    MakeBotFileName("bot_names.txt", NULL, szNameFileName);
    FILE *fp = fopen(szNameFileName, "r");
    char szNameBuffer[256];
    int iIndex, iStrIndex;

    if (!fp)
    {
        conoutf("Warning: Couldn't load bot names file");
        return;
    }

    while (fgets(szNameBuffer, 80, fp) != NULL)
    {
        if (m_sBotNameCount >= 200)
        {
            conoutf("Warning: Max bot names reached(200), ignoring the rest of the"
                   "names");
            break;
        }

		// Skip entries starting with //
		if(szNameBuffer[0] == '/' && szNameBuffer[0] == szNameBuffer[1])
			continue;

        short length = (short)strlen(szNameBuffer);

        if (szNameBuffer[length-1] == '\n')
        {
            szNameBuffer[length-1] = 0;  // remove '\n'
            length--;
        }

        iStrIndex = 0;
        while (iStrIndex < length)
        {
            if ((szNameBuffer[iStrIndex] < ' ') || (szNameBuffer[iStrIndex] > '~') ||
                (szNameBuffer[iStrIndex] == '"'))
            {
                for (iIndex=iStrIndex; iIndex < length; iIndex++)
                    szNameBuffer[iIndex] = szNameBuffer[iIndex+1];
            }

            iStrIndex++;
        }

        if (szNameBuffer[0] != 0)
        {
            if (strlen(szNameBuffer) >= 16)
            {    conoutf("Warning: bot name \"%s\" has too many characters(16 is max)",
                       szNameBuffer);
            }
            copystring(m_szBotNames[m_sBotNameCount], szNameBuffer, 16);
            m_sBotNameCount++;
        }
    }
    fclose(fp);
}

void CBotManager::GetBotName(int seed, int sk, char *out)
{
    int skip = 0;
	if(m_zombie(gamemode))
	{
		// First block of ranked names are not for zombies
		loopi(m_sBotNameCount)
		{
			if(m_szBotNames[i][0] == '*') skip = i + 1;
			else break;
		}
	}
	if(skip >= m_sBotNameCount)
	{
		copystring(out, m_zombie(gamemode) ? "a zombie" : "a bot");
		return;
	}
	// Use a random name
	char *name = m_szBotNames[detrnd(seed, m_sBotNameCount - skip) + skip];
	const char *rank = ""; // prepend rank (only if based on skill)

	if(*name == '*') // rank based on skill
	{
		if(*++name == ' ') ++name; // skip space
		if(sk >= 90) rank = "Lt. "; // 10%
		else if(sk >= 70) rank = "Sgt. "; // 20%
		else if(sk >= 40) rank = "Cpl. "; // 30%
		else if(sk >= 20) rank = "Pfc. "; // 20%
		else rank = "Pvt. "; // 20%
	}

	defformatstring(fname)("%s%s", rank, name);
	filtername(out, fname);
}

void playerent::removeai()
{
	if(pBot){
		DELETEP(pBot->m_pBotSkill);
		DELETEP(pBot);
	}
	loopv(players)
		if(players[i] && this == players[i]->enemy)
			players[i]->enemy = NULL;
}

void CBotManager::EndMap()
{
	loopv(players)
		if(players[i])
			players[i]->removeai();
}

void CBotManager::BeginMap(const char *szMapName)
{ 
	EndMap(); // End previous map
	
	WaypointClass.Init();
	WaypointClass.SetMapName(szMapName);
	if (*szMapName && !WaypointClass.LoadWaypoints())
		WaypointClass.StartFlood();
	//WaypointClass.LoadWPExpFile(); // UNDONE

	CalculateMaxAStarCount();
	m_sUsingAStarBotsCount = 0;
	PickNextTrigger();
}

void CBotManager::LetBotsHear(int n, const vec *loc)
{
	if (!loc) return;
		
	loopv(players)
	{
		if (!players[i] || players[i]->ownernum != player1->clientnum || !players[i]->pBot || players[i]->state == CS_DEAD) continue;
		players[i]->pBot->HearSound(n, loc);
	}
}

void LetBotsHear(int n, const vec *loc) { BotManager.LetBotsHear(n, loc); }

// Notify all bots of a new waypoint
void CBotManager::AddWaypoint(node_s *pNode)
{
	if (players.length())
    {
        short x, y;
        waypoint_s *pWP;

        loopv(players)
        {
            if (!players[i] || !isowned(players[i]) || !players[i]->pBot) continue;

            pWP = new waypoint_s;
            pWP->pNode = pNode;
            WaypointClass.GetNodeIndexes(pNode->v_origin, &x, &y);
            players[i]->pBot->m_WaypointList[x][y].AddNode(pWP);

#ifndef RELEASE_BUILD
            if (!players[i]->pBot->GetWPFromNode(pNode)) condebug("Error adding bot wp!");
#endif
        }
    }
	CalculateMaxAStarCount();
}

// Notify all bots of a deleted waypoint
void CBotManager::DelWaypoint(node_s *pNode)
{
	if (players.length())
    {
        short x, y;
        TLinkedList<waypoint_s *>::node_s *p;

        loopv(players)
        {
            if (!players[i] || !isowned(players[i]) || !players[i]->pBot) continue;

            WaypointClass.GetNodeIndexes(pNode->v_origin, &x, &y);
            p = players[i]->pBot->m_WaypointList[x][y].GetFirst();

            while(p)
            {
                if (p->Entry->pNode == pNode)
                {
                    delete p->Entry;
                    players[i]->pBot->m_WaypointList[x][y].DeleteNode(p);
                    break;
                }
                p = p->next;
            }
        }
    }

	CalculateMaxAStarCount();
}

void CBotManager::MakeBotFileName(const char *szFileName, const char *szDir1, char *szOutput)
{
	const char *DirSeperator;

#ifdef WIN32
	DirSeperator = "\\";
	strcpy(szOutput, "bot\\");
#else
	DirSeperator = "/";
	strcpy(szOutput, "bot/");
#endif
	
	if (szDir1){
		strcat(szOutput, szDir1);
		strcat(szOutput, DirSeperator);
	}
	
	strcat(szOutput, szFileName);
}

void CBotManager::CalculateMaxAStarCount()
{  
	if (WaypointClass.m_iWaypointCount > 0) // Are there any waypoints?
	{
		m_sMaxAStarBots = 8 - short(ceil((float)WaypointClass.m_iWaypointCount /
							   1000.0f));
		if (m_sMaxAStarBots < 1)
			m_sMaxAStarBots = 1;
	}
	else
		m_sMaxAStarBots = 1;
}

void CBotManager::PickNextTrigger()
{
	short lowest = -1;
	bool found0 = false; // True if found a trigger with nr 0
	
	loopv(ents)
	{
		entity &e = ents[i];
		
#if defined AC_CUBE		
/*		if ((e.type != TRIGGER) || !e.spawned)
			continue;*/
#elif defined VANILLA_CUBE
		if ((e.type != CARROT) || !e.spawned)
			continue;
#endif		
		if (OUTBORD(e.x, e.y)) continue;
		
		vec o(e.x, e.y, S(e.x, e.y)->floor+player1->eyeheight);

		node_s *pWptNearEnt = NULL;
		
		pWptNearEnt = WaypointClass.GetNearestTriggerWaypoint(o, 2.0f);
		
		if (pWptNearEnt)
		{
			if ((pWptNearEnt->sTriggerNr > 0) &&
			    ((pWptNearEnt->sTriggerNr < lowest) || (lowest == -1)))
				lowest = pWptNearEnt->sTriggerNr;
			if (pWptNearEnt->sTriggerNr == 0) found0 = true;
		}
		
#ifdef WP_FLOOD
		pWptNearEnt = WaypointClass.GetNearestTriggerFloodWP(o, 2.0f);
		
		if (pWptNearEnt)
		{
			if ((pWptNearEnt->sTriggerNr > 0) && 
			    ((pWptNearEnt->sTriggerNr < lowest) || (lowest == -1)))
				lowest = pWptNearEnt->sTriggerNr;
			if (pWptNearEnt->sTriggerNr == 0) found0 = true;
		}
				
#endif			
	}
	
	if ((lowest == -1) && found0) lowest = 0;
	
	if (lowest != -1)
		m_sCurrentTriggerNr = lowest;
}

// Bot manager class end

void togglegrap()
{
	if (SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GrabMode(0)) SDL_WM_GrabInput(SDL_GRAB_ON);
	else SDL_WM_GrabInput(SDL_GrabMode(0));
}

COMMAND(togglegrap, ARG_NONE);

#ifndef RELEASE_BUILD

#ifdef VANILLA_CUBE
void drawbeamtocarrots()
{
	loopv(ents)
	{
		entity &e = ents[i];
		vec o = { e.x, e.y, S(e.x, e.y)->floor+player1->eyeheight };
		if ((e.type != CARROT) || !e.spawned) continue;
		particle_trail(1, 500, player1->o, o);
	}
}

COMMAND(drawbeamtocarrots, ARG_NONE);

void drawbeamtoteleporters()
{
	loopv(ents)
	{
		entity &e = ents[i];
		vec o = { e.x, e.y, S(e.x, e.y)->floor+player1->eyeheight };
		if (e.type != TELEPORT) continue;
		particle_trail(1, 500, player1->o, o);
	}
}

COMMAND(drawbeamtoteleporters, ARG_NONE);
#endif

void testvisible(int iDir)
{
    
	vec angles, end, forward, right, up;
	traceresult_s tr;
	int Dir;
	
	switch(iDir)
	{
		case 0: default: Dir = FORWARD; break;
		case 1: Dir = BACKWARD; break;
		case 2: Dir = LEFT; break;
		case 3: Dir = RIGHT; break;
		case 4: Dir = UP; break;
		case 5: Dir = DOWN; break;
	}
	
	vec from = player1->o;
	from.z -= (player1->eyeheight - 1.25f);
	end = from;
	makevec(&angles, player1->pitch, player1->yaw, player1->roll);
	angles.x=0;
	
	if (Dir & UP)
		angles.x = WrapXAngle(angles.x + 45.0f);
	else if (Dir & DOWN)
		angles.x = WrapXAngle(angles.x - 45.0f);
		
	if ((Dir & FORWARD) || (Dir & BACKWARD))
	{
		if (Dir & BACKWARD)
			angles.y = WrapYZAngle(angles.y + 180.0f);
		
		if (Dir & LEFT)
		{
			if (Dir & FORWARD)
				angles.y = WrapYZAngle(angles.y - 45.0f);
			else
				angles.y = WrapYZAngle(angles.y + 45.0f);
		}
		else if (Dir & RIGHT)
		{
			if (Dir & FORWARD)
				angles.y = WrapYZAngle(angles.y + 45.0f);
			else
				angles.y = WrapYZAngle(angles.y - 45.0f);
		}
	}
	else if (Dir & LEFT)
		angles.y = WrapYZAngle(angles.y - 90.0f);
	else if (Dir & RIGHT)
		angles.y = WrapYZAngle(angles.y + 90.0f);
	else if (Dir & UP)
		angles.x = WrapXAngle(angles.x + 90.0f);
	else if (Dir & DOWN)
		angles.x = WrapXAngle(angles.x - 90.0f);
		
	AnglesToVectors(angles, forward, right, up);
	
	forward.mul(20.0f);
	end.add(forward);
	    
	TraceLine(from, end, player1, false, &tr);
	
	//debugbeam(from, tr.end);
	char sz[250];
	sprintf(sz, "dist: %f; hit: %d", GetDistance(from, tr.end), tr.collided);
	condebug(sz);
}

COMMAND(testvisible, ARG_1INT);

void mapsize(void)
{
	conoutf("ssize: %d", ssize);
}

COMMAND(mapsize, ARG_NONE);
			
#endif
