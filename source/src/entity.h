enum							// static entity types
{
	NOTUSED = 0,				// entity slot not in use in map
	LIGHT,					  // lightsource, attr1 = radius, attr2 = intensity
	PLAYERSTART,				// attr1 = angle, attr2 = team
	I_CLIPS, I_AMMO, I_GRENADE,
	I_HEALTH, I_HELMET, I_ARMOR, I_AKIMBO,
	MAPMODEL,				   // attr1 = angle, attr2 = idx
	CARROT,					 // attr1 = tag, attr2 = type
	LADDER,
	CTF_FLAG,				   // attr1 = angle, attr2 = red/blue
	SOUND,
	CLIP,
	PLCLIP,
	MAXENTTYPES
};

#define isitem(i) ((i) >= I_CLIPS && (i) <= I_AKIMBO)

struct persistent_entity		// map entity
{
	short x, y, z;			  // cube aligned position
	short attr1;
	uchar type;				 // type is one of the above
	uchar attr2, attr3, attr4;
	persistent_entity(short x, short y, short z, uchar type, short attr1, uchar attr2, uchar attr3, uchar attr4) : x(x), y(y), z(z), attr1(attr1), type(type), attr2(attr2), attr3(attr3), attr4(attr4) {}
	persistent_entity() {}
};

struct entity : public persistent_entity
{
	// dynamic states
	bool spawned;
	int spawntime;

	entity(short x, short y, short z, uchar type, short attr1, uchar attr2, uchar attr3, uchar attr4) : persistent_entity(x, y, z, type, attr1, attr2, attr3, attr4), spawned(false), spawntime(0) {}
	entity() {}
	bool fitsmode(int gamemode, int mutators) { return !m_noitems(gamemode, mutators) && isitem(type) && !(m_noitemsammo(gamemode, mutators) && type!=I_AMMO) && !(m_noitemsnade(gamemode, mutators) && type!=I_GRENADE); }
	void transformtype(int gamemode, int mutators)
	{
		if(m_noitemsammo(gamemode, mutators) && (type == I_CLIPS || type == I_GRENADE)) type = I_AMMO;
		else if(m_pistol(gamemode, mutators) && (type == I_AMMO || type == I_GRENADE)) type = I_CLIPS;
		else if(m_noitemsnade(gamemode, mutators)) switch(type){
			case I_CLIPS:
			case I_AMMO:
			case I_ARMOR:
			case I_AKIMBO:
				type = I_GRENADE;
				break;
		}
	}
};

enum { PR_CLEAR = 0, PR_ASSIST, PR_SPLAT, PR_HS, PR_KC, PR_KD, PR_HEALSELF, PR_HEALTEAM, PR_HEALENEMY, PR_HEALWOUND, PR_HEALEDBYTEAMMATE, PR_ARENA_WIN, PR_ARENA_WIND, PR_ARENA_LOSE, PR_SECURE_SECURED, PR_SECURE_SECURE, PR_SECURE_OVERTHROW, PR_BUZZKILL, PR_BUZZKILLED, PR_KD_SELF, PR_KD_ENEMY, PR_MAX };

#define HEALTHPRECISION 1
#define HEALTHSCALE 10 // 10 ^ HEALTHPRECISION
#define STARTHEALTH ((m_juggernaut(gamemode, mutators) ? 1000 : 100) * HEALTHSCALE)
#define MAXHEALTH ((m_juggernaut(gamemode, mutators) ? 1100 : 120) * HEALTHSCALE)
#define VAMPIREMAX (STARTHEALTH + 200 * HEALTHSCALE)
#define MAXZOMBIEROUND 30
#define ZOMBIEHEALTHFACTOR 5
#define MAXTHIRDPERSON 25

#define SPAWNDELAY (m_secure(gamemode) ? 2000 : m_affinity(gamemode) ? 5000 : 1500)

#define REGENDELAY 4250
#define REGENINT 2500

#define STARTARMOR 0
#define MAXARMOR 100

struct itemstat { short add, start, max, sound; };
extern const itemstat ammostats[WEAP_MAX];
extern const itemstat powerupstats[];

#define SWITCHTIME(perk) ((perk) ? 200 : 400)
#define ADSTIME(perk) ((perk) ? 200 : 300)
#define CROUCHTIME 500

#define SGRAYS 24
#define SGGIB 180 // 18-26 rays (only have 24)
#define NADEPOWER 2
#define NADETTL 4350
#define MARTYRDOMTTL 2500
#define KNIFEPOWER 4.5f
#define KNIFETTL 30000
#define GIBBLOODMUL 1.5f
#define SPAWNPROTECT (m_affinity(gamemode) ? 1000 : m_team(gamemode, mutators) ? 1500 : 1250)
#define COMBOTIME 1000

#define MAXLEVEL 100
#define MAXEXP 1000

struct mul{ float head, torso, leg; };
enum { MUL_NORMAL = 0, MUL_SNIPER, MUL_SHOTGUN, MUL_NUM };
extern const mul muls[MUL_NUM];

#define BLEEDDMG 10
#define BLEEDDMGZ 5
#define BLEEDDMGPLUS 15

struct guninfo { string modelname; short sound, reload, reloadtime, attackdelay, damage, range, endrange, rangeminus, projspeed, part, spread, spreadrem, kick, addsize, magsize, mdl_kick_rot, mdl_kick_back, recoil, maxrecoil, recoilangle, pushfactor; bool isauto; };
extern const guninfo guns[WEAP_MAX];

static inline ushort reloadtime(int gun) { return guns[gun].reloadtime; }
static inline ushort attackdelay(int gun) { return guns[gun].attackdelay; }
static inline ushort magsize(int gun) { return guns[gun].magsize; }
static inline ushort reloadsize(int gun) { return guns[gun].addsize; }
static inline ushort effectiveDamage(int gun, float dist, bool explosive, bool useReciprocal) {
	// useReciprocal is probably going to be unused, but when it is false, the damage should be AC-like (not possible because the values are gone?)
	float finaldamage = 0;
	if(dist <= guns[gun].range || (!guns[gun].range && !guns[gun].endrange)) finaldamage = guns[gun].damage;
	else if(dist >= guns[gun].endrange) finaldamage = guns[gun].damage - guns[gun].rangeminus;
	else{
		float subtractfactor = (dist - guns[gun].range) / (guns[gun].endrange - guns[gun].range);
		if(explosive){
			if(useReciprocal)
				finaldamage = guns[gun].damage/(1+(guns[gun].damage-1)*pow(subtractfactor,4));
			else{
				//if(dist >= rangeminus) return 0;
				subtractfactor = (dist - guns[gun].range) / (guns[gun].rangeminus - guns[gun].range);
				finaldamage = guns[gun].damage * (1 - subtractfactor * subtractfactor);
			}
		}
		else
			finaldamage = guns[gun].damage - subtractfactor * guns[gun].rangeminus;
	}
	return finaldamage * HEALTHSCALE;
}

extern ushort reloadtime(int gun);
extern ushort attackdelay(int gun);
extern ushort magsize(int gun);
extern ushort reloadsize(int gun);
extern ushort effectiveDamage(int gun, float dist, bool explosive = false, bool useReciprocal = true);

extern const int obit_suicide(int weap);
extern const char *suicname(int obit);
extern const bool isheadshot(int weapon, int style);
extern const int toobit(int weap, int style);
extern const char *killname(int obit, bool headshot);

enum { PERK_NONE = 0, PERK_RADAR, PERK_NINJA, PERK_POWER, PERK_TIME, PERK_MAX };
enum { PERK1_NONE = 0, PERK1_AGILE = PERK_MAX, PERK1_HAND, PERK1_LIGHT, PERK1_SCORE, PERK1_MAX, };
enum { PERK2_NONE = 0, PERK2_VISION = PERK_MAX, PERK2_STREAK, PERK2_STEADY, PERK2_HEALTH, PERK2_MAX };

extern float gunspeed(int gun, int ads, bool lightweight = false);

#define isteam(a,b)   (m_team(gamemode, mutators) && (a)->team == (b)->team)

enum { TEAM_RED = 0, TEAM_BLUE, TEAM_SPECT, TEAM_NUM };
#define team_valid(t) ((t) >= 0 && (t) < TEAM_NUM)
#define team_string(t) ((t) == TEAM_BLUE ? "BLUE" : (t) == TEAM_RED ? "RED" : "SPECT")
#define team_int(t) (!strcmp((t), "RED") ? TEAM_RED : !strcmp((t), "BLUE") ? TEAM_BLUE : TEAM_SPECT)
#define team_opposite(o) ((o) < TEAM_SPECT ? (o) ^ 1 : TEAM_SPECT)
#define team_color(t) ((t) == TEAM_SPECT ? 4 : (t) ? 1 : 3)
#define team_rel_color(a, b) (a == b ? 1 : a && b && b->team != TEAM_SPECT ? isteam(a, b) ? 0 : 3 : 4)

struct teamscore
{
	int team, points, flagscore, frags, assists, deaths;
	teamscore(int team) : team(team), points(0), flagscore(0), frags(0), assists(0), deaths(0) { }
};

enum { ENT_PLAYER = 0, ENT_CAMERA, ENT_BOUNCE };
enum { CS_ALIVE = 0, CS_DEAD, CS_WAITING, CS_EDITING };
enum { PRIV_NONE = 0, PRIV_MASTER, PRIV_ADMIN, PRIV_MAX };

static inline const uchar privcolor(int priv, bool dead = false){
	switch(priv){
		case PRIV_NONE: return dead ? 4 : 5;
		case PRIV_MASTER: return dead ? 8 : 0;
		case PRIV_ADMIN: return dead ? 7 : 3;
		case PRIV_MAX: return dead ? 9 : 1;
	}
	return 5;
}

static inline const char *privname(int priv){
	switch(priv){
		case PRIV_NONE: return "user";
		case PRIV_MASTER: return "master";
		case PRIV_ADMIN: return "admin";
		case PRIV_MAX: return "highest";
	}
	return "unknown";
}
enum { SM_NONE = 0, SM_DEATHCAM, SM_FOLLOWSAME, SM_FOLLOWALT, SM_FLY, SM_NUM };

struct physent
{
	vec o, vel;						 // origin, velocity
	vec deltapos, newpos;					   // movement interpolation
	float yaw, pitch, roll;			 // used as vec in one place
	float pitchvel, yawvel, pitchreturn, yawreturn;
	float maxspeed;					 // cubes per second, 24 for player
	int timeinair;					  // used for fake gravity
	float radius, eyeheight, maxeyeheight, aboveeye;  // bounding box size
	bool inwater;
	bool onfloor, onladder, jumpnext, crouching, trycrouch, cancollide, stuck;
	int lastsplash;
	char move, strafe;
	uchar state, type;
	float eyeheightvel;

	physent() : o(0, 0, 0), deltapos(0, 0, 0), newpos(0, 0, 0), yaw(270), pitch(0), pitchreturn(0), roll(0), pitchvel(0),
				crouching(false), trycrouch(false), cancollide(true), stuck(false), lastsplash(0), state(CS_ALIVE)
	{
		reset();
	}
	virtual ~physent() {}

	void resetinterp()
	{
		newpos = o;
		newpos.z -= eyeheight;
		deltapos = vec(0, 0, 0);
	}

	void reset()
	{
		vel.x = vel.y = vel.z = eyeheightvel = 0.0f;
		move = strafe = 0;
		timeinair = lastsplash = 0;
		onfloor = onladder = inwater = jumpnext = crouching = trycrouch = stuck = false;
	}

	virtual bool trystick(playerent *pl) { return false; }
	virtual void oncollision() {}
	virtual void onmoved(const vec &dist) {}
};

struct dynent : physent				 // animated ent
{
	bool k_left, k_right, k_up, k_down;		 // see input code

	animstate prev[2], current[2];			  // md2's need only [0], md3's need both for the lower&upper model
	int lastanimswitchtime[2];
	void *lastmodel[2];
	int lastrendered;

	void stopmoving()
	{
		k_left = k_right = k_up = k_down = jumpnext = false;
		move = strafe = 0;
	}

	void resetanim()
	{
		loopi(2)
		{
			prev[i].reset();
			current[i].reset();
			lastanimswitchtime[i] = -1;
			lastmodel[i] = NULL;
		}
		lastrendered = 0;
	}

	void reset()
	{
		physent::reset();
		stopmoving();
	}

	dynent() { reset(); resetanim(); }
	virtual ~dynent() {}
};

#define MAXNAMELEN 16
#define MAXTEAMLEN 4

struct bounceent;

#define POSHIST_SIZE 7

struct poshist
{
	int nextupdate, curpos, numpos;
	vec pos[POSHIST_SIZE];

	poshist() : nextupdate(0) { reset(); }

	const int size() const { return numpos; }

	void reset()
	{
		curpos = 0;
		numpos = 0;
	}

	void addpos(const vec &o)
	{
		pos[curpos] = o;
		curpos++;
		if(curpos>=POSHIST_SIZE) curpos = 0;
		if(numpos<POSHIST_SIZE) numpos++;
	}

	const vec &getpos(int i) const
	{
		i = curpos-1-i;
		if(i < 0) i += POSHIST_SIZE;
		return pos[i];
	}

	void update(const vec &o, int lastmillis)
	{
		if(lastmillis<nextupdate) return;
		if(o.dist(pos[0]) >= 4.0f) addpos(o);
		nextupdate = lastmillis + 100;
	}
};

struct playerstate
{
	int ownernum; // for bots
	int health, armor, spawnmillis, lastkiller;
	int pointstreak, deathstreak, assists, radarearned, airstrikes, nukemillis;
	int primary, secondary, perk1, perk2, nextprimary, nextsecondary, nextperk1, nextperk2;
	int gunselect, level;
	bool akimbo, scoping, sprinting, typing;
	int ammo[WEAP_MAX], mag[WEAP_MAX], gunwait[WEAP_MAX];
	ivector damagelog;

	playerstate() : primary(WEAP_ASSAULT), secondary(WEAP_PISTOL), perk1(PERK1_NONE), perk2(PERK2_NONE),
		nextprimary(WEAP_ASSAULT), nextsecondary(WEAP_PISTOL), nextperk1(PERK1_NONE), nextperk2(PERK2_NONE),
		ownernum(-1), level(1), pointstreak(0), deathstreak(0), airstrikes(0), radarearned(0), nukemillis(0), spawnmillis(0), lastkiller(-1) {}
	virtual ~playerstate() {}

	const itemstat &itemstats(int type)
	{
		switch(type)
		{
			case I_CLIPS: return ammostats[secondary];
			case I_AMMO: return ammostats[primary];
			case I_GRENADE: return ammostats[WEAP_GRENADE];
			case I_AKIMBO: return ammostats[WEAP_AKIMBO];
			case I_HEALTH: case I_HELMET: case I_ARMOR:
				return powerupstats[type - I_HEALTH];
			default:
				return *(itemstat *)0;
		}
	}

	bool canpickup(int type)
	{
		switch(type)
		{
			case I_CLIPS: return ammo[akimbo ? WEAP_AKIMBO : secondary]<ammostats[akimbo ? WEAP_AKIMBO : secondary].max;
			case I_AMMO: return primary == WEAP_SWORD || ammo[primary]<ammostats[primary].max;
			case I_GRENADE: return mag[WEAP_GRENADE]<ammostats[WEAP_GRENADE].max;
			case I_HEALTH: return health<powerupstats[type-I_HEALTH].max;
			case I_HELMET:
			case I_ARMOR:
				return armor<powerupstats[type-I_HEALTH].max;
			case I_AKIMBO: return !akimbo && ownernum < 0;
			default: return false;
		}
	}

	void additem(const itemstat &is, int &v)
	{
		v += is.add;
		if(v > is.max) v = is.max;
	}

	void pickup(int type)
	{
		switch(type)
		{
			case I_CLIPS:
				additem(ammostats[secondary], ammo[secondary]);
				additem(ammostats[WEAP_AKIMBO], ammo[WEAP_AKIMBO]);
				break;
			case I_AMMO: additem(ammostats[primary], ammo[primary]); break;
			case I_GRENADE: additem(ammostats[WEAP_GRENADE], mag[WEAP_GRENADE]); break;
			case I_HEALTH: additem(powerupstats[type-I_HEALTH], health); break;
			case I_HELMET:
			case I_ARMOR: additem(powerupstats[type-I_HEALTH], armor); break;
			case I_AKIMBO:
				akimbo = true;
				mag[WEAP_AKIMBO] = guns[WEAP_AKIMBO].magsize;
				additem(ammostats[WEAP_AKIMBO], ammo[WEAP_AKIMBO]);
				break;
		}
	}

	void respawn(int gamemode, int mutators)
	{
		health = STARTHEALTH;
		armor = STARTARMOR;
		spawnmillis = 0;
		assists = armor = 0;
		gunselect = WEAP_PISTOL;
		akimbo = scoping = sprinting = typing = false;
		loopi(WEAP_MAX) ammo[i] = mag[i] = gunwait[i] = 0;
		mag[WEAP_KNIFE] = 1;
		lastkiller = -1;
	}

	virtual void spawnstate(int team, int gamemode, int mutators)
	{
		if(m_zombie(gamemode) && team == TEAM_RED) primary = WEAP_SWORD;
		else if(m_pistol(gamemode, mutators)) primary = WEAP_PISTOL;
		else if(m_gib(gamemode, mutators)) primary = WEAP_KNIFE;
		else if(m_demolition(gamemode, mutators)) primary = WEAP_RPG; // inversion
		else switch(nextprimary){
			default: primary = m_sniper(gamemode, mutators) ? WEAP_BOLT : WEAP_ASSAULT; break;
			case WEAP_KNIFE:
			case WEAP_SHOTGUN:
			case WEAP_SUBGUN:
			case WEAP_ASSAULT:
			case WEAP_GRENADE:
			case WEAP_AKIMBO:
			case WEAP_ASSAULT2:
				if(m_sniper(gamemode, mutators)){
					primary = WEAP_BOLT;
					break;
				}
			// Only bolt/M82/M21 for insta mutator
			case WEAP_SNIPER:
				if(m_insta(gamemode, mutators)){
					primary = WEAP_BOLT;
					break;
				}
			// Only bolt/M82 for sniping mutator
			case WEAP_BOLT:
			case WEAP_SNIPER2:
				primary = nextprimary;
				break;
		}

		if(m_zombie(gamemode) && team == TEAM_RED) secondary = WEAP_KNIFE;
		else if(m_pistol(gamemode, mutators)) secondary = primary; // no secondary
		else if(m_sniper(gamemode, mutators) || m_demolition(gamemode, mutators) || m_gib(gamemode, mutators)) secondary = WEAP_SWORD;
		else switch(nextsecondary){
			default: secondary = WEAP_PISTOL; break;
			case WEAP_PISTOL:
			case WEAP_HEAL:
			case WEAP_SWORD:
			case WEAP_RPG:
				secondary = nextsecondary;
				break;
		}

		// always have a primary now
		if(true){
			ammo[primary] = (ammostats[primary].start-1);
			mag[primary] = magsize(primary);
		}
		if(primary != secondary){
			ammo[secondary] = (ammostats[secondary].start-1);
			mag[secondary] = magsize(secondary);
		}

		// extras
		ammo[WEAP_KNIFE] = ammostats[WEAP_KNIFE].start;
		if(!m_noitems(gamemode, mutators) && !m_noitemsammo(gamemode, mutators) && (team != TEAM_RED || !m_zombie(gamemode)))
			mag[WEAP_GRENADE] = ammostats[WEAP_GRENADE].start;

		gunselect = primary;

		if(m_zombie(gamemode) && team == TEAM_RED)
		{
			perk1 = PERK1_AGILE;
			perk2 = PERK2_STREAK;
		}
		else
		{
			perk1 = nextperk1;
			perk2 = nextperk2;
		}

		// no classic override

		if(perk1 <= PERK1_NONE || perk1 >= PERK1_MAX) perk1 = rnd(PERK1_MAX-1)+1;
		if(perk2 <= PERK1_NONE || perk2 >= PERK2_MAX) perk2 = rnd(PERK2_MAX-PERK_MAX-1)+PERK_MAX+1;

		// special perks need both slots
		if(perk1 < PERK_MAX) perk2 = perk1;

		const int healthsets[3] = { STARTHEALTH - 15 * HEALTHSCALE, STARTHEALTH, STARTHEALTH + 20 * HEALTHSCALE };
		health = healthsets[(!m_regen(gamemode, mutators) && m_sniper(gamemode, mutators) ? 0 : 1) + (perk2 == PERK2_HEALTH ? 1 : 0)];
		if(m_zombie(gamemode)){
			switch(team){
				case TEAM_RED:
					if(m_onslaught(gamemode, mutators)){
						health = STARTHEALTH * ZOMBIEHEALTHFACTOR;
						armor += 50;
					}
					else health = STARTHEALTH + rnd(STARTHEALTH * ZOMBIEHEALTHFACTOR);
					break;
				case TEAM_BLUE:
					if(!m_onslaught(gamemode, mutators)) break;
					// humans for onslaught only
					if(perk2 == PERK2_HEALTH) health = STARTHEALTH * ZOMBIEHEALTHFACTOR; // all 500
					else health = STARTHEALTH * (rnd(ZOMBIEHEALTHFACTOR - 2) + 2) + (STARTHEALTH/2); // 250 - 450
					armor += 2000;
					break;
			}
		}
		if(m_vampire(gamemode, mutators)) health /= 2;
	}

	// just subtract damage here, can set death, etc. later in code calling this
	int dodamage(int damage, bool penetration){
		int ad = penetration ? 0 : damage*3/10; // let armor absorb when possible
		if(ad>armor) ad = armor;
		damage -= ad;
		// apply it!
		armor -= ad;
		health -= damage;
		return damage;
	}

	int protect(int millis, int gamemode, int mutators){
		const int delay = SPAWNPROTECT, spawndelay = millis - spawnmillis;
		int amt = 0;
        if(spawnmillis && delay && spawndelay < delay) amt = delay - spawndelay;
        return amt;
	}
};

#define HEADSIZE 0.4f
#define TORSOPART 0.35f
#define LEGPART (1 - TORSOPART)

#define PLAYERRADIUS 1.1f
#define PLAYERHEIGHT 4.5f
#define PLAYERABOVEEYE .7f
#define WEAPONBELOWEYE .2f

struct eventicon{
    enum { CHAT = 0, VOICECOM, HEADSHOT, DECAPITATED, FIRSTBLOOD, CRITICAL, REVENGE, BLEED, PICKUP, RADAR, AIRSTRIKE, NUKE, JUGGERNAUT, DROPNADE, SUICIDEBOMB, TOTAL };
    int type, millis;
	eventicon(int type, int millis) : type(type), millis(millis){}
};

struct damageinfo{
	vec o;
	int millis, damage;
	damageinfo(vec s, int t, int d) : o(s), millis(t), damage(d) {} // lol read the constructor's parameters
};

class CBot;

struct playerent : dynent, playerstate
{
	int clientnum, lastrecieve, plag, ping, rank;
	int lifesequence;				   // sequence id for each respawn, used in damage test
	int radarmillis; float lastloudpos[3];
	int points, frags, flagscore, deaths;
	int lastaction, lastmove, lastpain, lasthitmarker;
	int priv, vote, voternum, lastregen;
	int ads, wantsswitch; bool wantsreload, delayedscope;
	bool attacking;
	string name;
	int weaponchanging;
	int nextweapon; // weapon we switch to
	int team, skin, thirdperson, build;
	int spectatemode, followplayercn;
	int eardamagemillis, flashmillis;
	int respawnoffset;
	vector<eventicon> icons;

	weapon *weapons[WEAP_MAX];
	weapon *prevweaponsel, *weaponsel, *nextweaponsel, *lastattackweapon;

	poshist history; // Previous stored locations of this player

	const char *skin_noteam, *skin_red, *skin_blue;

	float deltayaw, deltapitch, newyaw, newpitch;
	int smoothmillis;

	vector<damageinfo> damagestack;
	vec head, eject, muzzle;
	vec deathcamsrc;

	// AI
	CBot *pBot;

	playerent *enemy;  // monster wants to kill this entity
	float targetpitch, targetyaw; // monster wants to look in this direction

	playerent() : spectatemode(SM_NONE), vote(VOTE_NEUTRAL), voternum(MAXCLIENTS), priv(PRIV_NONE), head(-1, -1, -1), eject(-1, -1, -1), muzzle(-1, -1, -1), deathcamsrc(0, 0, 0)
	{
		// ai
		enemy = NULL;
		pBot = NULL;
		targetpitch = targetyaw = 0;

		lastrecieve = plag = ping = rank = lifesequence = points = frags = flagscore = deaths = lastpain = lastregen = lasthitmarker = skin = thirdperson = build = eardamagemillis = respawnoffset = radarmillis = ads = 0;
		radarearned = airstrikes = nukemillis = 0;
		weaponsel = nextweaponsel = lastattackweapon = prevweaponsel = NULL;
		type = ENT_PLAYER;
		clientnum = smoothmillis = followplayercn = wantsswitch = -1;
		name[0] = 0;
		team = TEAM_BLUE;
		maxeyeheight = PLAYERHEIGHT;
		aboveeye = PLAYERABOVEEYE;
		radius = PLAYERRADIUS;
		maxspeed = 16.0f;
		skin_noteam = skin_red = skin_blue = NULL;
		respawn(G_DM, G_M_NONE);
		damagestack.setsize(0);
		wantsreload = delayedscope = false;
	}

	void addicon(int type)
	{
		switch(type){
			case eventicon::CRITICAL:
			case eventicon::PICKUP:
				loopv(icons) if(icons[i].type == type) icons.remove(i--);
				break;
		}
		extern int lastmillis;
		eventicon icon(type, lastmillis);
		icons.add(icon);
	}

	void removeai();

	virtual ~playerent()
	{
		removeai();
		icons.shrink(0);
		extern void removebounceents(playerent *owner);
		extern void detachsounds(playerent *owner);
		extern void removedynlights(physent *owner);
		extern void zapplayerflags(playerent *owner);
		extern void cleanplayervotes(playerent *owner);
		extern physent *camera1;
		extern void togglespect();
		removebounceents(this);
		detachsounds(this);
		removedynlights(this);
		zapplayerflags(this);
		cleanplayervotes(this);
		if(this==camera1) togglespect();
	}

	void damageroll(float damage)
	{
		extern void clamproll(physent *pl);
		float damroll = 2.0f*damage;
		roll += roll>0 ? damroll : (roll<0 ? -damroll : (rnd(2) ? damroll : -damroll)); // give player a kick
		clamproll(this);
	}

	void hitpush(int damage, const vec &dir, int gun, bool slows)
	{
		if(gun<0 || gun>WEAP_MAX || dir.iszero() || !damage) return;
		const float pushf = damage*guns[gun].pushfactor/100.f/HEALTHSCALE;
		vec push = dir;
		push.normalize().mul(pushf);
		if(slows) vel.div(clamp<float>(pushf*5, 1, 5));
		vel.add(push);
		extern int lastmillis;
		if(gun==WEAP_GRENADE && damage > 50 * HEALTHSCALE) eardamagemillis = lastmillis+damage*100/HEALTHSCALE;
	}

	void resetspec()
	{
		spectatemode = SM_NONE;
		followplayercn = -1;
	}

	void respawn(int gamemode, int mutators)
	{
		dynent::reset();
		playerstate::respawn(gamemode, mutators);
		history.reset();
		if(weaponsel) weaponsel->reset();
		lastregen = lasthitmarker = lastaction = weaponchanging = eardamagemillis = radarmillis = flashmillis = 0;
		lastattackweapon = NULL;
		ads = 0.f;
		wantsswitch = -1;
		scoping = attacking = false;
		lastaction = 0;
		resetspec();
		eyeheight = maxeyeheight;
		damagestack.setsize(0);
	}

	void spawnstate(int team, int gamemode, int mutators)
	{
		playerstate::spawnstate(team, gamemode, mutators);
		prevweaponsel = weaponsel = weapons[gunselect];
	}

	void selectweapon(int w) { prevweaponsel = weaponsel = weapons[(gunselect = w)]; }
	void setprimary(int w) { primary = w; }
	bool isspectating() { return team==TEAM_SPECT || (state==CS_DEAD && spectatemode > SM_NONE); }
	void weaponswitch(weapon *w)
	{
		if(!w) return;
		extern playerent *player1;
		if(ads){
			if(this == player1){
				wantsswitch = w->type;
				delayedscope = scoping;
				scoping = false;
			}
			return;
		}
		wantsswitch = -1;
		extern int lastmillis;
		// weaponsel->ondeselecting();
		weaponchanging = lastmillis;
		prevweaponsel = weaponsel;
		nextweaponsel = w;
		extern void addmsg(int type, const char *fmt = NULL, ...);
		if(this == player1 || ownernum == player1->clientnum) addmsg(N_SWITCHWEAP, "ri2", clientnum, w->type);
		w->onselecting();
	}
};

enum { CTFF_INBASE = 0, CTFF_STOLEN, CTFF_DROPPED, CTFF_IDLE };

struct flaginfo
{
	int team;
	entity *flagent;
	int actor_cn;
	playerent *actor;
	vec pos;
	int state; // one of CTFF_*
	flaginfo() : flagent(0), actor(0), state(CTFF_INBASE) {}
};

enum { BT_NONE, BT_NADE, BT_GIB, BT_SHELL, BT_KNIFE };

struct bounceent : physent // nades, gibs
{
	int millis, timetolive, bouncetype; // see enum above
	float rotspeed;
	playerent *owner;

	bounceent() : bouncetype(BT_NONE), rotspeed(1.0f), owner(NULL)
	{
		type = ENT_BOUNCE;
		maxspeed = 40;
		radius = 0.2f;
		eyeheight = maxeyeheight = 0.3f;
		aboveeye = 0.0f;
	}

	virtual ~bounceent() {}

	bool isalive(int lastmillis) { return lastmillis - millis < timetolive; }
	virtual void destroy() {}
	virtual bool applyphysics() { return true; }
};

enum { HIT_NONE = 0, HIT_TORSO, HIT_LEG, HIT_HEAD };

struct grenadeent : bounceent
{
	bool local;
	int nadestate, id;
	float distsincebounce;
	grenadeent(playerent *owner, int millis = 0);
	~grenadeent();
	void activate();
	void _throw(const vec &from, const vec &vel);
	void explode();
	virtual void destroy();
	virtual bool applyphysics();
	void moveoutsidebbox(const vec &direction, playerent *boundingbox);
	void oncollision();
	void onmoved(const vec &dist);
};

struct knifeent : bounceent
{
	bool local;
	int knifestate;
	playerent *hit;
	knifeent(playerent *owner, int millis = 0);
	~knifeent();
	void activate();
	void _throw(const vec &from, const vec &vel);
	void explode();
	virtual void destroy();
	virtual bool applyphysics();
	void moveoutsidebbox(const vec &direction, playerent *boundingbox);
	void oncollision();
	bool trystick(playerent *pl);
};