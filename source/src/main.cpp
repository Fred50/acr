// main.cpp: initialisation & main loop

#include "pch.h"
#include "cube.h"

void cleanup(char *msg)		 // single program exit point;
{
	if(!msg)
	{
		cleanupclient();
		soundcleanup();
		cleanupserver();
		SDL_SetGamma(1, 1, 1);
	}
	SDL_ShowCursor(1);
	if(msg)
	{
		#ifdef WIN32
		MessageBox(NULL, msg, "ACR fatal error", MB_OK|MB_SYSTEMMODAL|MB_ICONERROR);
		#else
		printf("%s", msg);
		#endif
	}
	SDL_Quit();
}

VAR(resetcfg, 0, 0, 1);

void quit()					 // normal exit
{
	extern void writeinitcfg();
	writeinitcfg();
	writeservercfg();
	if(resetcfg) deletecfg();
	else writecfg();
	cleanup(NULL);
	popscontext();
	exit(EXIT_SUCCESS);
}

void fatal(const char *s, ...)	// failure exit
{
	static int errors = 0;
	errors++;

	if(errors <= 2)
	{
		defvformatstring(msg,s,s);
		if(errors <= 1)
		{
			defvformatstring(msg,s,s);
			defformatstring(msgerr)("%s (%s)\n", msg, SDL_GetError());
			cleanup(msgerr);
		}
		else puts(msg);
	}
	exit(EXIT_FAILURE);
}

SDL_Surface *screen = NULL;

static int initing = NOT_INITING;
static bool restoredinits = false;

bool initwarning(const char *desc, int level, int type)
{
	if(initing < level)
	{
		addchange(desc, type);
		return true;
	}
	return false;
}

VARF(scr_w, 320, 1024, 10000, initwarning("screen resolution"));
VARF(scr_h, 200, 768, 10000, initwarning("screen resolution"));
VARF(colorbits, 0, 0, 32, initwarning("color depth"));
VARF(depthbits, 0, 0, 32, initwarning("depth-buffer precision"));
VARF(stencilbits, 0, 0, 32, initwarning("stencil-buffer precision"));
VARF(fsaa, -1, -1, 16, initwarning("anti-aliasing"));
VARF(vsync, -1, -1, 1, initwarning("vertical sync"));

static bool grabinput = false, minimized = false;

void inputgrab(bool on)
{
#ifndef WIN32
	if(!(screen->flags & SDL_FULLSCREEN)) SDL_WM_GrabInput(SDL_GRAB_OFF);
	else
#endif
	SDL_WM_GrabInput(on ? SDL_GRAB_ON : SDL_GRAB_OFF);
	SDL_ShowCursor(on ? SDL_DISABLE : SDL_ENABLE);
}

void setfullscreen(bool enable)
{
	if(!screen) return;
#if defined(WIN32) || defined(__APPLE__)
	initwarning(enable ? "fullscreen" : "windowed");
#else
	if(enable == !(screen->flags&SDL_FULLSCREEN))
	{
		SDL_WM_ToggleFullScreen(screen);
		inputgrab(grabinput);
	}
#endif
}

#ifdef _DEBUG
VARF(fullscreen, 0, 0, 1, setfullscreen(fullscreen!=0));
#else
VARF(fullscreen, 0, 1, 1, setfullscreen(fullscreen!=0));
#endif

void writeinitcfg()
{
	if(!restoredinits) return;
	FILE *f = openfile(path("config/init.cfg", true), "w");
	if(!f) return;
	fprintf(f, "// automatically written on exit, DO NOT MODIFY\n// modify settings in game\n");
	extern int fullscreen;
	fprintf(f, "fullscreen %d\n", fullscreen);
	fprintf(f, "scr_w %d\n", scr_w);
	fprintf(f, "scr_h %d\n", scr_h);
	fprintf(f, "colorbits %d\n", colorbits);
	fprintf(f, "depthbits %d\n", depthbits);
	fprintf(f, "stencilbits %d\n", stencilbits);
	fprintf(f, "fsaa %d\n", fsaa);
	fprintf(f, "vsync %d\n", vsync);
	extern int audio, soundchannels;
	fprintf(f, "audio %d\n", audio > 0 ? 1 : 0);
	fprintf(f, "soundchannels %d\n", soundchannels);
	fclose(f);
}

#if 0
VARP(highprocesspriority, 0, 1, 1);

void setprocesspriority(bool high)
{
#if defined(WIN32) && !defined(_DEBUG)
	SetPriorityClass(GetCurrentProcess(), high && highprocesspriority && fullscreen ? HIGH_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);
#endif
}
#endif

void screenshot(char *imagepath)
{
	SDL_Surface *image = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 24, 0x0000FF, 0x00FF00, 0xFF0000, 0);
	if(!image) return;
	uchar *tmp = new uchar[screen->w*screen->h*3];
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, tmp);
	uchar *dst = (uchar *)image->pixels;
	loopi(scr_h)
	{
		memcpy(dst, &tmp[3*screen->w*(screen->h-i-1)], 3*screen->w);
		endianswap(dst, 3, screen->w);
		dst += image->pitch;
	}
	delete[] tmp;
	if(!imagepath || !imagepath[0])
	{
		static string buf;
		if(getclientmap()[0])
			formatstring(buf)("screenshots/%s_%s_%s.bmp", timestring(), behindpath(getclientmap()), modestr(gamemode, true));
		else
			formatstring(buf)("screenshots/%s.bmp", timestring());
		imagepath = buf;
	}
	const char *found = findfile(path(imagepath), "wb");
	conoutf("writing to file: %s", found);
	SDL_SaveBMP(image, found);
	SDL_FreeSurface(image);
}

COMMAND(screenshot, ARG_1STR);
COMMAND(quit, ARG_NONE);

void screenres(int w, int h)
{
#if !defined(WIN32) && !defined(__APPLE__)
	if(initing >= INIT_RESET)
	{
#endif
		scr_w = w;
		scr_h = h;
#if defined(WIN32) || defined(__APPLE__)
		initwarning("screen resolution");
#else
		return;
	}
	SDL_Surface *surf = SDL_SetVideoMode(w, h, 0, SDL_OPENGL|SDL_RESIZABLE|(screen->flags&SDL_FULLSCREEN));
	if(!surf) return;
	screen = surf;
	scr_w = screen->w;
	scr_h = screen->h;
	glViewport(0, 0, scr_w, scr_h);
	VIRTW = scr_w*VIRTH/scr_h;
#endif
}
#if defined(WIN32) || defined(__APPLE__) || !defined(WIN32)
void setresdata(char *s, enet_uint32 c)
{
	extern hashtable<char *, enet_uint32> &resdata;
	resdata[newstring(s)] = c;
}
#endif

COMMAND(screenres, ARG_2INT);

VARFP(gamma, 30, 100, 300,
{
	float f = gamma/100.0f;
	if(SDL_SetGamma(f,f,f)==-1)
	{
		conoutf("Could not set gamma (card/driver doesn't support it?)");
		conoutf("sdl: %s", SDL_GetError());
	}
});

void resetgamma()
{
	float f = gamma/100.0f;
	if(f==1) return;
	SDL_SetGamma(1, 1, 1);
	SDL_SetGamma(f, f, f);
}

void setupscreen(int &usedcolorbits, int &useddepthbits, int &usedfsaa)
{
	int flags = SDL_RESIZABLE;
	#if defined(WIN32) || defined(__APPLE__)
	flags = 0;
	putenv("SDL_VIDEO_CENTERED=1"); //Center window
	#endif
	if(fullscreen) flags |= SDL_FULLSCREEN;
	SDL_Rect **modes = SDL_ListModes(NULL, SDL_OPENGL|flags);
	if(modes && modes!=(SDL_Rect **)-1)
	{
		bool hasmode = false;
		for(int i = 0; modes[i]; i++)
		{
			if(scr_w <= modes[i]->w && scr_h <= modes[i]->h) { hasmode = true; break; }
		}
		if(!hasmode) { scr_w = modes[0]->w; scr_h = modes[0]->h; }
	}
	bool hasbpp = true;
	if(colorbits && modes)
		hasbpp = SDL_VideoModeOK(modes!=(SDL_Rect **)-1 ? modes[0]->w : scr_w, modes!=(SDL_Rect **)-1 ? modes[0]->h : scr_h, colorbits, SDL_OPENGL|flags)==colorbits;

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#if SDL_VERSION_ATLEAST(1, 2, 11)
	if(vsync>=0) SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, vsync);
#endif
	static int configs[] =
	{
		0x7, /* try everything */
		0x6, 0x5, 0x3, /* try disabling one at a time */
		0x4, 0x2, 0x1, /* try disabling two at a time */
		0 /* try disabling everything */
	};
	int config = 0;
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	if(!depthbits) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	if(!fsaa)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	}
	loopi(sizeof(configs)/sizeof(configs[0]))
	{
		config = configs[i];
		if(!depthbits && config&1) continue;
		if(!stencilbits && config&2) continue;
		if(fsaa<=0 && config&4) continue;
		if(depthbits) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, config&1 ? depthbits : 16);
		if(stencilbits)
		{
			SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, config&2 ? stencilbits : 0);
			hasstencil = (config&2)!=0;
		}
		else hasstencil = false;
		if(fsaa>0)
		{
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, config&4 ? 1 : 0);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, config&4 ? fsaa : 0);
		}
		screen = SDL_SetVideoMode(scr_w, scr_h, hasbpp ? colorbits : 0, SDL_OPENGL|flags);
		if(screen) break;
	}
	if(!screen) fatal("Unable to create OpenGL screen");
	else
	{
		if(!hasbpp) conoutf("%d bit color buffer not supported - disabling", colorbits);
		if(depthbits && (config&1)==0) conoutf("%d bit z-buffer not supported - disabling", depthbits);
		if(stencilbits && (config&2)==0) conoutf("%d bit stencil buffer not supported - disabling", stencilbits);
		if(fsaa>0 && (config&4)==0) conoutf("%dx anti-aliasing not supported - disabling", fsaa);
	}

	scr_w = screen->w;
	scr_h = screen->h;
	VIRTW = scr_w*VIRTH/scr_h;

	#ifdef WIN32
	SDL_WM_GrabInput(SDL_GRAB_ON);
	#else
	SDL_WM_GrabInput(fullscreen ? SDL_GRAB_ON : SDL_GRAB_OFF);
	#endif

	usedcolorbits = hasbpp ? colorbits : 0;
	useddepthbits = config&1 ? depthbits : 0;
	usedfsaa = config&2 ? fsaa : 0;
}

void resetgl()
{
	clearchanges(CHANGE_GFX);

	loadingscreen();

	extern void cleanupparticles();
	extern void cleanupmodels();
	extern void cleanuptextures();
	extern void cleanuptmus();
	extern void cleanupgl();
	cleanupparticles();
	cleanupmodels();
	cleanuptextures();
	cleanuptmus();
	cleanupgl();
	c2skeepalive();

	SDL_SetVideoMode(0, 0, 0, 0);

	int usedcolorbits = 0, useddepthbits = 0, usedfsaa = 0;
	setupscreen(usedcolorbits, useddepthbits, usedfsaa);
	gl_init(scr_w, scr_h, usedcolorbits, useddepthbits, usedfsaa);

	extern void reloadfonts();
	extern void reloadtextures();
	c2skeepalive();
	if(!reloadtexture(*notexture) ||
	   !reloadtexture("packages/misc/startscreen.png"))
		fatal("failed to reload core texture");
	loadingscreen();
	c2skeepalive();
	resetgamma();
	c2skeepalive();
	// preload some stuffs
	extern Texture **obittex();
	extern void load_waypointtex();
	geteventicons(); getperktex1(); getperktex2(); obittex(); load_waypointtex();
	c2skeepalive();
	reloadfonts();
	reloadtextures();
	c2skeepalive();
	preload_playermodels();
	c2skeepalive();
	preload_hudguns();
	c2skeepalive();
	preload_entmodels();
	c2skeepalive();
	preload_mapmodels();
	c2skeepalive();
}

COMMAND(resetgl, ARG_NONE);

VARP(maxfps, 0, 0, 1000);

void limitfps(int &millis, int curmillis)
{
	if(!maxfps) return;
	static int fpserror = 0;
	int delay = 1000/maxfps - (millis-curmillis);
	if(delay < 0) fpserror = 0;
	else
	{
		fpserror += 1000%maxfps;
		if(fpserror >= maxfps)
		{
			++delay;
			fpserror -= maxfps;
		}
		if(delay > 0)
		{
			SDL_Delay(delay);
			millis += delay;
		}
	}
}

int lowfps = 30, highfps = 40;

void fpsrange(int low, int high)
{
	if(low>high || low<1) return;
	lowfps = low;
	highfps = high;
}

COMMAND(fpsrange, ARG_2INT);

void keyrepeat(bool on)
{
	SDL_EnableKeyRepeat(on ? SDL_DEFAULT_REPEAT_DELAY : 0,
							 SDL_DEFAULT_REPEAT_INTERVAL);
}

vector<SDL_Event> events;

bool interceptkey(int sym)
{
	static int lastintercept = SDLK_UNKNOWN;
	int len = lastintercept == sym ? events.length() : 0;
	SDL_Event event;
	while(SDL_PollEvent(&event)) switch(event.type)
	{
		case SDL_MOUSEMOTION: break;
		default: events.add(event); break;
	}
	lastintercept = sym;
	if(sym != SDLK_UNKNOWN) for(int i = len; i < events.length(); i++)
	{
		if(events[i].type == SDL_KEYDOWN && events[i].key.keysym.sym == sym) { events.remove(i); return true; }
	}
	return false;
}

static void resetmousemotion()
{
#ifndef WIN32
	if(!(screen->flags&SDL_FULLSCREEN))
	{
		SDL_WarpMouse(screen->w / 2, screen->h / 2);
	}
#endif
}

static inline bool skipmousemotion(SDL_Event &event)
{
	if(event.type != SDL_MOUSEMOTION) return true;
#ifndef WIN32
	if(!(screen->flags&SDL_FULLSCREEN))
	{
		#ifdef __APPLE__
		if(event.motion.y == 0) return true;  // let mac users drag windows via the title bar
		#endif
		if(event.motion.x == screen->w / 2 && event.motion.y == screen->h / 2) return true;  // ignore any motion events generated SDL_WarpMouse
	}
#endif
	return false;
}

static void checkmousemotion(int &dx, int &dy)
{
	loopv(events)
	{
		SDL_Event &event = events[i];
		if(skipmousemotion(event))
		{
			if(i > 0) events.remove(0, i);
			return;
		}
		dx += event.motion.xrel;
		dy += event.motion.yrel;
	}
	events.setsize(0);
	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		if(skipmousemotion(event))
		{
			events.add(event);
			return;
		}
		dx += event.motion.xrel;
		dy += event.motion.yrel;
	}
}

static int ignoremouse = 5;

void checkinput()
{
	SDL_Event event;
	int lasttype = 0, lastbut = 0;
	int tdx=0, tdy=0;
	while(events.length() || SDL_PollEvent(&event))
	{
		if(events.length()) event = events.remove(0);

		switch(event.type)
		{
			case SDL_QUIT:
				quit();
				break;

			#if !defined(WIN32) && !defined(__APPLE__)
			case SDL_VIDEORESIZE:
				screenres(event.resize.w, event.resize.h);
				break;
			#endif

			case SDL_KEYDOWN:
			case SDL_KEYUP:
				keypress(event.key.keysym.sym, event.key.state==SDL_PRESSED, event.key.keysym.unicode, event.key.keysym.mod);
				break;

			case SDL_ACTIVEEVENT:
				if(event.active.state & SDL_APPINPUTFOCUS)
					inputgrab(grabinput = event.active.gain!=0);
				if(event.active.state & SDL_APPACTIVE)
					minimized = !event.active.gain;
#if 0
				if(event.active.state==SDL_APPMOUSEFOCUS) setprocesspriority(event.active.gain > 0); // switch priority on focus change
#endif
				break;

			case SDL_MOUSEMOTION:
				if(ignoremouse) { ignoremouse--; break; }
				if(grabinput && !skipmousemotion(event))
				{
					int dx = event.motion.xrel, dy = event.motion.yrel;
					checkmousemotion(dx, dy);
					resetmousemotion();
					tdx+=dx;tdy+=dy;
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if(lasttype==event.type && lastbut==event.button.button) break; // why?? get event twice without it
				keypress(-event.button.button, event.button.state!=0, 0);
				lasttype = event.type;
				lastbut = event.button.button;
				break;
		}
	}
	if(tdx || tdy) mousemove(tdx, tdy);
}

VARF(gamespeed, 10, 100, 1000, if(multiplayer()) gamespeed = 100);
VARF(paused, 0, 0, 1, if(multiplayer()) paused = 0);

bool firstrun = false, inmainloop = false;
static int clockrealbase = 0, clockvirtbase = 0;
static void clockreset() { clockrealbase = SDL_GetTicks(); clockvirtbase = totalmillis; }
VARFP(clockerror, 990000, 1000000, 1010000, clockreset());
VARFP(clockfix, 0, 0, 1, clockreset());

const char *rndmapname()
{
	extern cvector found_map_files;
	if(!found_map_files.length()) return "ac_desert";
	return found_map_files[rnd(found_map_files.length())];
}

int main(int argc, char **argv)
{
	extern struct servercommandline scl;
	#ifdef WIN32
	//atexit((void (__cdecl *)(void))_CrtDumpMemoryLeaks);
	#ifndef _DEBUG
	#ifndef __GNUC__
	__try {
	#endif
	#endif
	#endif

	bool dedicated = false;
	bool quitdirectly = false;

	const char *initmap = NULL;

	pushscontext(IEXC_CFG);

	#define initlog(s) puts("init: " s)

	initing = INIT_RESET;
	for(int i = 1; i<argc; i++)
	{
		if(!scl.checkarg(argv[i]))
		{
			char *a = &argv[i][2];
			if(argv[i][0]=='-') switch(argv[i][1])
			{
				case '-':
					if(!strncmp(argv[i], "--home=", 7))
					{
						sethomedir(&argv[i][7]);
					}
					else if(!strncmp(argv[i], "--mod=", 6))
					{
						addpackagedir(&argv[i][6]);
					}
					else if(!strcmp(argv[i], "--init"))
					{
						execfile((char *)"config/init.cfg");
						restoredinits = true;
					}
					else if(!strncmp(argv[i], "--init=", 7))
					{
						execfile(&argv[i][7]);
						restoredinits = true;
					}
					else if(!strcmp(argv[i], "--version"))
					{
						printf("%.3f\n", AC_VERSION/1000.0f);
						quitdirectly = true;
					}
					else if(!strcmp(argv[i], "--protocol"))
					{
						printf("%d\n", PROTOCOL_VERSION);
						quitdirectly = true;
					}
					else if(!strncmp(argv[i], "--loadmap=", 10))
					{
						initmap = &argv[i][10];
					}
					else conoutf("\f3unknown commandline switch: %s", argv[i]);
					break;
				case 'd': dedicated = true; break;
				case 't': fullscreen = atoi(a); break;
				case 'w': scr_w  = atoi(a); break;
				case 'h': scr_h  = atoi(a); break;
				case 'z': depthbits = atoi(a); break;
				case 'b': colorbits = atoi(a); break;
				case 's': stencilbits = atoi(&argv[i][2]); break;
				case 'a': fsaa = atoi(a); break;
				case 'v': vsync = atoi(a); break;
				default:  conoutf("unknown commandline option");
			}
			else conoutf("unknown commandline argument");
		}
	}
	if(!quitdirectly)
	{

		initing = NOT_INITING;

		initlog("sdl");
		int par = 0;
		#ifdef _DEBUG
		par = SDL_INIT_NOPARACHUTE;
		#endif
		if(SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO|par)<0) fatal("Unable to initialize SDL");

	#if 0
		if(highprocesspriority) setprocesspriority(true);
	#endif

		initlog("net");
		if(enet_initialize()<0) fatal("Unable to initialise network module");

		initclient();
		initserver(dedicated);  // never returns if dedicated

		initlog("world");
		empty_world(7, true);

		initlog("video: sdl");
		if(SDL_InitSubSystem(SDL_INIT_VIDEO)<0) fatal("Unable to initialize SDL Video");

		initlog("video: mode");
		int usedcolorbits = 0, useddepthbits = 0, usedfsaa = 0;
		setupscreen(usedcolorbits, useddepthbits, usedfsaa);

		initlog("video: misc");
		SDL_WM_SetCaption("AssaultCube Reloaded", NULL);
		keyrepeat(false);
		SDL_ShowCursor(0);

		initlog("gl");
		gl_checkextensions();
		gl_init(scr_w, scr_h, usedcolorbits, useddepthbits, usedfsaa);

		notexture = noworldtexture = textureload("packages/misc/notexture.jpg");
		if(!notexture) fatal("could not find core textures (hint: run ACR from the parent of the bin directory)");

		initlog("console");
		persistidents = false;
		if(!execfile("config/font.cfg")) fatal("cannot find font definitions");
		if(!setfont("default")) fatal("no default font specified");

		loadingscreen();

		particleinit();

		initlog("sound");
		initsound();

		initlog("cfg");
		extern void *scoremenu, *teammenu, *ctfmenu, *servmenu, *searchmenu, *serverinfomenu, *kickmenu, *banmenu, *forceteammenu, *giveadminmenu, *revokemenu, *whoismenu, *spectmenu, *docmenu, *applymenu;
		scoremenu = addmenu("score", "score\tfrag\tassist\tdeath\tratio\tpj/ping\trank\tlvl\tname", false, renderscores, NULL, false, true);
		teammenu = addmenu("team score", "score\tfrag\tassist\tdeath\tratio\tpj/ping\trank\tlvl\tname", false, renderscores, NULL, false, true);
		ctfmenu = addmenu("flag score", "score\tflag\tfrag\tassist\tdeath\tratio\tpj/ping\trank\tlvl\tname", false, renderscores, NULL, false, true);
		servmenu = addmenu("server", NULL, true, refreshservers, serverskey);
		searchmenu = addmenu("search", NULL, true, refreshservers, serverskey);
		serverinfomenu = addmenu("serverinfo", NULL, true, refreshservers, serverinfokey);
		kickmenu = addmenu("kick player", NULL, true, refreshsopmenu);
		banmenu = addmenu("ban player", NULL, true, refreshsopmenu);
		forceteammenu = addmenu("force team", NULL, true, refreshsopmenu);
		giveadminmenu = addmenu("give permission", NULL, true, refreshsopmenu);
		revokemenu = addmenu("revoke privilege", NULL, true, refreshsopmenu);
		whoismenu = addmenu("WHOIS", NULL, true, refreshsopmenu);
		spectmenu = addmenu("toggle spect", NULL, true, refreshsopmenu);
		docmenu = addmenu("reference", NULL, true, renderdocmenu);
		applymenu = addmenu("apply", "apply changes now?", true, refreshapplymenu);

		exec("config/scontext.cfg");
		exec("config/keymap.cfg");
		exec("config/scripts.cfg");
		exec("config/prefabs.cfg");
		exec("config/sounds.cfg");
		exec("config/securemaps.cfg");
		exec("config/admin.cfg");
		execfile("config/servers.cfg");
		persistidents = true;

		static char resdata[] = { 112, 97, 99, 107, 97, 103, 101, 115, 47, 116, 101, 120, 116, 117, 114, 101, 115, 47, 107, 117, 114, 116, 47, 107, 108, 105, 116, 101, 50, 46, 106, 112, 103, 0 };
		gzFile f = gzopen(resdata, "rb9");
		if(f)
		{
			int n;
			gzread(f, &n, sizeof(int));
			endianswap(&n, sizeof(int), 1);
			loopi(n)
			{
				string s;
				gzread(f, s, sizeof(string));
				enet_uint32 c;
				gzread(f, &c, sizeof(enet_uint32));
				setresdata(s, c);
			}
			gzclose(f);
		}

		initing = INIT_LOAD;
		if(!execfile("config/saved.cfg"))
		{
			exec("config/defaults.cfg");
			firstrun = true;
		}
		execfile("config/autoexec.cfg");
		execute("addallfavcatmenus");  // exec here, to add all categories (including those defined in autoexec.cfg)
		initing = NOT_INITING;

		persistidents = false;
		execfile("locale/_.cfg"); // load locale after settings, before menus
		exec("config/menus.cfg");
		persistidents = true;

		initlog("models");
		preload_playermodels();
		preload_hudguns();
		preload_entmodels();
		// preload some stuffs
		extern Texture **obittex();
		extern void load_waypointtex();
		geteventicons(); getperktex1(); getperktex2(); obittex(); load_waypointtex();

		initlog("docs");
		persistidents = false;
		execfile("config/docs.cfg");
		persistidents = true;

		initlog("localconnect");
		localconnect();
		changemap(initmap ? initmap : rndmapname());

		initlog("mainloop");

		inputgrab(grabinput = true);

		inmainloop = true;
	#ifdef _DEBUG
		int lastflush = 0;
	#endif
		for(;;)
		{
			static int frames = 0;
			static float fps = 10.0f;
			int millis = SDL_GetTicks() - clockrealbase;
			if(clockfix) millis = int(millis*(double(clockerror)/1000000));
			millis += clockvirtbase;
			if(millis<totalmillis) millis = totalmillis;
			limitfps(millis, totalmillis);
			int elapsed = millis-totalmillis;
			if(multiplayer(false)) curtime = elapsed;
			else
			{
				static int timeerr = 0;
				int scaledtime = elapsed*gamespeed + timeerr;
				curtime = scaledtime/100;
				timeerr = scaledtime%100;
				if(paused) curtime = 0;
			}
			lastmillis += curtime;
			totalmillis = millis;

			checkinput();

			if(lastmillis) updateworld(curtime, lastmillis);

			serverslice(0);

			fps = (1000.0f/max(1,elapsed)+fps*10)/11;
			frames++;

			updateaudio();

			computeraytable(camera1->o.x, camera1->o.y, dynfov());
			if(frames>3)
			{
				gl_drawframe(screen->w, screen->h, fps<lowfps ? fps/lowfps : (fps>highfps ? fps/highfps : 1.0f), fps);
				if(frames>4) SDL_GL_SwapBuffers();
			}

	#ifdef _DEBUG
			if(millis>lastflush+60000) {
				fflush(stdout); lastflush = millis;
			}
	#endif
		}

		quit();
	}
	return EXIT_SUCCESS;

	#if defined(WIN32) && !defined(_DEBUG) && !defined(__GNUC__)
	} __except(stackdumper(0, GetExceptionInformation()), EXCEPTION_CONTINUE_SEARCH) { return 0; }
	#endif
}

VAR(version, 1, AC_VERSION, 0);
