/* See LICENSE file for copyright and license details. */
#include <X11/XF86keysym.h>

/* Appearance */
static const unsigned int borderpx  = 2;        /* window border width in pixels */
static const unsigned int snap      = 15;       /* snap distance to screen edges */
static const unsigned int gappih    = 10;       /* horizontal gap between windows */
static const unsigned int gappiv    = 10;       /* vertical gap between windows */
static const unsigned int gappoh    = 10;       /* horizontal gap around screen edge */
static const unsigned int gappov    = 10;       /* vertical gap around screen edge */
static int smartgaps                = 0;        /* 1 = remove outer gaps when only one window is open */
static const int showbar            = 1;        /* 1 = show bar, 0 = hide */
static const int topbar             = 1;        /* 1 = bar at top, 0 = bar at bottom */
static const char *fonts[]          = { "IosevkaTerm Nerd Font Mono:style=Regular:size=18" };
static const char dmenufont[]       = "monospace:size=10";
static const char bg[]              = "#101010"; /* bar background */
static const char fg[]              = "#e4e4ef"; /* main text color */
static const char gray5[]           = "#484440"; /* inactive workspace tags */
static const char col_border[]      = "#484440"; /* unfocused window border */
static const char accent[]          = "#95959f"; /* focused window border and accents */
static const char occ_fg[]          = "#95959f"; /* workspace tag with open windows */
static const char cap_bg[]          = "#101010"; /* status pill icon background */
static const char cap_fg[]          = "#95959f"; /* status pill icon color */
static const char val_bg[]          = "#101010"; /* status pill text background */
static const char val_fg[]          = "#eef1f6"; /* status pill text color */
static const char *colors[][3]      = {
	/*               fg         bg        border   */
	[SchemeNorm] = { fg,        bg,       col_border },
	[SchemeSel]  = { accent,    bg,       accent },
	[SchemeTag]  = { gray5,     bg,       bg },
	[SchemeOcc]  = { occ_fg,    bg,       bg },
	[SchemeTag1] = { accent,    bg,       bg },
	[SchemeTag2] = { accent,    bg,       bg },
	[SchemeTag3] = { accent,    bg,       bg },
	[SchemeTag4] = { accent,    bg,       bg },
	[SchemeTag5] = { accent,    bg,       bg },
	[SchemeTag6] = { accent,    bg,       bg },
	[SchemeTag7] = { accent,    bg,       bg },
	[SchemeTag8] = { accent,    bg,       bg },
	[SchemeTag9] = { accent,    bg,       bg },
};

/* Bar style */
static const int user_bh            = 30;  /* bar height in px (0 = auto from font) */
static const int pill_vpad          = 6;   /* top/bottom padding for status pills */
static const int pill_hpad          = 6;   /* left/right padding inside status pills */
static const int floatbar           = 0;   /* 1 = floating bar, 0 = full width */
static const int vertpadbar         = 0;   /* vertical gap around floating bar */
static const int horizpadbar        = 0;   /* horizontal gap around floating bar */
static const int colorfultag        = 0;   /* 1 = colorful tags, 0 = single accent color */
static const unsigned int ulinepad     = 5; /* left/right padding for tag underline */
static const unsigned int ulinestroke  = 2; /* tag underline thickness in px */
static const unsigned int ulinevoffset = 1; /* distance from bottom of bar */

static const int tagschemes[] = {
	SchemeTag1, SchemeTag2, SchemeTag3,
	SchemeTag4, SchemeTag5, SchemeTag6,
	SchemeTag7, SchemeTag8, SchemeTag9,
};

/* Workspaces */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

/* Window rules: use `xprop` to find WM_CLASS and WM_NAME */
static const Rule rules[] = {
	/* class      instance    title       tags mask     isfloating   monitor */
	{ "Gimp",     NULL,       NULL,       0,            1,           -1 },
	{ "Firefox",  NULL,       NULL,       1 << 8,       0,           -1 },
};

/* Layout settings */
static const float mfact        = 0.55; /* master area width (0.05 to 0.95) */
static const int nmaster        = 1;    /* number of windows in master area */
static const int resizehints    = 1;    /* 1 = respect window size hints when tiling */
static const int lockfullscreen = 1;    /* 1 = keep focus on fullscreen window */
static const int refreshrate    = 120;  /* refresh rate (Hz) for smooth resize/drag */

#include "vanitygaps.c"

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[T]",      tile },    /* default tiled layout */
	{ "[F]",      NULL },    /* floating layout */
	{ "[M]",      monocle }, /* monocle (single full window) layout */
};

/* Key definitions */
#define MODKEY Mod1Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* Helper to run shell commands */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* Custom commands */
static char dmenumon[16] = "0"; /* monitor dmenu opens on */
static const char *dmenucmd[]       = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", bg, "-nf", fg, "-sb", accent, "-sf", bg, NULL };
static const char *termcmd[]        = { "alacritty", NULL };
static const char *rofi[]           = { "rofi", "-show", "drun", NULL };
static const char *upvol[]          = { "wpctl", "set-volume", "@DEFAULT_SINK@", "5%+", NULL };
static const char *downvol[]        = { "wpctl", "set-volume", "@DEFAULT_SINK@", "5%-", NULL };
static const char *mutevol[]        = { "wpctl", "set-mute", "@DEFAULT_SINK@", "toggle", NULL };
static const char *brupcmd[]        = { "brightnessctl", "set", "5%+", NULL };
static const char *brdowncmd[]      = { "brightnessctl", "set", "5%-", NULL };

static const Key keys[] = {
	/* modifier                     key                       function        argument */
	/* Applications */
	{ MODKEY,                       XK_p,                     spawn,          {.v = dmenucmd } },
	{ MODKEY,                       XK_d,                     spawn,          {.v = rofi } },
	{ MODKEY,                       XK_Return,                spawn,          {.v = termcmd } },
	{ MODKEY,                       XK_s,                     spawn,          SHCMD("scrot -d 3 '%Y-%m-%d-%s_$wx$h.jpg' -e 'mv $f ~/Screenshots/'") },

	/* Volume & Brightness */
	{ 0,                            XF86XK_AudioLowerVolume,  spawn,          {.v = downvol } },
	{ 0,                            XF86XK_AudioRaiseVolume,  spawn,          {.v = upvol } },
	{ 0,                            XF86XK_AudioMute,         spawn,          {.v = mutevol } },
	{ 0,                            XF86XK_MonBrightnessUp,   spawn,          {.v = brupcmd } },
	{ 0,                            XF86XK_MonBrightnessDown, spawn,          {.v = brdowncmd } },

	/* Window management */
	{ MODKEY,                       XK_b,                     togglebar,      {0} },
	{ MODKEY,                       XK_j,                     focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,                     focusstack,     {.i = -1 } },
	{ MODKEY,                       XK_i,                     incnmaster,     {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_d,                     incnmaster,     {.i = -1 } },
	{ MODKEY,                       XK_h,                     setmfact,       {.f = -0.05 } },
	{ MODKEY,                       XK_l,                     setmfact,       {.f = +0.05 } },
	{ MODKEY|ShiftMask,             XK_h,                     setcfact,       {.f = +0.25 } },
	{ MODKEY|ShiftMask,             XK_l,                     setcfact,       {.f = -0.25 } },
	{ MODKEY|ShiftMask,             XK_o,                     setcfact,       {.f =  0.00 } },
	{ MODKEY,                       XK_z,                     zoom,           {0} },
	{ MODKEY,                       XK_Tab,                   view,           {0} },
	{ MODKEY,                       XK_q,                     killclient,     {0} },

	/* Layouts & Fullscreen */
	{ MODKEY,                       XK_t,                     setlayout,      {.v = &layouts[0]} },
	{ MODKEY,                       XK_f,                     setlayout,      {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,                     setlayout,      {.v = &layouts[2]} },
	{ MODKEY,                       XK_space,                 setlayout,      {0} },
	{ MODKEY|ShiftMask,             XK_space,                 togglefloating, {0} },
	{ MODKEY|ShiftMask,             XK_f,                     togglefullscr,  {0} },
	{ MODKEY|ControlMask,           XK_f,                     togglefakefullscreen, {0} },

	/* Window gaps */
	{ MODKEY|ControlMask,           XK_equal,                 incrgaps,       {.i = +2 } },
	{ MODKEY|ControlMask,           XK_minus,                 incrgaps,       {.i = -2 } },
	{ MODKEY|ControlMask|ShiftMask, XK_equal,                 incrigaps,      {.i = +2 } },
	{ MODKEY|ControlMask|ShiftMask, XK_minus,                 incrigaps,      {.i = -2 } },
	{ MODKEY|ControlMask,           XK_g,                     togglegaps,     {0} },
	{ MODKEY|ControlMask|ShiftMask, XK_g,                     defaultgaps,    {0} },

	/* Multi-monitor */
	{ MODKEY,                       XK_comma,                 focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_period,                focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,                 tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period,                tagmon,         {.i = +1 } },

	/* Workspaces */
	{ MODKEY,                       XK_0,                     view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,                     tag,            {.ui = ~0 } },
	TAGKEYS(                        XK_1,                                     0)
	TAGKEYS(                        XK_2,                                     1)
	TAGKEYS(                        XK_3,                                     2)
	TAGKEYS(                        XK_4,                                     3)
	TAGKEYS(                        XK_5,                                     4)
	TAGKEYS(                        XK_6,                                     5)
	TAGKEYS(                        XK_7,                                     6)
	TAGKEYS(                        XK_8,                                     7)
	TAGKEYS(                        XK_9,                                     8)

	/* Restart & Quit */
	{ MODKEY|ShiftMask,             XK_q,                     quit,           {0} },
	{ MODKEY|ShiftMask,             XK_r,                     quit,           {1} },
};

/* Mouse bindings */
/* click targets: ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, ClkRootWin */
static const Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd} },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

/* Status bar */
#define DELIMITER " | "

/* Status blocks:
 * icon     - label or icon shown before value
 * cmd      - shell command to run in background (NULL if using func)
 * func     - native C function (NULL if using cmd)
 * interval - update time in seconds (0 = run once at startup)
 */
static Block status_blocks[] = {
	/* icon     command                                                                func             interval */
	{ " ",     "free -h | awk '/^Mem:/ {print $3}'",                                  NULL,            15 },
	{ "󰁹 ",     "cat /sys/class/power_supply/BAT*/capacity 2>/dev/null | head -1 | sed 's/$/%/'", NULL, 30 },
	{ "󱑆",      "date '+%H:%M'",                                                       NULL,            30 },
};

/* Programs to run on dwm startup */
static const char *const autostart[] = {
	/* "pipewire", NULL, */
	/* "wireplumber", NULL, */
	/* "dwmblocks", NULL, */
	NULL /* end of list */
};
