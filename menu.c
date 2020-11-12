/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "quakedef.h"
#include "cdaudio.h"
#include "image.h"
#include "progsvm.h"

#include "mprogdefs.h"

#define TYPE_DEMO 1
#define TYPE_GAME 2
#define TYPE_BOTH 3

static cvar_t forceqmenu = { 0, "forceqmenu", "0", "enables the quake menu instead of the quakec menu.dat (if present)" };
static cvar_t menu_progs = { 0, "menu_progs", "menu.dat", "name of quakec menu.dat file" };

static int NehGameType;

enum m_state_e m_state;
char m_return_reason[128];

void M_Menu_Main_f (void);
	void M_Menu_SinglePlayer_f (void);
		void M_Menu_Load_f (void);
		void M_Menu_Save_f (void);
	void M_Menu_MultiPlayer_f (void);
	void M_Menu_Options_f (void);
	void M_Menu_Options_Effects_f (void);
	void M_Menu_Options_Graphics_f (void);
	void M_Menu_Options_ColorControl_f (void);
		void M_Menu_Keys_f (void);
		void M_Menu_Reset_f (void);
		void M_Menu_Video_f (void);
	void M_Menu_Help_f (void);
void M_Menu_LanConfig_f (void);
void M_Menu_GameOptions_f (void);
void M_Menu_ServerList_f (void);

static void M_Main_Draw (void);
	static void M_SinglePlayer_Draw (void);
		static void M_Load_Draw (void);
		static void M_Save_Draw (void);
	static void M_MultiPlayer_Draw (void);
	static void M_Options_Draw (void);
	static void M_Options_Effects_Draw (void);
	static void M_Options_Graphics_Draw (void);
	static void M_Options_ColorControl_Draw (void);
		static void M_Keys_Draw (void);
		static void M_Reset_Draw (void);
		static void M_Video_Draw (void);
	static void M_Help_Draw (void);
static void M_LanConfig_Draw (void);
static void M_GameOptions_Draw (void);
static void M_ServerList_Draw (void);


static void M_Main_Key (int key, int ascii);
	static void M_SinglePlayer_Key (int key, int ascii);
		static void M_Load_Key (int key, int ascii);
		static void M_Save_Key (int key, int ascii);
	static void M_MultiPlayer_Key (int key, int ascii);
	static void M_Options_Key (int key, int ascii);
	static void M_Options_Effects_Key (int key, int ascii);
	static void M_Options_Graphics_Key (int key, int ascii);
	static void M_Options_ColorControl_Key (int key, int ascii);
		static void M_Keys_Key (int key, int ascii);
		static void M_Reset_Key (int key, int ascii);
		static void M_Video_Key (int key, int ascii);
	static void M_Help_Key (int key, int ascii);
static void M_LanConfig_Key (int key, int ascii);
static void M_GameOptions_Key (int key, int ascii);
static void M_ServerList_Key (int key, int ascii);

static int CursorX, CursorY, mouseX, mouseY, LastCursorX, LastCursorY;
static double LastCheck;
void GetMyCursorPos()
{
	if (LastCheck + 0.25 < realtime)
	{
		LastCursorX = CursorX;
		LastCursorY = CursorY;
		LastCheck = realtime;
	}
	if (SDL_GetMouseState(&mouseX, &mouseY))
	{
		CursorX = mouseX * 1000 / vid.mode.width;
		CursorY = mouseY * 1000 / vid.mode.height;
	}
}

static void CursorMouseMove(int* cursor, int amount)
{
	if (LastCursorX != CursorX && LastCursorY != CursorY)
	{
		if (CursorX > 130 && CursorX < 540)
		{
			if (CursorY > 470 && CursorY <= 540) *cursor = 0;
			else if (amount > 1 && CursorY > 540 && CursorY <= 602) *cursor = 1;
			else if (amount > 2 && CursorY > 602 && CursorY <= 666) *cursor = 2;
			else if (amount > 3 && CursorY > 666 && CursorY <= 732) *cursor = 3;
			else if (amount > 4 && CursorY > 732 && CursorY <= 800) *cursor = 4;
		}
	}
}

static void CursorMouseMoveOptions(int* cursor, int StartY, int EndY)
{
	if (LastCursorX != CursorX && LastCursorY != CursorY)
	if (CursorX > 310 && CursorX < 800 && CursorY > StartY && CursorY < EndY) *cursor = (CursorY - StartY) / 25; //286 Y start; 25 heigh of element
}
static void CursorMouseMoveDisplay(int* cursor)
{
	if (LastCursorX != CursorX && LastCursorY != CursorY)
	{
		if (CursorX > 310 && CursorX < 800 && CursorY > 400 && CursorY < 450) *cursor = 0;
		else if (CursorX > 310 && CursorX < 800 && CursorY > 460 && CursorY < 684) *cursor = (CursorY - 460) / 25 + 1; //286 Y start; 25 heigh of element
	}
}
static void CursorMouseMoveServers(int* cursor, int StartY, int EndY, int max)
{
	int r = 0;
	if (LastCursorX != CursorX && LastCursorY != CursorY)
		if (CursorX > 220 && CursorX < 780 && CursorY > StartY && CursorY < EndY)
		{
			r = (CursorY - StartY) / 50;
			if (r > max) r = max;
			*cursor = r;
		}
}

static qboolean	m_entersound;		///< play after drawing a frame, so caching won't disrupt the sound

void M_Update_Return_Reason(const char *s)
{
	strlcpy(m_return_reason, s, sizeof(m_return_reason));
	if (s) Con_DPrintf("%s\n", s);
}

#define StartingGame	(m_multiplayer_cursor == 1)
#define JoiningGame		(m_multiplayer_cursor == 0)


static float menu_x, menu_y, menu_width, menu_height;
static void M_Background(int width, int height)
{
	menu_width = bound(1.0f, (float)width, vid_conwidth.value);
	menu_height = bound(1.0f, (float)height, vid_conheight.value);
	menu_x = 0;			//menu_x = (vid_conwidth.integer - menu_width) * 0.5;
	menu_y = (vid_conheight.integer - menu_height);	//menu_y = (vid_conheight.integer - menu_height) * 0.5;
	DrawQ_Fill(0, 0, vid_conwidth.integer, vid_conheight.integer, 0, 0, 0, 0.5, 0);
}
static void M_Background2(int width, int height)
{
	menu_width =  bound(1.0f, (float)width, vid_conwidth.value);
	menu_height = bound(1.0f, (float)height, vid_conheight.value);
	menu_x = (vid_conwidth.integer - menu_width) * 0.5;
	menu_y = (vid_conheight.integer - menu_height) * 0.5;
	DrawQ_Fill(0, 0, vid_conwidth.integer, vid_conheight.integer, 0, 0, 0, 0.5, 0);
}
/*
================
M_DrawCharacter

Draws one solid graphics character
================
*/
static void M_DrawCharacter (float cx, float cy, int num)
{
	char temp[2];
	temp[0] = num;
	temp[1] = 0;
	DrawQ_String(menu_x + cx, menu_y + cy, temp, 1, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
}

static void M_PrintColored(float cx, float cy, const char *str)
{
	DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 1, 1, 1, 1, 0, NULL, false, FONT_MENU);
}

static void M_Print(float cx, float cy, const char *str)
{
	DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
}

static void M_PrintRed(float cx, float cy, const char *str)
{
	DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 1, 0, 0, 1, 0, NULL, true, FONT_MENU);
}

static void M_ItemPrint(float cx, float cy, const char *str, int unghosted)
{
	if (unghosted)
		DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
	else
		DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 0.4, 0.4, 0.4, 1, 0, NULL, true, FONT_MENU);
}

static void M_DrawPic(float cx, float cy, const char *picname)
{
	DrawQ_Pic(menu_x + cx, menu_y + cy, Draw_CachePic (picname), 0, 0, 1, 1, 1, 1, 0);
}

static void M_DrawTextBox(float x, float y, float width, float height)
{
	int n;
	float cx, cy;

	// draw left side
	cx = x;
	cy = y;
	M_DrawPic (cx, cy, "gfx/box_tl");
	for (n = 0; n < height; n++)
	{
		cy += 8;
		M_DrawPic (cx, cy, "gfx/box_ml");
	}
	M_DrawPic (cx, cy+8, "gfx/box_bl");

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		M_DrawPic (cx, cy, "gfx/box_tm");
		for (n = 0; n < height; n++)
		{
			cy += 8;
			if (n >= 1)
				M_DrawPic (cx, cy, "gfx/box_mm2");
			else
				M_DrawPic (cx, cy, "gfx/box_mm");
		}
		M_DrawPic (cx, cy+8, "gfx/box_bm");
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	M_DrawPic (cx, cy, "gfx/box_tr");
	for (n = 0; n < height; n++)
	{
		cy += 8;
		M_DrawPic (cx, cy, "gfx/box_mr");
	}
	M_DrawPic (cx, cy+8, "gfx/box_br");
}

//=============================================================================

//int m_save_demonum;

/*
================
M_ToggleMenu
================
*/
static void M_ToggleMenu(int mode)
{
	m_entersound = true;

	if ((key_dest != key_menu && key_dest != key_menu_grabbed) || m_state != m_main)
	{
		if(mode == 0) return; // the menu is off, and we want it off
		M_Menu_Main_f ();
	}
	else
	{
		if(mode == 1) return; // the menu is on, and we want it on
		key_dest = key_game;
		m_state = m_none;
	}
}


//=============================================================================
/* MAIN MENU */

static int	m_main_cursor;
static qboolean m_missingdata = false;

static int MAIN_ITEMS = 4; // Nehahra: Menu Disable


void M_Menu_Main_f (void)
{
	const char *s;
	s = "gfx/mainmenu";
	MAIN_ITEMS = 5;

	// check if the game data is missing and use a different main menu if so
	m_missingdata = !forceqmenu.integer && !Draw_IsPicLoaded(Draw_CachePic_Flags(s, CACHEPICFLAG_FAILONMISSING));
	if (m_missingdata) MAIN_ITEMS = 2;

	key_dest = key_menu;
	m_state = m_main;
	m_entersound = true;
}


static void M_Main_Draw (void)
{
	int		f;
	cachepic_t	*p;
	char vabuf[1024];

	if (m_missingdata)
	{
		float y;
		const char *s;
		M_Background2(640, 480); //fall back is always to 640x480, this makes it most readable at that.
		y = 480/3-16;
		s = "You have reached this menu due to missing or unlocatable content/data";M_PrintRed ((640-strlen(s)*8)*0.5, (480/3)-16, s);y+=8;
		y+=8;
		s = "You may consider adding";M_Print ((640-strlen(s)*8)*0.5, y, s);y+=8;
		s = "-basedir /path/to/game";M_Print ((640-strlen(s)*8)*0.5, y, s);y+=8;
		s = "to your launch commandline";M_Print ((640-strlen(s)*8)*0.5, y, s);y+=8;
		M_Print (640/2 - 48, 480/2, "Open Console"); //The console usually better shows errors (failures)
		M_Print (640/2 - 48, 480/2 + 8, "Quit");
		M_DrawCharacter(640/2 - 56, 480/2 + (8 * m_main_cursor), 12+((int)(realtime*4)&1));
		return;
	}

	M_Background(320, 200);
	CursorMouseMove(&m_main_cursor, 5);
	M_DrawPic (72, 32, "gfx/mainmenu");
	f = (int)(realtime * 10)%6;
	M_DrawPic (54, 32 + m_main_cursor * 20, va(vabuf, sizeof(vabuf), "gfx/menudot%i", f+1));
	//DrawQ_String(0, vid_conheight.integer -100, "TEST", 8, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);

	//DrawQ_Pic(menu_x + cx, menu_y + cy, Draw_CachePic(picname), 0, 0, 1, 1, 1, 1, 0);

}

static void M_Main_Key (int key, int ascii)
{
	switch (key)
	{
	case K_MOUSE1:
		GetMyCursorPos(); 

		if (CursorX > 130 && CursorX < 540)
		{
			if (CursorY > 470 && CursorY <= 540)
			{
				m_entersound = true;
				M_Menu_SinglePlayer_f();
			}
			else if (CursorY > 540 && CursorY <= 602)
			{
				m_entersound = true;
				M_Menu_MultiPlayer_f();
			}
			else if (CursorY > 602 && CursorY <= 666)
			{
				m_entersound = true;
				M_Menu_Options_f();
			}
			else if (CursorY > 666 && CursorY <= 732)
			{
				m_entersound = true;
				M_Menu_Help_f();
			}
			else if (CursorY > 732 && CursorY <= 800)
			{
				m_entersound = true;
				Host_Quit_f();
			}
		}
		break;

	case K_ESCAPE:
		key_dest = key_game;
		m_state = m_none;
		//cls.demonum = m_save_demonum;
		//if (cls.demonum != -1 && !cls.demoplayback && cls.state != ca_connected)
		//	CL_NextDemo ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		if (m_missingdata)
		{
			switch (m_main_cursor)
			{
			case 0:
				if (cls.state == ca_connected)
				{
					m_state = m_none;
					key_dest = key_game;
				}
				Con_ToggleConsole_f ();
				break;
			case 1:
				Host_Quit_f();
				break;
			}
		}
		else
		{
			switch (m_main_cursor)
			{
			case 0:
				M_Menu_SinglePlayer_f ();
				break;

			case 1:
				M_Menu_MultiPlayer_f ();
				break;

			case 2:
				M_Menu_Options_f ();
				break;

			case 3:
				M_Menu_Help_f ();
				break;

			case 4:
				Host_Quit_f();
				break;
			}
		}
	}
}

//=============================================================================
/* SINGLE PLAYER MENU */

static int	m_singleplayer_cursor;
#define	SINGLEPLAYER_ITEMS	3


void M_Menu_SinglePlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_singleplayer;
	m_entersound = true;
}


static void M_SinglePlayer_Draw (void)
{
	cachepic_t	*p;
	char vabuf[1024];

	M_Background(320, 200);
	{
		CursorMouseMove(&m_singleplayer_cursor, 3);
		M_DrawPic (72, 32, "gfx/sp_menu");
		int f = (int)(realtime * 10)%6;
		M_DrawPic (54, 32 + m_singleplayer_cursor * 20, va(vabuf, sizeof(vabuf), "gfx/menudot%i", f+1));
	}
}

static void M_SinglePlayer_Key (int key, int ascii)
{
	switch (key)
	{
	case K_MOUSE1:
		GetMyCursorPos();

		if (CursorX > 130 && CursorX < 540)
		{
			if (CursorY > 470 && CursorY <= 540)
			{
				m_entersound = true;
				key_dest = key_game;
				if (sv.active) Cbuf_AddText("disconnect\n");
				Cbuf_AddText("maxplayers 1\n");
				Cbuf_AddText("deathmatch 0\n");
				Cbuf_AddText("coop 0\n");
				Cbuf_AddText("startmap_sp\n");
			}
			if (CursorY > 540 && CursorY <= 602)
			{
				m_entersound = true;
				M_Menu_Load_f();
			}
			else if (CursorY > 602 && CursorY <= 666)
			{
				m_entersound = true;
				M_Menu_Save_f();
			}
		}
		break;
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		if (++m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
			m_singleplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		if (--m_singleplayer_cursor < 0)
			m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_singleplayer_cursor)
		{
		case 0:
			key_dest = key_game;
			if (sv.active) Cbuf_AddText ("disconnect\n");
			Cbuf_AddText ("maxplayers 1\n");
			Cbuf_AddText ("deathmatch 0\n");
			Cbuf_AddText ("coop 0\n");
			Cbuf_AddText ("startmap_sp\n");
			break;

		case 1:
			M_Menu_Load_f ();
			break;

		case 2:
			M_Menu_Save_f ();
			break;
		}
	}
}


//=============================================================================
/* CLASS SELECT */
static int CLASS_ITEMS = 3;
static int class_cursor = 0;
static int Team_selected = 0;

static void M_Class_Draw(void)
{
	const char* s = "SELECT CLASS";
	DrawQ_Fill(vid_conwidth.integer*0.4, vid_conheight.integer*0.2, vid_conwidth.integer*0.2, vid_conwidth.integer*0.048, 0, 0, 0, 0.5, 0);
	DrawQ_String(vid_conwidth.integer * 0.5 - strlen(s) * 4, vid_conheight.integer * 0.22, s, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
	

	{
		if (CursorX > 400 && CursorX < 600 && CursorY > 285 && CursorY <= 335) DrawQ_Fill(vid_conwidth.integer * 0.4, vid_conheight.integer * 0.285, vid_conwidth.integer * 0.2, vid_conheight.integer * 0.05, 0.6, 0.53, 0.4, 0.5, 0);
		else DrawQ_Fill(vid_conwidth.integer * 0.4, vid_conheight.integer * 0.285, vid_conwidth.integer * 0.2, vid_conheight.integer * 0.05, 0.0, 0.0, 0.0, 0.5, 0);
		if (CursorX > 400 && CursorX < 600 && CursorY > 335 && CursorY <= 385) DrawQ_Fill(vid_conwidth.integer * 0.4, vid_conheight.integer * 0.335, vid_conwidth.integer * 0.2, vid_conheight.integer * 0.05, 0.6, 0.53, 0.4, 0.5, 0);
		else DrawQ_Fill(vid_conwidth.integer * 0.4, vid_conheight.integer * 0.335, vid_conwidth.integer * 0.2, vid_conheight.integer * 0.05, 0.0, 0.0, 0.0, 0.5, 0);
		if (CursorX > 400 && CursorX < 600 && CursorY > 385 && CursorY <= 435) DrawQ_Fill(vid_conwidth.integer * 0.4, vid_conheight.integer * 0.385, vid_conwidth.integer * 0.2, vid_conheight.integer * 0.05, 0.6, 0.53, 0.4, 0.5, 0);
		else DrawQ_Fill(vid_conwidth.integer * 0.4, vid_conheight.integer * 0.385, vid_conwidth.integer * 0.2, vid_conheight.integer * 0.05, 0.0, 0.0, 0.0, 0.5, 0);
		if (CursorX > 400 && CursorX < 600 && CursorY > 435 && CursorY <= 485) DrawQ_Fill(vid_conwidth.integer * 0.4, vid_conheight.integer * 0.435, vid_conwidth.integer * 0.2, vid_conheight.integer * 0.05, 0.6, 0.53, 0.4, 0.5, 0);
		else DrawQ_Fill(vid_conwidth.integer * 0.4, vid_conheight.integer * 0.435, vid_conwidth.integer * 0.2, vid_conheight.integer * 0.05, 0.0, 0.0, 0.0, 0.5, 0);
		if (CursorX > 400 && CursorX < 600 && CursorY > 485 && CursorY <= 535) DrawQ_Fill(vid_conwidth.integer * 0.4, vid_conheight.integer * 0.485, vid_conwidth.integer * 0.2, vid_conheight.integer * 0.05, 0.6, 0.53, 0.4, 0.5, 0);
		else DrawQ_Fill(vid_conwidth.integer * 0.4, vid_conheight.integer * 0.485, vid_conwidth.integer * 0.2, vid_conheight.integer * 0.05, 0.0, 0.0, 0.0, 0.5, 0);
	}

	s = "RIFLEMAN";
	DrawQ_String(vid_conwidth.integer * 0.5 - strlen(s) * 4, vid_conheight.integer * 0.3, s, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);

	s = "BREACHER";
	DrawQ_String(vid_conwidth.integer * 0.5 - strlen(s) * 4, vid_conheight.integer * 0.35, s, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);

	s = "MARKSMAN";
	DrawQ_String(vid_conwidth.integer * 0.5 - strlen(s) * 4, vid_conheight.integer * 0.4, s, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);

	s = "ENGINEER";
	DrawQ_String(vid_conwidth.integer * 0.5 - strlen(s) * 4, vid_conheight.integer * 0.45, s, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);

	s = "ASSASSIN";
	DrawQ_String(vid_conwidth.integer * 0.5 - strlen(s) * 4, vid_conheight.integer * 0.5, s, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
	
	if (CursorX > 250 && CursorX < 750 && CursorY > 600 && CursorY <= 675) DrawQ_Fill(vid_conwidth.integer *0.4, vid_conheight.integer * 0.6, vid_conwidth.integer * 0.2, 24, 0.6, 0.53, 0.4, 0.5, 0);
	else DrawQ_Fill(vid_conwidth.integer * 0.4, vid_conheight.integer * 0.6, vid_conwidth.integer * 0.2, 24, 0, 0, 0, 0.5, 0);
	s = "CANCEL";
	DrawQ_String((vid_conwidth.integer / 2) - strlen(s) * 4, vid_conheight.integer * 0.6+8, s, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
}


static void M_Class_Key(int k, int ascii)
{
	char vabuf[1024];
	switch (k)
	{
	case K_MOUSE1:
		GetMyCursorPos();
		if (CursorX > 400 && CursorX < 600)
		{
			if (CursorY > 285 && CursorY <= 335) 
			{
				if(Team_selected == 0) Cbuf_AddText("impulse 20\n");
				else Cbuf_AddText("impulse 25\n");
				key_dest = key_game;
				m_state = m_none;
			}
			if (CursorY > 335 && CursorY <= 385)
			{
				if (Team_selected == 0) Cbuf_AddText("impulse 21\n");
				else Cbuf_AddText("impulse 26\n");
				key_dest = key_game;
				m_state = m_none;
			}

			if (CursorY > 385 && CursorY <= 435)
			{
				if (Team_selected == 0) Cbuf_AddText("impulse 22\n");
				else Cbuf_AddText("impulse 27\n");
				key_dest = key_game;
				m_state = m_none;
			}
			if (CursorY > 435 && CursorY <= 485)
			{
				if (Team_selected == 0) Cbuf_AddText("impulse 23\n");
				else Cbuf_AddText("impulse 28\n");
				key_dest = key_game;
				m_state = m_none;
			}
			if (CursorY > 485 && CursorY <= 535)
			{
				if (Team_selected == 0) Cbuf_AddText("impulse 24\n");
				else Cbuf_AddText("impulse 29\n");
				key_dest = key_game;
				m_state = m_none;
			} 
			else if (CursorY > 600 && CursorY <= 675)
			{
				key_dest = key_game;
				m_state = m_none;
			}
		}
		break;
	case 49:
		if (Team_selected == 0) Cbuf_AddText("impulse 20\n");
		else Cbuf_AddText("impulse 25\n");
		key_dest = key_game;
		m_state = m_none;
		break;
	case 50:
		if (Team_selected == 0) Cbuf_AddText("impulse 21\n");
		else Cbuf_AddText("impulse 26\n");
		key_dest = key_game;
		m_state = m_none;
		break;
	case 51:
		if (Team_selected == 0) Cbuf_AddText("impulse 22\n");
		else Cbuf_AddText("impulse 27\n");
		key_dest = key_game;
		m_state = m_none;
		break;
	case 52:
		if (Team_selected == 0) Cbuf_AddText("impulse 23\n");
		else Cbuf_AddText("impulse 28\n");
		key_dest = key_game;
		m_state = m_none;
		break;
	case 53:
		if (Team_selected == 0) Cbuf_AddText("impulse 24\n");
		else Cbuf_AddText("impulse 29\n");
		key_dest = key_game;
		m_state = m_none;
		break;
	case 109:
	case K_ESCAPE:
		key_dest = key_game;
		m_state = m_none;
		break;
	}
}


//=============================================================================
/* TEAM SELECT */
static int TEAM_ITEMS = 3;
static int team_cursor = 0;
void M_Menu_Team_f(void)
{
	if (cl.gametype == GAME_DEATHMATCH)
	{
		m_entersound = true;
		m_state = m_team;
		key_dest = key_menu;
	}
}

static void M_Team_Draw(void)
{
	const char* s = "SELECT TEAM";
	DrawQ_Fill(vid_conwidth.integer/4, vid_conheight.integer / 4, vid_conwidth.integer / 2, vid_conheight.integer / 2-8, 0, 0, 0, 0.5, 0);
	DrawQ_String((vid_conwidth.integer / 2) - strlen(s) * 4, vid_conheight.integer / 4 + 1, s, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
	DrawQ_Pic(vid_conwidth.integer / 4, vid_conwidth.integer / 4 /2 + 20, Draw_CachePic("gfx/ussr"), vid_conwidth.integer / 4, vid_conwidth.integer / 4, 1, 1, 1, 1, 0);
	DrawQ_Pic(vid_conwidth.integer / 2, vid_conwidth.integer / 4 / 2 + 20, Draw_CachePic("gfx/usa"), vid_conwidth.integer / 4, vid_conwidth.integer / 4, 1, 1, 1, 1, 0);
	s = "CANCEL";
	if (CursorX > 250 && CursorX < 750 && CursorY > 726 && CursorY <= 800) DrawQ_Fill(vid_conwidth.integer / 4, vid_conheight.integer * 0.75 - 8, vid_conwidth.integer / 2, 24, 0.6, 0.53, 0.4, 0.5, 0);
	else DrawQ_Fill(vid_conwidth.integer / 4, vid_conheight.integer * 0.75 - 8, vid_conwidth.integer / 2, 24, 0, 0, 0, 0.5, 0);
	DrawQ_String((vid_conwidth.integer / 2) - strlen(s) * 4, vid_conheight.integer *0.75, s, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
}


static void M_Team_Key(int k, int ascii)
{
	char vabuf[1024];
	switch (k)
	{
	case 49:
		m_entersound = true;
		Team_selected = 0;
		m_state = m_class;
		break;
	case 50:
		m_entersound = true;
		Team_selected = 1;
		m_state = m_class;
		break;
	case K_MOUSE1:
		GetMyCursorPos();
		if (CursorX > 250 && CursorX < 750)
		{
			if (CursorY > 280 && CursorY <= 726)
			{
				m_entersound = true;
				if(CursorX < 500) Team_selected = 0;
				else Team_selected = 1;
				m_state = m_class;
			}
			else if (CursorY > 726 && CursorY <= 800)
			{
				key_dest = key_game;
				m_state = m_none;
			}
		}
		break;
	case 109:
	case K_ESCAPE:
		key_dest = key_game;
		m_state = m_none;
		break;
	//default:
	//	if (k != 0)
	//	{
	//		char str[50];
	//		sprintf(str, "%d", k);
	//		MessageBoxA(NULL, str, NULL, NULL);
	//		//DrawQ_String(0, 0, str, 50, 20, 20, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);

	//	}
	//	break;
	}
}


//=============================================================================
/* LOAD/SAVE MENU */

static int		load_cursor;		///< 0 < load_cursor < MAX_SAVEGAMES

static char	m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH+1];
static int	loadable[MAX_SAVEGAMES];

static void M_ScanSaves (void)
{
	int		i, j;
	size_t	len;
	char	name[MAX_OSPATH];
	char	buf[SAVEGAME_COMMENT_LENGTH + 256];
	const char *t;
	qfile_t	*f;
//	int		version;

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
	{
		strlcpy (m_filenames[i], "--- UNUSED SLOT ---", sizeof(m_filenames[i]));
		loadable[i] = false;
		dpsnprintf (name, sizeof(name), "s%i.sav", (int)i);
		f = FS_OpenRealFile (name, "rb", false);
		if (!f) continue;
		// read enough to get the comment
		len = FS_Read(f, buf, sizeof(buf) - 1);
		len = min(len, sizeof(buf)-1);
		buf[len] = 0;
		t = buf;
		// version
		COM_ParseToken_Simple(&t, false, false, true);
		//version = atoi(com_token);
		// description
		COM_ParseToken_Simple(&t, false, false, true);
		strlcpy (m_filenames[i], com_token, sizeof (m_filenames[i]));

	// change _ back to space
		for (j=0 ; j<SAVEGAME_COMMENT_LENGTH ; j++) if (m_filenames[i][j] == '_') m_filenames[i][j] = ' ';
		loadable[i] = true;
		FS_Close (f);
	}
}

void M_Menu_Load_f (void)
{
	m_entersound = true;
	m_state = m_load;
	key_dest = key_menu;
	M_ScanSaves ();
}


void M_Menu_Save_f (void)
{
	if (!sv.active)
		return;
#if 1
	// LordHavoc: allow saving multiplayer games
	if (cl.islocalgame && cl.intermission)
		return;
#else
	if (cl.intermission)
		return;
	if (!cl.islocalgame)
		return;
#endif
	m_entersound = true;
	m_state = m_save;
	key_dest = key_menu;
	M_ScanSaves ();
}


static void M_Load_Draw (void)
{
	int		i;
	cachepic_t	*p;

	M_Background2(320, 200);

	p = Draw_CachePic ("gfx/p_load");
	M_DrawPic ( (320-Draw_GetPicWidth(p))/2, 4, "gfx/p_load" );

	for (i=0 ; i< MAX_SAVEGAMES; i++) M_Print(84, 32 + 8*i, m_filenames[i]);

// line cursor
	CursorMouseMoveOptions(&load_cursor, 286, 586);
	DrawQ_Fill(menu_x, 92 + load_cursor * 8, 320, 8, (0.5 + 0.2 * sin(realtime * M_PI)), 0, 0, 0.5, 0);
	//M_DrawCharacter (8, 32 + load_cursor*8, 12+((int)(realtime*4)&1));
}


static void M_Save_Draw (void)
{
	int		i;
	cachepic_t	*p;

	M_Background2(320, 200);

	p = Draw_CachePic ("gfx/p_save");
	M_DrawPic ( (320-Draw_GetPicWidth(p))/2, 4, "gfx/p_save");

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
		M_Print(84, 32 + 8*i, m_filenames[i]);

	CursorMouseMoveOptions(&load_cursor, 286, 586);
	DrawQ_Fill(menu_x, 92 + load_cursor * 8, 320, 8, (0.5 + 0.2 * sin(realtime * M_PI)), 0, 0, 0.5, 0);
	//M_DrawCharacter (8, 32 + load_cursor*8, 12+((int)(realtime*4)&1));
}


static void M_Load_Key (int k, int ascii)
{
	char vabuf[1024];
	switch (k)
	{
	case K_ESCAPE:
			M_Menu_SinglePlayer_f ();
		break;

	case K_MOUSE1:
	case K_ENTER:
		S_LocalSound ("sound/menu/menu2.wav");
		if (!loadable[load_cursor])
			return;
		m_state = m_none;
		key_dest = key_game;

		// issue the load command
		Cbuf_AddText (va(vabuf, sizeof(vabuf), "load s%i\n", load_cursor) );
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}


static void M_Save_Key (int k, int ascii)
{
	char vabuf[1024];
	switch (k)
	{
	case K_ESCAPE:
			M_Menu_SinglePlayer_f ();
		break;

	case K_MOUSE1:
	case K_ENTER:
		m_state = m_none;
		key_dest = key_game;
		Cbuf_AddText (va(vabuf, sizeof(vabuf), "save s%i\n", load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}

//=============================================================================
/* MULTIPLAYER MENU */

static int	m_multiplayer_cursor;
#define	MULTIPLAYER_ITEMS 2


void M_Menu_MultiPlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_multiplayer;
	m_entersound = true;
}


static void M_MultiPlayer_Draw (void)
{
	int		f;
	cachepic_t	*p;
	char vabuf[1024];

	M_Background(320, 200);
	CursorMouseMove(&m_multiplayer_cursor, 2);
	M_DrawPic (72, 32, "gfx/mp_menu");
	f = (int)(realtime * 10)%6;
	M_DrawPic (54, 32 + m_multiplayer_cursor * 20, va(vabuf, sizeof(vabuf), "gfx/menudot%i", f+1));
}


static void M_MultiPlayer_Key (int key, int ascii)
{
	switch (key)
	{
	case K_MOUSE1:
		GetMyCursorPos();
		if (CursorX > 130 && CursorX < 540)
		{
			if (CursorY > 470 && CursorY <= 540)
			{
				m_multiplayer_cursor = 0;
				m_entersound = true;
				M_Menu_LanConfig_f();
			}
			if (CursorY > 540 && CursorY <= 602)
			{
				m_multiplayer_cursor = 1;
				m_entersound = true;
				M_Menu_LanConfig_f();
			}
		}
		break;
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		if (++m_multiplayer_cursor >= MULTIPLAYER_ITEMS) m_multiplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		if (--m_multiplayer_cursor < 0) m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;
		switch (m_multiplayer_cursor)
		{
		case 0:
		case 1:
			M_Menu_LanConfig_f ();
			break;
		}
	}
}

//=============================================================================
/* OPTIONS MENU */

#define	SLIDER_RANGE	10

static void M_DrawSlider (int x, int y, float num, float rangemin, float rangemax)
{
	char text[16];
	int i;
	float range;
	range = bound(0, (num - rangemin) / (rangemax - rangemin), 1);
	M_DrawCharacter (x-8, y, 128);
	for (i = 0;i < SLIDER_RANGE; i++) M_DrawCharacter (x + i*8, y, 129);
	M_DrawCharacter (x+i*8, y, 130);
	M_DrawCharacter (x + (SLIDER_RANGE-1) * 8 * range, y, 131);
	if (fabs((int)num - num) < 0.01) dpsnprintf(text, sizeof(text), "%i", (int)num);
	else dpsnprintf(text, sizeof(text), "%.3f", num);
	M_Print(x + (SLIDER_RANGE+2) * 8, y, text);
}

static void M_DrawCheckbox (int x, int y, int on)
{
	if (on) M_Print(x, y, "on");
	else M_Print(x, y, "off");
}


#define OPTIONS_ITEMS 23


static int options_cursor;

void M_Menu_Options_f (void)
{
	key_dest = key_menu;
	m_state = m_options;
	m_entersound = true;
}

extern cvar_t slowmo;
extern dllhandle_t jpeg_dll;
extern cvar_t gl_texture_anisotropy;
extern cvar_t r_textshadow;
extern cvar_t r_hdr_scenebrightness;

static void M_Menu_Options_AdjustSliders (int dir)
{
	int optnum;
	double f;
	S_LocalSound ("sound/misc/menu3.wav");

	optnum = 0;
	     if (options_cursor == optnum++) ;
	else if (options_cursor == optnum++) ;
	else if (options_cursor == optnum++) ;
	else if (options_cursor == optnum++) ;
	else if (options_cursor == optnum++) Cvar_SetValueQuick(&crosshair, bound(0, crosshair.integer + dir, 7));
	else if (options_cursor == optnum++) Cvar_SetValueQuick(&sensitivity, bound(1, sensitivity.value + dir * 0.5, 20));
	else if (options_cursor == optnum++) Cvar_SetValueQuick(&m_pitch, -m_pitch.value);
	else if (options_cursor == optnum++) Cvar_SetValueQuick(&showfps, !showfps.integer);
	else if (options_cursor == optnum++) ;
	else if (options_cursor == optnum++) Cvar_SetValueQuick(&r_hdr_scenebrightness, bound(1, r_hdr_scenebrightness.value + dir * 0.1, 4));
	else if (options_cursor == optnum++) Cvar_SetValueQuick(&v_contrast, bound(1, v_contrast.value + dir * 0.1, 2));
	else if (options_cursor == optnum++) Cvar_SetValueQuick(&v_gamma, bound(0.5, v_gamma.value + dir * 0.1, 3));
	else if (options_cursor == optnum++) Cvar_SetValueQuick(&volume, bound(0, volume.value + dir * 0.1, 1));
	else if (options_cursor == optnum++) Cvar_SetValueQuick(&bgmvolume, bound(0, bgmvolume.value + dir * 0.1, 1));
}

static int m_optnum;
static int m_opty;
static int m_optcursor;

static void M_Options_PrintCommand(const char *s, int enabled)
{
	if (m_opty >= 32)
	{
		if (m_optnum == m_optcursor)
			DrawQ_Fill(menu_x + 48, menu_y + m_opty, 320, 8, m_optnum == m_optcursor ? (0.5 + 0.2 * sin(realtime * M_PI)) : 0, 0, 0, 0.5, 0);
		M_ItemPrint(0 + 48, m_opty, s, enabled);
	}
	m_opty += 8;
	m_optnum++;
}

static void M_Options_PrintCheckbox(const char *s, int enabled, int yes)
{
	if (m_opty >= 32)
	{
		if (m_optnum == m_optcursor)
			DrawQ_Fill(menu_x + 48, menu_y + m_opty, 320, 8, m_optnum == m_optcursor ? (0.5 + 0.2 * sin(realtime * M_PI)) : 0, 0, 0, 0.5, 0);
		M_ItemPrint(0 + 48, m_opty, s, enabled);
		M_DrawCheckbox(0 + 48 + (int)strlen(s) * 8 + 8, m_opty, yes);
	}
	m_opty += 8;
	m_optnum++;
}

static void M_Options_PrintSlider(const char *s, int enabled, float value, float minvalue, float maxvalue)
{
	if (m_opty >= 32)
	{
		if (m_optnum == m_optcursor)
			DrawQ_Fill(menu_x + 48, menu_y + m_opty, 320, 8, m_optnum == m_optcursor ? (0.5 + 0.2 * sin(realtime * M_PI)) : 0, 0, 0, 0.5, 0);
		M_ItemPrint(0 + 48, m_opty, s, enabled);
		M_DrawSlider(0 + 48 + (int)strlen(s) * 8 + 8, m_opty, value, minvalue, maxvalue);
	}
	m_opty += 8;
	m_optnum++;
}

static void M_Options_Draw (void)
{
	int visible;
	cachepic_t	*p;

	M_Background2(320, 200);
	//M_DrawPic(16, 4, "gfx/qplaque");
	p = Draw_CachePic ("gfx/p_option");
	M_DrawPic((320-Draw_GetPicWidth(p))/2, 4, "gfx/p_option");

	m_optnum = 0;
	m_optcursor = options_cursor;
	//visible = (int)((menu_height - 32) / 8);
	//m_opty = 32 - bound(0, m_optcursor - (visible >> 1), max(0, OPTIONS_ITEMS - visible)) * 8;
	m_opty = 32;
	
	CursorMouseMoveOptions(&options_cursor, 286, 890);

	M_Options_PrintCommand( "    Customize controls", true);
	M_Options_PrintCommand( "         Go to console", true);
	M_Options_PrintCommand( "     Reset to defaults", true);
	M_Options_PrintCommand( "     Change Video Mode", true);
	M_Options_PrintSlider(  "             Crosshair", true, crosshair.value, 0, 7);
	M_Options_PrintSlider(  "           Mouse Speed", true, sensitivity.value, 1, 20);
	M_Options_PrintCheckbox("          Invert Mouse", true, m_pitch.value < 0);
	M_Options_PrintCheckbox("        Show Framerate", true, showfps.integer);
	//M_Options_PrintCheckbox("    Show Date and Time", true, showdate.integer && showtime.integer);
	M_Options_PrintCommand( "     Custom Brightness", true);
	M_Options_PrintSlider(  "       Game Brightness", true, r_hdr_scenebrightness.value, 1, 4);
	M_Options_PrintSlider(  "            Brightness", true, v_contrast.value, 1, 2);
	M_Options_PrintSlider(  "                 Gamma", true, v_gamma.value, 0.5, 3);
	M_Options_PrintSlider(  "          Sound Volume", snd_initialized.integer, volume.value, 0, 1);
	M_Options_PrintSlider(  "          Music Volume", cdaudioinitialized.integer, bgmvolume.value, 0, 1);
	M_Options_PrintCommand( "     Customize Effects", true);
	M_Options_PrintCommand( "       Effects:  Quake", true);
	M_Options_PrintCommand( "       Effects: Normal", true);
	M_Options_PrintCommand( "       Effects:   High", true);
	M_Options_PrintCommand( "    Customize Lighting", true);
	M_Options_PrintCommand( "      Lighting: Flares", true);
	M_Options_PrintCommand( "      Lighting: Normal", true);
	M_Options_PrintCommand( "      Lighting:   High", true);
	M_Options_PrintCommand( "      Lighting:   Full", true);


	char str[60] = "LEFT MOUSE BUTTON - INCREASE; RIGHT MOUSE BUTTON - DECREASE";
	DrawQ_String(vid_conwidth.integer * 0.5 - strlen(str) * 3, vid_conheight.integer * 0.9, str, 0, 6, 6, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);

}


static void M_Options_Key (int k, int ascii)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_MOUSE1:
	case K_ENTER:
		m_entersound = true;
		switch (options_cursor)
		{
		case 0:
			M_Menu_Keys_f ();
			break;
		case 1:
			m_state = m_none;
			key_dest = key_game;
			Con_ToggleConsole_f ();
			break;
		case 2:
			M_Menu_Reset_f ();
			break;
		case 3:
			M_Menu_Video_f ();
			break;
		case 8:
			M_Menu_Options_ColorControl_f ();
			break;
		case 14: // Customize Effects
			M_Menu_Options_Effects_f ();
			break;
		case 15: // Effects: Quake
			Cbuf_AddText("cl_particles 1;cl_particles_quake 1;cl_particles_quality 1;cl_particles_explosions_shell 0;r_explosionclip 1;cl_stainmaps 0;cl_stainmaps_clearonload 1;cl_decals 0;cl_particles_bulletimpacts 1;cl_particles_smoke 1;cl_particles_sparks 1;cl_particles_bubbles 1;cl_particles_blood 1;cl_particles_blood_alpha 1;cl_particles_blood_bloodhack 0;cl_beams_polygons 0;cl_beams_instantaimhack 0;cl_beams_quakepositionhack 1;cl_beams_lightatend 0;r_lerpmodels 1;r_lerpsprites 1;r_lerplightstyles 0;gl_polyblend 1;r_skyscroll1 1;r_skyscroll2 2;r_waterwarp 1;r_wateralpha 1;r_waterscroll 1\n");
			break;
		case 16: // Effects: Normal
			Cbuf_AddText("cl_particles 1;cl_particles_quake 0;cl_particles_quality 1;cl_particles_explosions_shell 0;r_explosionclip 1;cl_stainmaps 0;cl_stainmaps_clearonload 1;cl_decals 1;cl_particles_bulletimpacts 1;cl_particles_smoke 1;cl_particles_sparks 1;cl_particles_bubbles 1;cl_particles_blood 1;cl_particles_blood_alpha 1;cl_particles_blood_bloodhack 1;cl_beams_polygons 1;cl_beams_instantaimhack 0;cl_beams_quakepositionhack 1;cl_beams_lightatend 0;r_lerpmodels 1;r_lerpsprites 1;r_lerplightstyles 0;gl_polyblend 1;r_skyscroll1 1;r_skyscroll2 2;r_waterwarp 1;r_wateralpha 1;r_waterscroll 1\n");
			break;
		case 17: // Effects: High
			Cbuf_AddText("cl_particles 1;cl_particles_quake 0;cl_particles_quality 2;cl_particles_explosions_shell 0;r_explosionclip 1;cl_stainmaps 1;cl_stainmaps_clearonload 1;cl_decals 1;cl_particles_bulletimpacts 1;cl_particles_smoke 1;cl_particles_sparks 1;cl_particles_bubbles 1;cl_particles_blood 1;cl_particles_blood_alpha 1;cl_particles_blood_bloodhack 1;cl_beams_polygons 1;cl_beams_instantaimhack 0;cl_beams_quakepositionhack 1;cl_beams_lightatend 0;r_lerpmodels 1;r_lerpsprites 1;r_lerplightstyles 0;gl_polyblend 1;r_skyscroll1 1;r_skyscroll2 2;r_waterwarp 1;r_wateralpha 1;r_waterscroll 1\n");
			break;
		case 18:
			M_Menu_Options_Graphics_f ();
			break;
		case 19: // Lighting: Flares
			Cbuf_AddText("r_coronas 1;gl_flashblend 1;r_shadow_gloss 0;r_shadow_realtime_dlight 0;r_shadow_realtime_dlight_shadows 0;r_shadow_realtime_world 0;r_shadow_realtime_world_lightmaps 0;r_shadow_realtime_world_shadows 1;r_bloom 0");
			break;
		case 20: // Lighting: Normal
			Cbuf_AddText("r_coronas 1;gl_flashblend 0;r_shadow_gloss 1;r_shadow_realtime_dlight 1;r_shadow_realtime_dlight_shadows 0;r_shadow_realtime_world 0;r_shadow_realtime_world_lightmaps 0;r_shadow_realtime_world_shadows 1;r_bloom 0");
			break;
		case 21: // Lighting: High
			Cbuf_AddText("r_coronas 1;gl_flashblend 0;r_shadow_gloss 1;r_shadow_realtime_dlight 1;r_shadow_realtime_dlight_shadows 1;r_shadow_realtime_world 0;r_shadow_realtime_world_lightmaps 0;r_shadow_realtime_world_shadows 1;r_bloom 1");
			break;
		case 22: // Lighting: Full
			Cbuf_AddText("r_coronas 1;gl_flashblend 0;r_shadow_gloss 1;r_shadow_realtime_dlight 1;r_shadow_realtime_dlight_shadows 1;r_shadow_realtime_world 1;r_shadow_realtime_world_lightmaps 0;r_shadow_realtime_world_shadows 1;r_bloom 1");
			break;
		default:
			M_Menu_Options_AdjustSliders (1);
			break;
		}
		return;

	case K_UPARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		options_cursor--;
		if (options_cursor < 0)
			options_cursor = OPTIONS_ITEMS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		options_cursor++;
		if (options_cursor >= OPTIONS_ITEMS)
			options_cursor = 0;
		break;

	case K_MOUSE2:
	case K_LEFTARROW:
		M_Menu_Options_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_Menu_Options_AdjustSliders (1);
		break;
	}
}

#define	OPTIONS_EFFECTS_ITEMS	35

static int options_effects_cursor;

void M_Menu_Options_Effects_f (void)
{
	key_dest = key_menu;
	m_state = m_options_effects;
	m_entersound = true;
}


extern cvar_t cl_stainmaps;
extern cvar_t cl_stainmaps_clearonload;
extern cvar_t r_explosionclip;
extern cvar_t r_coronas;
extern cvar_t gl_flashblend;
extern cvar_t cl_beams_polygons;
extern cvar_t cl_beams_quakepositionhack;
extern cvar_t cl_beams_instantaimhack;
extern cvar_t cl_beams_lightatend;
extern cvar_t r_lightningbeam_thickness;
extern cvar_t r_lightningbeam_scroll;
extern cvar_t r_lightningbeam_repeatdistance;
extern cvar_t r_lightningbeam_color_red;
extern cvar_t r_lightningbeam_color_green;
extern cvar_t r_lightningbeam_color_blue;
extern cvar_t r_lightningbeam_qmbtexture;

static void M_Menu_Options_Effects_AdjustSliders (int dir)
{
	int optnum;
	S_LocalSound ("sound/misc/menu3.wav");

	optnum = 0;
	     if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles, !cl_particles.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_quake, !cl_particles_quake.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_quality, bound(1, cl_particles_quality.value + dir * 0.5, 4));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_explosions_shell, !cl_particles_explosions_shell.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_explosionclip, !r_explosionclip.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_stainmaps, !cl_stainmaps.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_stainmaps_clearonload, !cl_stainmaps_clearonload.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_decals, !cl_decals.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_bulletimpacts, !cl_particles_bulletimpacts.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_smoke, !cl_particles_smoke.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_sparks, !cl_particles_sparks.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_bubbles, !cl_particles_bubbles.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_blood, !cl_particles_blood.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_blood_alpha, bound(0.2, cl_particles_blood_alpha.value + dir * 0.1, 1));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_particles_blood_bloodhack, !cl_particles_blood_bloodhack.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_beams_polygons, !cl_beams_polygons.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_beams_instantaimhack, !cl_beams_instantaimhack.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_beams_quakepositionhack, !cl_beams_quakepositionhack.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&cl_beams_lightatend, !cl_beams_lightatend.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lightningbeam_thickness, bound(1, r_lightningbeam_thickness.integer + dir, 10));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lightningbeam_scroll, bound(0, r_lightningbeam_scroll.integer + dir, 10));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lightningbeam_repeatdistance, bound(64, r_lightningbeam_repeatdistance.integer + dir * 64, 1024));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lightningbeam_color_red, bound(0, r_lightningbeam_color_red.value + dir * 0.1, 1));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lightningbeam_color_green, bound(0, r_lightningbeam_color_green.value + dir * 0.1, 1));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lightningbeam_color_blue, bound(0, r_lightningbeam_color_blue.value + dir * 0.1, 1));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lightningbeam_qmbtexture, !r_lightningbeam_qmbtexture.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lerpmodels, !r_lerpmodels.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lerpsprites, !r_lerpsprites.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_lerplightstyles, !r_lerplightstyles.integer);
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&gl_polyblend, bound(0, gl_polyblend.value + dir * 0.1, 1));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_skyscroll1, bound(-8, r_skyscroll1.value + dir * 0.1, 8));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_skyscroll2, bound(-8, r_skyscroll2.value + dir * 0.1, 8));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_waterwarp, bound(0, r_waterwarp.value + dir * 0.1, 1));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_wateralpha, bound(0, r_wateralpha.value + dir * 0.1, 1));
	else if (options_effects_cursor == optnum++) Cvar_SetValueQuick (&r_waterscroll, bound(0, r_waterscroll.value + dir * 0.5, 10));
}

static void M_Options_Effects_Draw (void)
{
	int visible;
	cachepic_t	*p;

	M_Background2(320, bound(200, 32 + OPTIONS_EFFECTS_ITEMS * 8, vid_conheight.integer));

	//M_DrawPic(16, 4, "gfx/qplaque");
	p = Draw_CachePic ("gfx/p_option");
	M_DrawPic((320-Draw_GetPicWidth(p))/2, 4, "gfx/p_option");

	m_optcursor = options_effects_cursor;
	m_optnum = 0;
	visible = (int)((menu_height - 32) / 8);
	m_opty = 32 - bound(0, m_optcursor - (visible >> 1), max(0, OPTIONS_EFFECTS_ITEMS - visible)) * 8;
	CursorMouseMoveOptions(&options_effects_cursor, 112,990);

	M_Options_PrintCheckbox("             Particles", true, cl_particles.integer);
	M_Options_PrintCheckbox(" Quake-style Particles", true, cl_particles_quake.integer);
	M_Options_PrintSlider(  "     Particles Quality", true, cl_particles_quality.value, 1, 4);
	M_Options_PrintCheckbox("       Explosion Shell", true, cl_particles_explosions_shell.integer);
	M_Options_PrintCheckbox("  Explosion Shell Clip", true, r_explosionclip.integer);
	M_Options_PrintCheckbox("             Stainmaps", true, cl_stainmaps.integer);
	M_Options_PrintCheckbox("Onload Clear Stainmaps", true, cl_stainmaps_clearonload.integer);
	M_Options_PrintCheckbox("                Decals", true, cl_decals.integer);
	M_Options_PrintCheckbox("        Bullet Impacts", true, cl_particles_bulletimpacts.integer);
	M_Options_PrintCheckbox("                 Smoke", true, cl_particles_smoke.integer);
	M_Options_PrintCheckbox("                Sparks", true, cl_particles_sparks.integer);
	M_Options_PrintCheckbox("               Bubbles", true, cl_particles_bubbles.integer);
	M_Options_PrintCheckbox("                 Blood", true, cl_particles_blood.integer);
	M_Options_PrintSlider(  "         Blood Opacity", true, cl_particles_blood_alpha.value, 0.2, 1);
	M_Options_PrintCheckbox("Force New Blood Effect", true, cl_particles_blood_bloodhack.integer);
	M_Options_PrintCheckbox("     Polygon Lightning", true, cl_beams_polygons.integer);
	M_Options_PrintCheckbox("Smooth Sweep Lightning", true, cl_beams_instantaimhack.integer);
	M_Options_PrintCheckbox(" Waist-level Lightning", true, cl_beams_quakepositionhack.integer);
	M_Options_PrintCheckbox("   Lightning End Light", true, cl_beams_lightatend.integer);
	M_Options_PrintSlider(  "   Lightning Thickness", cl_beams_polygons.integer, r_lightningbeam_thickness.integer, 1, 10);
	M_Options_PrintSlider(  "      Lightning Scroll", cl_beams_polygons.integer, r_lightningbeam_scroll.integer, 0, 10);
	M_Options_PrintSlider(  " Lightning Repeat Dist", cl_beams_polygons.integer, r_lightningbeam_repeatdistance.integer, 64, 1024);
	M_Options_PrintSlider(  "   Lightning Color Red", cl_beams_polygons.integer, r_lightningbeam_color_red.value, 0, 1);
	M_Options_PrintSlider(  " Lightning Color Green", cl_beams_polygons.integer, r_lightningbeam_color_green.value, 0, 1);
	M_Options_PrintSlider(  "  Lightning Color Blue", cl_beams_polygons.integer, r_lightningbeam_color_blue.value, 0, 1);
	M_Options_PrintCheckbox(" Lightning QMB Texture", cl_beams_polygons.integer, r_lightningbeam_qmbtexture.integer);
	M_Options_PrintCheckbox("   Model Interpolation", true, r_lerpmodels.integer);
	M_Options_PrintCheckbox("  Sprite Interpolation", true, r_lerpsprites.integer);
	M_Options_PrintCheckbox(" Flicker Interpolation", true, r_lerplightstyles.integer);
	M_Options_PrintSlider(  "            View Blend", true, gl_polyblend.value, 0, 1);
	M_Options_PrintSlider(  "Upper Sky Scroll Speed", true, r_skyscroll1.value, -8, 8);
	M_Options_PrintSlider(  "Lower Sky Scroll Speed", true, r_skyscroll2.value, -8, 8);
	M_Options_PrintSlider(  "  Underwater View Warp", true, r_waterwarp.value, 0, 1);
	M_Options_PrintSlider(  " Water Alpha (opacity)", true, r_wateralpha.value, 0, 1);
	M_Options_PrintSlider(  "        Water Movement", true, r_waterscroll.value, 0, 10);
}


static void M_Options_Effects_Key (int k, int ascii)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Options_f ();
		break;

	case K_MOUSE1:
	case K_ENTER:
		M_Menu_Options_Effects_AdjustSliders (1);
		break;

	case K_UPARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		options_effects_cursor--;
		if (options_effects_cursor < 0)
			options_effects_cursor = OPTIONS_EFFECTS_ITEMS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		options_effects_cursor++;
		if (options_effects_cursor >= OPTIONS_EFFECTS_ITEMS)
			options_effects_cursor = 0;
		break;

	case K_MOUSE2:
	case K_LEFTARROW:
		M_Menu_Options_Effects_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_Menu_Options_Effects_AdjustSliders (1);
		break;
	}
}


#define	OPTIONS_GRAPHICS_ITEMS	20

static int options_graphics_cursor;

void M_Menu_Options_Graphics_f (void)
{
	key_dest = key_menu;
	m_state = m_options_graphics;
	m_entersound = true;
}

extern cvar_t r_shadow_gloss;
extern cvar_t r_shadow_realtime_dlight;
extern cvar_t r_shadow_realtime_dlight_shadows;
extern cvar_t r_shadow_realtime_world;
extern cvar_t r_shadow_realtime_world_lightmaps;
extern cvar_t r_shadow_realtime_world_shadows;
extern cvar_t r_bloom;
extern cvar_t r_bloom_colorscale;
extern cvar_t r_bloom_colorsubtract;
extern cvar_t r_bloom_colorexponent;
extern cvar_t r_bloom_blur;
extern cvar_t r_bloom_brighten;
extern cvar_t r_bloom_resolution;
extern cvar_t r_hdr_scenebrightness;
extern cvar_t r_hdr_glowintensity;
extern cvar_t gl_picmip;

static void M_Menu_Options_Graphics_AdjustSliders (int dir)
{
	int optnum;
	S_LocalSound ("sound/misc/menu3.wav");

	optnum = 0;

	     if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&r_coronas, bound(0, r_coronas.value + dir * 0.125, 4));
	else if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&gl_flashblend, !gl_flashblend.integer);
	else if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&r_shadow_gloss,							bound(0, r_shadow_gloss.integer + dir, 2));
	else if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&r_shadow_realtime_dlight,				!r_shadow_realtime_dlight.integer);
	else if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&r_shadow_realtime_dlight_shadows,		!r_shadow_realtime_dlight_shadows.integer);
	else if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&r_shadow_realtime_world,					!r_shadow_realtime_world.integer);
	else if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&r_shadow_realtime_world_lightmaps,		bound(0, r_shadow_realtime_world_lightmaps.value + dir * 0.1, 1));
	else if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&r_shadow_realtime_world_shadows,			!r_shadow_realtime_world_shadows.integer);
	else if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&r_bloom,                                 !r_bloom.integer);
	else if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&r_hdr_scenebrightness,                   bound(0.25, r_hdr_scenebrightness.value + dir * 0.125, 4));
	else if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&r_hdr_glowintensity,                     bound(0, r_hdr_glowintensity.value + dir * 0.25, 4));
	else if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&r_bloom_colorscale,                      bound(0.0625, r_bloom_colorscale.value + dir * 0.0625, 1));
	else if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&r_bloom_colorsubtract,                   bound(0, r_bloom_colorsubtract.value + dir * 0.0625, 1-0.0625));
	else if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&r_bloom_colorexponent,                   bound(1, r_bloom_colorexponent.value * (dir > 0 ? 2.0 : 0.5), 8));
	else if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&r_bloom_brighten,                        bound(1, r_bloom_brighten.value + dir * 0.0625, 4));
	else if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&r_bloom_blur,                            bound(1, r_bloom_blur.value + dir * 1, 16));
	else if (options_graphics_cursor == optnum++) Cvar_SetValueQuick (&r_bloom_resolution,                      bound(64, r_bloom_resolution.value + dir * 64, 2048));
	else if (options_graphics_cursor == optnum++) Cbuf_AddText ("r_restart\n");
}


static void M_Options_Graphics_Draw (void)
{
	int visible;
	cachepic_t	*p;

	M_Background2(320, bound(200, 32 + OPTIONS_GRAPHICS_ITEMS * 8, vid_conheight.integer));

	//M_DrawPic(16, 4, "gfx/qplaque");
	p = Draw_CachePic ("gfx/p_option");
	M_DrawPic((320-Draw_GetPicWidth(p))/2, 4, "gfx/p_option");

	m_optcursor = options_graphics_cursor;
	m_optnum = 0;
	visible = (int)((menu_height - 32) / 8);
	m_opty = 32 - bound(0, m_optcursor - (visible >> 1), max(0, OPTIONS_GRAPHICS_ITEMS - visible)) * 8;
	CursorMouseMoveOptions(&options_graphics_cursor, 290, 730);

	M_Options_PrintSlider(  "      Corona Intensity", true, r_coronas.value, 0, 4);
	M_Options_PrintCheckbox("      Use Only Coronas", true, gl_flashblend.integer);
	M_Options_PrintSlider(  "            Gloss Mode", true, r_shadow_gloss.integer, 0, 2);
	M_Options_PrintCheckbox("            RT DLights", !gl_flashblend.integer, r_shadow_realtime_dlight.integer);
	M_Options_PrintCheckbox("     RT DLight Shadows", !gl_flashblend.integer, r_shadow_realtime_dlight_shadows.integer);
	M_Options_PrintCheckbox("              RT World", true, r_shadow_realtime_world.integer);
	M_Options_PrintSlider(  "    RT World Lightmaps", true, r_shadow_realtime_world_lightmaps.value, 0, 1);
	M_Options_PrintCheckbox("       RT World Shadow", true, r_shadow_realtime_world_shadows.integer);
	M_Options_PrintCheckbox("          Bloom Effect", true, r_bloom.integer);
	M_Options_PrintSlider(  "      Scene Brightness", true, r_hdr_scenebrightness.value, 0.25, 4);
	M_Options_PrintSlider(  "       Glow Brightness", true, r_hdr_glowintensity.value, 0, 4);
	M_Options_PrintSlider(  "     Bloom Color Scale", r_bloom.integer, r_bloom_colorscale.value, 0.0625, 1);
	M_Options_PrintSlider(  "  Bloom Color Subtract", r_bloom.integer, r_bloom_colorsubtract.value, 0, 1-0.0625);
	M_Options_PrintSlider(  "  Bloom Color Exponent", r_bloom.integer, r_bloom_colorexponent.value, 1, 8);
	M_Options_PrintSlider(  "       Bloom Intensity", r_bloom.integer, r_bloom_brighten.value, 1, 4);
	M_Options_PrintSlider(  "            Bloom Blur", r_bloom.integer, r_bloom_blur.value, 1, 16);
	M_Options_PrintSlider(  "      Bloom Resolution", r_bloom.integer, r_bloom_resolution.value, 64, 2048);
	M_Options_PrintCommand( "      Restart Renderer", true);

	char str[60] = "LEFT MOUSE BUTTON - INCREASE; RIGHT MOUSE BUTTON - DECREASE";
	DrawQ_String(vid_conwidth.integer * 0.5 - strlen(str) * 3, vid_conheight.integer * 0.9, str, 0, 6, 6, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);

}


static void M_Options_Graphics_Key (int k, int ascii)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Options_f ();
		break;

	case K_MOUSE1:
	case K_ENTER:
		M_Menu_Options_Graphics_AdjustSliders (1);
		break;

	case K_UPARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		options_graphics_cursor--;
		if (options_graphics_cursor < 0)
			options_graphics_cursor = OPTIONS_GRAPHICS_ITEMS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		options_graphics_cursor++;
		if (options_graphics_cursor >= OPTIONS_GRAPHICS_ITEMS)
			options_graphics_cursor = 0;
		break;

	case K_MOUSE2:
	case K_LEFTARROW:
		M_Menu_Options_Graphics_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_Menu_Options_Graphics_AdjustSliders (1);
		break;
	}
}


#define	OPTIONS_COLORCONTROL_ITEMS	18

static int		options_colorcontrol_cursor;

// intensity value to match up to 50% dither to 'correct' quake
static cvar_t menu_options_colorcontrol_correctionvalue = {0, "menu_options_colorcontrol_correctionvalue", "0.5", "intensity value that matches up to white/black dither pattern, should be 0.5 for linear color"};

void M_Menu_Options_ColorControl_f (void)
{
	key_dest = key_menu;
	m_state = m_options_colorcontrol;
	m_entersound = true;
}


static void M_Menu_Options_ColorControl_AdjustSliders (int dir)
{
	int optnum;
	float f;
	S_LocalSound ("sound/misc/menu3.wav");

	optnum = 1;
	if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 0);
		Cvar_SetValueQuick (&v_gamma, bound(1, v_gamma.value + dir * 0.125, 5));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 0);
		Cvar_SetValueQuick (&v_contrast, bound(1, v_contrast.value + dir * 0.125, 5));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 0);
		Cvar_SetValueQuick (&v_brightness, bound(0, v_brightness.value + dir * 0.05, 0.8));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, !v_color_enable.integer);
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_black_r, bound(0, v_color_black_r.value + dir * 0.0125, 0.8));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_black_g, bound(0, v_color_black_g.value + dir * 0.0125, 0.8));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_black_b, bound(0, v_color_black_b.value + dir * 0.0125, 0.8));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		f = bound(0, (v_color_black_r.value + v_color_black_g.value + v_color_black_b.value) / 3 + dir * 0.0125, 0.8);
		Cvar_SetValueQuick (&v_color_black_r, f);
		Cvar_SetValueQuick (&v_color_black_g, f);
		Cvar_SetValueQuick (&v_color_black_b, f);
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_grey_r, bound(0, v_color_grey_r.value + dir * 0.0125, 0.95));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_grey_g, bound(0, v_color_grey_g.value + dir * 0.0125, 0.95));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_grey_b, bound(0, v_color_grey_b.value + dir * 0.0125, 0.95));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		f = bound(0, (v_color_grey_r.value + v_color_grey_g.value + v_color_grey_b.value) / 3 + dir * 0.0125, 0.95);
		Cvar_SetValueQuick (&v_color_grey_r, f);
		Cvar_SetValueQuick (&v_color_grey_g, f);
		Cvar_SetValueQuick (&v_color_grey_b, f);
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_white_r, bound(1, v_color_white_r.value + dir * 0.125, 5));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_white_g, bound(1, v_color_white_g.value + dir * 0.125, 5));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_white_b, bound(1, v_color_white_b.value + dir * 0.125, 5));
	}
	else if (options_colorcontrol_cursor == optnum++)
	{
		Cvar_SetValueQuick (&v_color_enable, 1);
		f = bound(1, (v_color_white_r.value + v_color_white_g.value + v_color_white_b.value) / 3 + dir * 0.125, 5);
		Cvar_SetValueQuick (&v_color_white_r, f);
		Cvar_SetValueQuick (&v_color_white_g, f);
		Cvar_SetValueQuick (&v_color_white_b, f);
	}
}

static void M_Options_ColorControl_Draw (void)
{
	int visible;
	float x, s, t, u, v;
	float c[3];
	cachepic_t	*p, *dither;

	dither = Draw_CachePic_Flags ("gfx/colorcontrol/ditherpattern", CACHEPICFLAG_NOCLAMP);

	M_Background2(320, 256);

	//M_DrawPic(16, 4, "gfx/qplaque");
	p = Draw_CachePic ("gfx/p_option");
	M_DrawPic((320-Draw_GetPicWidth(p))/2, 4, "gfx/p_option");

	m_optcursor = options_colorcontrol_cursor;
	m_optnum = 0;
	visible = (int)((menu_height - 32) / 8);
	m_opty = 32 - bound(0, m_optcursor - (visible >> 1), max(0, OPTIONS_COLORCONTROL_ITEMS - visible)) * 8;
	CursorMouseMoveOptions(&options_colorcontrol_cursor, 200, 622);

	M_Options_PrintCommand( "     Reset to defaults", true);
	M_Options_PrintSlider(  "                 Gamma", !v_color_enable.integer, v_gamma.value, 1, 5);
	M_Options_PrintSlider(  "              Contrast", !v_color_enable.integer, v_contrast.value, 1, 5);
	M_Options_PrintSlider(  "            Brightness", !v_color_enable.integer, v_brightness.value, 0, 0.8);
	M_Options_PrintCheckbox("  Color Level Controls", true, v_color_enable.integer);
	M_Options_PrintSlider(  "          Black: Red  ", v_color_enable.integer, v_color_black_r.value, 0, 0.8);
	M_Options_PrintSlider(  "          Black: Green", v_color_enable.integer, v_color_black_g.value, 0, 0.8);
	M_Options_PrintSlider(  "          Black: Blue ", v_color_enable.integer, v_color_black_b.value, 0, 0.8);
	M_Options_PrintSlider(  "          Black: Grey ", v_color_enable.integer, (v_color_black_r.value + v_color_black_g.value + v_color_black_b.value) / 3, 0, 0.8);
	M_Options_PrintSlider(  "           Grey: Red  ", v_color_enable.integer, v_color_grey_r.value, 0, 0.95);
	M_Options_PrintSlider(  "           Grey: Green", v_color_enable.integer, v_color_grey_g.value, 0, 0.95);
	M_Options_PrintSlider(  "           Grey: Blue ", v_color_enable.integer, v_color_grey_b.value, 0, 0.95);
	M_Options_PrintSlider(  "           Grey: Grey ", v_color_enable.integer, (v_color_grey_r.value + v_color_grey_g.value + v_color_grey_b.value) / 3, 0, 0.95);
	M_Options_PrintSlider(  "          White: Red  ", v_color_enable.integer, v_color_white_r.value, 1, 5);
	M_Options_PrintSlider(  "          White: Green", v_color_enable.integer, v_color_white_g.value, 1, 5);
	M_Options_PrintSlider(  "          White: Blue ", v_color_enable.integer, v_color_white_b.value, 1, 5);
	M_Options_PrintSlider(  "          White: Grey ", v_color_enable.integer, (v_color_white_r.value + v_color_white_g.value + v_color_white_b.value) / 3, 1, 5);

	m_opty += 4;
	DrawQ_Fill(menu_x, menu_y + m_opty, 320, 4 + 64 + 8 + 64 + 4, 0, 0, 0, 1, 0);m_opty += 4;
	s = (float) 312 / 2 * vid.width / vid_conwidth.integer;
	t = (float) 4 / 2 * vid.height / vid_conheight.integer;
	DrawQ_SuperPic(menu_x + 4, menu_y + m_opty, dither, 312, 4, 0,0, 1,0,0,1, s,0, 1,0,0,1, 0,t, 1,0,0,1, s,t, 1,0,0,1, 0);m_opty += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + m_opty, NULL  , 312, 4, 0,0, 0,0,0,1, 1,0, 1,0,0,1, 0,1, 0,0,0,1, 1,1, 1,0,0,1, 0);m_opty += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + m_opty, dither, 312, 4, 0,0, 0,1,0,1, s,0, 0,1,0,1, 0,t, 0,1,0,1, s,t, 0,1,0,1, 0);m_opty += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + m_opty, NULL  , 312, 4, 0,0, 0,0,0,1, 1,0, 0,1,0,1, 0,1, 0,0,0,1, 1,1, 0,1,0,1, 0);m_opty += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + m_opty, dither, 312, 4, 0,0, 0,0,1,1, s,0, 0,0,1,1, 0,t, 0,0,1,1, s,t, 0,0,1,1, 0);m_opty += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + m_opty, NULL  , 312, 4, 0,0, 0,0,0,1, 1,0, 0,0,1,1, 0,1, 0,0,0,1, 1,1, 0,0,1,1, 0);m_opty += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + m_opty, dither, 312, 4, 0,0, 1,1,1,1, s,0, 1,1,1,1, 0,t, 1,1,1,1, s,t, 1,1,1,1, 0);m_opty += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + m_opty, NULL  , 312, 4, 0,0, 0,0,0,1, 1,0, 1,1,1,1, 0,1, 0,0,0,1, 1,1, 1,1,1,1, 0);m_opty += 4;

	c[0] = menu_options_colorcontrol_correctionvalue.value; // intensity value that should be matched up to a 50% dither to 'correct' quake
	c[1] = c[0];
	c[2] = c[1];
	VID_ApplyGammaToColor(c, c);
	s = (float) 48 / 2 * vid.width / vid_conwidth.integer;
	t = (float) 48 / 2 * vid.height / vid_conheight.integer;
	u = s * 0.5;
	v = t * 0.5;
	m_opty += 8;
	x = 4;
	DrawQ_Fill(menu_x + x, menu_y + m_opty, 64, 48, c[0], 0, 0, 1, 0);
	DrawQ_SuperPic(menu_x + x + 16, menu_y + m_opty + 16, dither, 16, 16, 0,0, 1,0,0,1, s,0, 1,0,0,1, 0,t, 1,0,0,1, s,t, 1,0,0,1, 0);
	DrawQ_SuperPic(menu_x + x + 32, menu_y + m_opty + 16, dither, 16, 16, 0,0, 1,0,0,1, u,0, 1,0,0,1, 0,v, 1,0,0,1, u,v, 1,0,0,1, 0);
	x += 80;
	DrawQ_Fill(menu_x + x, menu_y + m_opty, 64, 48, 0, c[1], 0, 1, 0);
	DrawQ_SuperPic(menu_x + x + 16, menu_y + m_opty + 16, dither, 16, 16, 0,0, 0,1,0,1, s,0, 0,1,0,1, 0,t, 0,1,0,1, s,t, 0,1,0,1, 0);
	DrawQ_SuperPic(menu_x + x + 32, menu_y + m_opty + 16, dither, 16, 16, 0,0, 0,1,0,1, u,0, 0,1,0,1, 0,v, 0,1,0,1, u,v, 0,1,0,1, 0);
	x += 80;
	DrawQ_Fill(menu_x + x, menu_y + m_opty, 64, 48, 0, 0, c[2], 1, 0);
	DrawQ_SuperPic(menu_x + x + 16, menu_y + m_opty + 16, dither, 16, 16, 0,0, 0,0,1,1, s,0, 0,0,1,1, 0,t, 0,0,1,1, s,t, 0,0,1,1, 0);
	DrawQ_SuperPic(menu_x + x + 32, menu_y + m_opty + 16, dither, 16, 16, 0,0, 0,0,1,1, u,0, 0,0,1,1, 0,v, 0,0,1,1, u,v, 0,0,1,1, 0);
	x += 80;
	DrawQ_Fill(menu_x + x, menu_y + m_opty, 64, 48, c[0], c[1], c[2], 1, 0);
	DrawQ_SuperPic(menu_x + x + 16, menu_y + m_opty + 16, dither, 16, 16, 0,0, 1,1,1,1, s,0, 1,1,1,1, 0,t, 1,1,1,1, s,t, 1,1,1,1, 0);
	DrawQ_SuperPic(menu_x + x + 32, menu_y + m_opty + 16, dither, 16, 16, 0,0, 1,1,1,1, u,0, 1,1,1,1, 0,v, 1,1,1,1, u,v, 1,1,1,1, 0);

	char str[60] = "LEFT MOUSE BUTTON - INCREASE; RIGHT MOUSE BUTTON - DECREASE";
	DrawQ_String(vid_conwidth.integer * 0.5 - strlen(str) * 3, vid_conheight.integer * 0.96, str, 0, 6, 6, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);

}


static void M_Options_ColorControl_Key (int k, int ascii)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Options_f ();
		break;

	case K_MOUSE1:
	case K_ENTER:
		m_entersound = true;
		switch (options_colorcontrol_cursor)
		{
		case 0:
			Cvar_SetValueQuick(&v_gamma, 1);
			Cvar_SetValueQuick(&v_contrast, 1);
			Cvar_SetValueQuick(&v_brightness, 0);
			Cvar_SetValueQuick(&v_color_enable, 0);
			Cvar_SetValueQuick(&v_color_black_r, 0);
			Cvar_SetValueQuick(&v_color_black_g, 0);
			Cvar_SetValueQuick(&v_color_black_b, 0);
			Cvar_SetValueQuick(&v_color_grey_r, 0);
			Cvar_SetValueQuick(&v_color_grey_g, 0);
			Cvar_SetValueQuick(&v_color_grey_b, 0);
			Cvar_SetValueQuick(&v_color_white_r, 1);
			Cvar_SetValueQuick(&v_color_white_g, 1);
			Cvar_SetValueQuick(&v_color_white_b, 1);
			break;
		default:
			M_Menu_Options_ColorControl_AdjustSliders (1);
			break;
		}
		return;

	case K_UPARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		options_colorcontrol_cursor--;
		if (options_colorcontrol_cursor < 0)
			options_colorcontrol_cursor = OPTIONS_COLORCONTROL_ITEMS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		options_colorcontrol_cursor++;
		if (options_colorcontrol_cursor >= OPTIONS_COLORCONTROL_ITEMS)
			options_colorcontrol_cursor = 0;
		break;

	case K_MOUSE2:
	case K_LEFTARROW:
		M_Menu_Options_ColorControl_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_Menu_Options_ColorControl_AdjustSliders (1);
		break;
	}
}


//=============================================================================
/* KEYS MENU */

static const char *quakebindnames[][2] =
{
{"+attack", 		"attack"},
{"zoom", 			"scope"},
{"+use", 			"use/open/pick up"},
{"reload", 			"reload"},
{"impulse 10", 		"next weapon"},
{"impulse 12", 		"previous weapon"},
{"+jump", 			"jump / swim up"},
{"+duck", 		    "crouch"},
{"+forward", 		"run forward"},
{"+back", 			"backpedal"},
{"+moveleft", 		"step left"},
{"+moveright", 		"step right"},
{"+speed", 			"walk"},
{"impulse 1", 		"knife"},
{"impulse 2", 		"pistol"},
{"impulse 3", 		"shotgun"},
{"impulse 4", 		"rifle"},
{"impulse 5", 		"sniper"},
{"impulse 6", 		"grenade"},
{"menu_team", 		"multiplayer menu"},
{"impulse 14", 		"flashlight"},
{"impulse 22", 		"menu down"},
{"impulse 23", 		"menu select"},

};


static int numcommands;
static const char *(*bindnames)[2];

/*
typedef struct binditem_s
{
	char *command, *description;
	struct binditem_s *next;
}
binditem_t;

typedef struct bindcategory_s
{
	char *name;
	binditem_t *binds;
	struct bindcategory_s *next;
}
bindcategory_t;

static bindcategory_t *bindcategories = NULL;

static void M_ClearBinds (void)
{
	for (c = bindcategories;c;c = cnext)
	{
		cnext = c->next;
		for (b = c->binds;b;b = bnext)
		{
			bnext = b->next;
			Z_Free(b);
		}
		Z_Free(c);
	}
	bindcategories = NULL;
}

static void M_AddBindToCategory(bindcategory_t *c, char *command, char *description)
{
	for (b = &c->binds;*b;*b = &(*b)->next);
	*b = Z_Alloc(sizeof(binditem_t) + strlen(command) + 1 + strlen(description) + 1);
	*b->command = (char *)((*b) + 1);
	*b->description = *b->command + strlen(command) + 1;
	strlcpy(*b->command, command, strlen(command) + 1);
	strlcpy(*b->description, description, strlen(description) + 1);
}

static void M_AddBind (char *category, char *command, char *description)
{
	for (c = &bindcategories;*c;c = &(*c)->next)
	{
		if (!strcmp(category, (*c)->name))
		{
			M_AddBindToCategory(*c, command, description);
			return;
		}
	}
	*c = Z_Alloc(sizeof(bindcategory_t));
	M_AddBindToCategory(*c, command, description);
}

static void M_DefaultBinds (void)
{
	M_ClearBinds();
	M_AddBind("movement", "+jump", "jump / swim up");
	M_AddBind("movement", "+forward", "walk forward");
	M_AddBind("movement", "+back", "backpedal");
	M_AddBind("movement", "+left", "turn left");
	M_AddBind("movement", "+right", "turn right");
	M_AddBind("movement", "+speed", "run");
	M_AddBind("movement", "+moveleft", "step left");
	M_AddBind("movement", "+moveright", "step right");
	M_AddBind("movement", "+strafe", "sidestep");
	M_AddBind("movement", "+lookup", "look up");
	M_AddBind("movement", "+lookdown", "look down");
	M_AddBind("movement", "centerview", "center view");
	M_AddBind("movement", "+mlook", "mouse look");
	M_AddBind("movement", "+klook", "keyboard look");
	M_AddBind("movement", "+moveup", "swim up");
	M_AddBind("movement", "+movedown", "swim down");
	M_AddBind("weapons", "+attack", "attack");
	M_AddBind("weapons", "impulse 10", "next weapon");
	M_AddBind("weapons", "impulse 12", "previous weapon");
	M_AddBind("weapons", "impulse 1", "select weapon 1 (axe)");
	M_AddBind("weapons", "impulse 2", "select weapon 2 (shotgun)");
	M_AddBind("weapons", "impulse 3", "select weapon 3 (super )");
	M_AddBind("weapons", "impulse 4", "select weapon 4 (nailgun)");
	M_AddBind("weapons", "impulse 5", "select weapon 5 (super nailgun)");
	M_AddBind("weapons", "impulse 6", "select weapon 6 (grenade launcher)");
	M_AddBind("weapons", "impulse 7", "select weapon 7 (rocket launcher)");
	M_AddBind("weapons", "impulse 8", "select weapon 8 (lightning gun)");
}
*/


static int		keys_cursor;
static int		bind_grab;

void M_Menu_Keys_f (void)
{
	key_dest = key_menu_grabbed;
	m_state = m_keys;
	m_entersound = true;


	{
		numcommands = sizeof(quakebindnames) / sizeof(quakebindnames[0]);
		bindnames = quakebindnames;
	}

	// Make sure "keys_cursor" doesn't start on a section in the binding list
	keys_cursor = 0;
	while (bindnames[keys_cursor][0][0] == '\0')
	{
		keys_cursor++;

		// Only sections? There may be a problem somewhere...
		if (keys_cursor >= numcommands)
			Sys_Error ("M_Init: The key binding list only contains sections");
	}
}

#define NUMKEYS 5

static void M_UnbindCommand (const char *command)
{
	int		j;
	const char	*b;

	for (j = 0; j < (int)sizeof (keybindings[0]) / (int)sizeof (keybindings[0][0]); j++)
	{
		b = keybindings[0][j];
		if (!b)
			continue;
		if (!strcmp (b, command))
			Key_SetBinding (j, 0, "");
	}
}


static void M_Keys_Draw (void)
{
	int		i, j;
	int		keys[NUMKEYS];
	int		y;
	cachepic_t	*p;
	char	keystring[MAX_INPUTLINE];

	M_Background2(320, 48 + 8 * numcommands);

	p = Draw_CachePic ("gfx/ttl_cstm");
	M_DrawPic ( (320-Draw_GetPicWidth(p))/2, 4, "gfx/ttl_cstm");



	if (bind_grab) M_Print(12, 32, "Press a key or button for this action");
	else M_Print(18, 32, "Enter to change, backspace to clear");

// search for known bindings
	for (i = 0; i<numcommands ; i++)
	{
		y = 48 + 8*i;

		// If there's no command, it's just a section
		if (bindnames[i][0][0] == '\0')
		{
			M_PrintRed (4, y, "\x0D");  // #13 is the little arrow pointing to the right
			M_PrintRed (16, y, bindnames[i][1]);
			continue;
		}
		else M_Print(16, y, bindnames[i][1]);

		Key_FindKeysForCommand (bindnames[i][0], keys, NUMKEYS, 0);

		// LordHavoc: redesigned to print more than 2 keys, inspired by Tomaz's MiniRacer
		if (keys[0] == -1)
			strlcpy(keystring, "???", sizeof(keystring));
		else
		{
			char tinystr[2];
			keystring[0] = 0;
			for (j = 0;j < NUMKEYS;j++)
			{
				if (keys[j] != -1)
				{
					if (j > 0)
						strlcat(keystring, " or ", sizeof(keystring));
					strlcat(keystring, Key_KeynumToString (keys[j], tinystr, sizeof(tinystr)), sizeof(keystring));
				}
			}
		}
		M_Print(150, y, keystring);
	}

	CursorMouseMoveOptions(&keys_cursor, 286, 860);

	if (bind_grab)
		M_DrawCharacter (140, 48 + keys_cursor*8, '=');
	else
	{
		DrawQ_Fill(menu_x, 92 + keys_cursor * 8, 320, 8, (0.5 + 0.2 * sin(realtime * M_PI)), 0, 0, 0.5, 0);
		M_DrawCharacter(140, 48 + keys_cursor * 8, 12 + ((int)(realtime * 4) & 1));
	}
}


static void M_Keys_Key (int k, int ascii)
{
	char	cmd[80];
	int		keys[NUMKEYS];
	char	tinystr[2];

	if (bind_grab)
	{	// defining a key
		S_LocalSound ("sound/menu/menu1.wav");
		if (k == K_ESCAPE) bind_grab = false;
		else //if (k != '`')
		{
			dpsnprintf (cmd, sizeof(cmd), "bind \"%s\" \"%s\"\n", Key_KeynumToString (k, tinystr, sizeof(tinystr)), bindnames[keys_cursor][0]);
			Cbuf_InsertText (cmd);
		}

		bind_grab = false;
		return;
	}

	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Options_f ();
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		do
		{
			keys_cursor--;
			if (keys_cursor < 0)
				keys_cursor = numcommands-1;
		}
		while (bindnames[keys_cursor][0][0] == '\0');  // skip sections
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		do
		{
			keys_cursor++;
			if (keys_cursor >= numcommands)
				keys_cursor = 0;
		}
		while (bindnames[keys_cursor][0][0] == '\0');  // skip sections
		break;

	case K_MOUSE1:
	case K_ENTER:		// go into bind mode
		Key_FindKeysForCommand (bindnames[keys_cursor][0], keys, NUMKEYS, 0);
		S_LocalSound ("sound/menu/menu2.wav");
		if (keys[NUMKEYS - 1] != -1)
			M_UnbindCommand (bindnames[keys_cursor][0]);
		bind_grab = true;
		break;

	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
		S_LocalSound ("sound/menu/menu2.wav");
		M_UnbindCommand (bindnames[keys_cursor][0]);
		break;
	}
}

void M_Menu_Reset_f (void)
{
	key_dest = key_menu;
	m_state = m_reset;
	m_entersound = true;
}


static void M_Reset_Key (int key, int ascii)
{
	switch (key)
	{
	case 'Y':
	case 'y':
		Cbuf_AddText ("cvar_resettodefaults_all;exec default.cfg\n");
		// no break here since we also exit the menu

	case K_ESCAPE:
	case 'n':
	case 'N':
		m_state = m_options;
		m_entersound = true;
		break;

	default:
		break;
	}
}

static void M_Reset_Draw (void)
{
	int lines = 2, linelength = 20;
	M_Background2(linelength * 8 + 16, lines * 8 + 16);
	M_DrawTextBox(0, 0, linelength, lines);
	M_Print(8 + 4 * (linelength - 19),  8, "Really wanna reset?");
	M_Print(8 + 4 * (linelength - 11), 16, "Press y / n");
}

//=============================================================================
/* VIDEO MENU */

video_resolution_t video_resolutions_hardcoded[] =
{

{"WideScreen 16x9"           , 1280, 720, 640, 320, 1     },
{"WideScreen 16x9"           , 1320, 768, 640, 320, 1     },
//{"WideScreen 16x9"           , 1366, 768, 640, 320, 1     },
{"WideScreen 16x9"           , 1600,900, 640, 320, 1     },
{"WideScreen 16x9"           , 1920,1080, 640, 320, 1     },
{"WideScreen 16x9"           , 2560,1440, 640, 320, 1     },
{"WideScreen 16x9"           , 3840,2160, 640, 320, 1     },
{"WideScreen 16x9"           ,  640, 360, 640, 320, 1     },
//{"WideScreen 16x9"           ,  683, 384, 640, 320, 1     },
//{"WideScreen 16x9"           ,  960, 540, 640, 320, 1     },
//{"Standard 4x3"              ,  320, 240, 640, 320, 1     },
//{"Standard 4x3"              ,  400, 300, 640, 320, 1     },
{"Standard 4x3"              ,  640, 480, 640, 320, 1     },
{"Standard 4x3"              ,  800, 600, 640, 320, 1     },
{"Standard 4x3"              , 1024, 768, 640, 320, 1     },
//{"Standard 4x3"              , 1152, 864, 640, 320, 1     },
{"Standard 4x3"              , 1280, 960, 640, 320, 1     },
{"Standard 4x3"              , 1400,1050, 640, 320, 1     },
{"Standard 4x3"              , 1600,1200, 640, 320, 1     },
//{"Standard 4x3"              , 1792,1344, 640, 320, 1     },
//{"Standard 4x3"              , 1856,1392, 640, 320, 1     },
{"Standard 4x3"              , 1920,1440, 640, 320, 1     },
//{"WideScreen 14x9"           ,  840, 540, 640, 320, 1     },
//{"WideScreen 14x9"           , 1680,1080, 640, 320, 1     },
//{"Standard 4x3"              , 2048,1536, 640, 480, 1     },
//{"Short Pixel (CRT) 5x4"     ,  320, 256, 320, 256, 0.9375},
//{"Short Pixel (CRT) 5x4"     ,  640, 512, 640, 512, 0.9375},
//{"Short Pixel (CRT) 5x4"     , 1280,1024, 640, 512, 0.9375},
//{"Tall Pixel (CRT) 8x5"      ,  320, 200, 320, 200, 1.2   },
//{"Tall Pixel (CRT) 8x5"      ,  640, 400, 640, 400, 1.2   },
//{"Tall Pixel (CRT) 8x5"      ,  840, 525, 640, 400, 1.2   },
//{"Tall Pixel (CRT) 8x5"      ,  960, 600, 640, 400, 1.2   },
//{"Tall Pixel (CRT) 8x5"      , 1680,1050, 640, 400, 1.2   },
//{"Tall Pixel (CRT) 8x5"      , 1920,1200, 640, 400, 1.2   },
//{"Square Pixel (LCD) 5x4"    ,  320, 256, 320, 256, 1     },
//{"Square Pixel (LCD) 5x4"    ,  640, 512, 640, 512, 1     },
//{"Square Pixel (LCD) 5x4"    , 1280,1024, 640, 512, 1     },
//{"WideScreen 5x3"            ,  640, 384, 640, 384, 1     },
//{"WideScreen 5x3"            , 1280, 768, 640, 384, 1     },
//{"WideScreen 8x5"            ,  320, 200, 320, 200, 1     },
//{"WideScreen 8x5"            ,  640, 400, 640, 400, 1     },
//{"WideScreen 8x5"            ,  720, 450, 720, 450, 1     },
//{"WideScreen 8x5"            ,  840, 525, 640, 400, 1     },
//{"WideScreen 8x5"            ,  960, 600, 640, 400, 1     },
//{"WideScreen 8x5"            , 1280, 800, 640, 400, 1     },
//{"WideScreen 8x5"            , 1440, 900, 720, 450, 1     },
//{"WideScreen 8x5"            , 1680,1050, 640, 400, 1     },
//{"WideScreen 8x5"            , 1920,1200, 640, 400, 1     },
//{"WideScreen 8x5"            , 2560,1600, 640, 400, 1     },
//{"WideScreen 8x5"            , 3840,2400, 640, 400, 1     },


//{"NTSC 3x2"                  ,  360, 240, 360, 240, 1.125 },
//{"NTSC 3x2"                  ,  720, 480, 720, 480, 1.125 },
//{"PAL 14x11"                 ,  360, 283, 360, 283, 0.9545},
//{"PAL 14x11"                 ,  720, 566, 720, 566, 0.9545},
//{"NES 8x7"                   ,  256, 224, 256, 224, 1.1667},
//{"SNES 8x7"                  ,  512, 448, 512, 448, 1.1667},
{NULL, 0, 0, 0, 0, 0}
};
// this is the number of the default mode (640x480) in the list above
int video_resolutions_hardcoded_count = sizeof(video_resolutions_hardcoded) / sizeof(*video_resolutions_hardcoded) - 1;

#define VIDEO_ITEMS 10
static int video_cursor = 0;
static int video_cursor_table[VIDEO_ITEMS] = {68, 88, 96, 104, 112, 120, 128, 136, 144, 152};
static int menu_video_resolution;

video_resolution_t *video_resolutions;
int video_resolutions_count;

static video_resolution_t *menu_video_resolutions;
static int menu_video_resolutions_count;
static qboolean menu_video_resolutions_forfullscreen;

static void M_Menu_Video_FindResolution(int w, int h, float a)
{
	int i;

	if(menu_video_resolutions_forfullscreen)
	{
		menu_video_resolutions = video_resolutions;
		menu_video_resolutions_count = video_resolutions_count;
	}
	else
	{
		menu_video_resolutions = video_resolutions_hardcoded;
		menu_video_resolutions_count = video_resolutions_hardcoded_count;
	}

	// Look for the closest match to the current resolution
	menu_video_resolution = 0;
	for (i = 1;i < menu_video_resolutions_count;i++)
	{
		// if the new mode would be a worse match in width, skip it
		if (abs(menu_video_resolutions[i].width - w) > abs(menu_video_resolutions[menu_video_resolution].width - w))
			continue;
		// if it is equal in width, check height
		if (menu_video_resolutions[i].width == w && menu_video_resolutions[menu_video_resolution].width == w)
		{
			// if the new mode would be a worse match in height, skip it
			if (abs(menu_video_resolutions[i].height - h) > abs(menu_video_resolutions[menu_video_resolution].height - h))
				continue;
			// if it is equal in width and height, check pixel aspect
			if (menu_video_resolutions[i].height == h && menu_video_resolutions[menu_video_resolution].height == h)
			{
				// if the new mode would be a worse match in pixel aspect, skip it
				if (fabs(menu_video_resolutions[i].pixelheight - a) > fabs(menu_video_resolutions[menu_video_resolution].pixelheight - a))
					continue;
				// if it is equal in everything, skip it (prefer earlier modes)
				if (menu_video_resolutions[i].pixelheight == a && menu_video_resolutions[menu_video_resolution].pixelheight == a)
					continue;
				// better match for width, height, and pixel aspect
				menu_video_resolution = i;
			}
			else // better match for width and height
				menu_video_resolution = i;
		}
		else // better match for width
			menu_video_resolution = i;
	}
}

void M_Menu_Video_f (void)
{
	key_dest = key_menu;
	m_state = m_video;
	m_entersound = true;

	M_Menu_Video_FindResolution(vid.width, vid.height, vid_pixelheight.value);
}


static void M_Video_Draw (void)
{
	int t;
	cachepic_t	*p;
	char vabuf[1024];

	if(!!vid_fullscreen.integer != menu_video_resolutions_forfullscreen)
	{
		video_resolution_t *res = &menu_video_resolutions[menu_video_resolution];
		menu_video_resolutions_forfullscreen = !!vid_fullscreen.integer;
		M_Menu_Video_FindResolution(res->width, res->height, res->pixelheight);
	}

	M_Background2(320, 200);

	//M_DrawPic(16, 4, "gfx/qplaque");
	p = Draw_CachePic ("gfx/vidmodes");
	M_DrawPic((320-Draw_GetPicWidth(p))/2, 4, "gfx/vidmodes");

	t = 0;

	CursorMouseMoveDisplay(&video_cursor);
	//DrawQ_Fill(menu_x, 120 + video_cursor * 8, 320, 8, (0.5 + 0.2 * sin(realtime * M_PI)), 0, 0, 0.5, 0);


	// Current and Proposed Resolution
	M_Print(16, video_cursor_table[t] - 12, "    Current Resolution");
	if (vid_supportrefreshrate && vid.userefreshrate && vid.fullscreen)
		M_Print(220, video_cursor_table[t] - 12, va(vabuf, sizeof(vabuf), "%dx%d %.2fhz", vid.width, vid.height, vid.refreshrate));
	else
		M_Print(220, video_cursor_table[t] - 12, va(vabuf, sizeof(vabuf), "%dx%d", vid.width, vid.height));
	M_Print(16, video_cursor_table[t], "        New Resolution");
	M_Print(220, video_cursor_table[t], va(vabuf, sizeof(vabuf), "%dx%d", menu_video_resolutions[menu_video_resolution].width, menu_video_resolutions[menu_video_resolution].height));
	M_Print(96, video_cursor_table[t] + 8, va(vabuf, sizeof(vabuf), "Type: %s", menu_video_resolutions[menu_video_resolution].type));
	t++;

	// Antialiasing
	M_Print(16, video_cursor_table[t], "          Antialiasing");
	M_DrawSlider(220, video_cursor_table[t], vid_samples.value, 1, 32);
	t++;

	// Refresh Rate
	M_ItemPrint(16, video_cursor_table[t], "      Use Refresh Rate", vid_supportrefreshrate);
	M_DrawCheckbox(220, video_cursor_table[t], vid_userefreshrate.integer);
	t++;

	// Refresh Rate
	M_ItemPrint(16, video_cursor_table[t], "          Refresh Rate", vid_supportrefreshrate && vid_userefreshrate.integer);
	M_DrawSlider(220, video_cursor_table[t], vid_refreshrate.value, 50, 150);
	t++;

	// Fullscreen
	M_Print(16, video_cursor_table[t], "            Fullscreen");
	M_DrawCheckbox(220, video_cursor_table[t], vid_fullscreen.integer);
	t++;

	// Vertical Sync
	M_ItemPrint(16, video_cursor_table[t], "         Vertical Sync", true);
	M_DrawCheckbox(220, video_cursor_table[t], vid_vsync.integer);
	t++;

	M_ItemPrint(16, video_cursor_table[t], "    Anisotropic Filter", vid.support.ext_texture_filter_anisotropic);
	M_DrawSlider(220, video_cursor_table[t], gl_texture_anisotropy.integer, 1, vid.max_anisotropy);
	t++;

	M_ItemPrint(16, video_cursor_table[t], "       Texture Quality", true);
	M_DrawSlider(220, video_cursor_table[t], gl_picmip.value, 3, 0);
	t++;

	M_ItemPrint(16, video_cursor_table[t], "   Texture Compression", true);
	M_DrawCheckbox(220, video_cursor_table[t], gl_texturecompression.integer);
	t++;

	// "Apply" button
	M_Print(220, video_cursor_table[t], "Apply");
	t++;

	// Cursor
	M_DrawCharacter(200, video_cursor_table[video_cursor], 12+((int)(realtime*4)&1));

	char str[60] = "LEFT MOUSE BUTTON - INCREASE; RIGHT MOUSE BUTTON - DECREASE";
	DrawQ_String(vid_conwidth.integer * 0.5 - strlen(str) * 3, vid_conheight.integer * 0.9, str, 0, 6, 6, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);

}


static void M_Menu_Video_AdjustSliders (int dir)
{
	int t;

	S_LocalSound ("sound/misc/menu3.wav");

	t = 0;
	if (video_cursor == t++)
	{
		// Resolution
		int r;
		for(r = 0;r < menu_video_resolutions_count;r++)
		{
			menu_video_resolution += dir;
			if (menu_video_resolution >= menu_video_resolutions_count)
				menu_video_resolution = 0;
			if (menu_video_resolution < 0)
				menu_video_resolution = menu_video_resolutions_count - 1;
			if (menu_video_resolutions[menu_video_resolution].width >= vid_minwidth.integer && menu_video_resolutions[menu_video_resolution].height >= vid_minheight.integer)
				break;
		}
	}
	else if (video_cursor == t++)
		Cvar_SetValueQuick (&vid_samples, bound(1, vid_samples.value * (dir > 0 ? 2 : 0.5), 32));
	else if (video_cursor == t++)
		Cvar_SetValueQuick (&vid_userefreshrate, !vid_userefreshrate.integer);
	else if (video_cursor == t++)
		Cvar_SetValueQuick (&vid_refreshrate, bound(50, vid_refreshrate.value + dir, 150));
	else if (video_cursor == t++)
		Cvar_SetValueQuick (&vid_fullscreen, !vid_fullscreen.integer);
	else if (video_cursor == t++)
		Cvar_SetValueQuick (&vid_vsync, !vid_vsync.integer);
	else if (video_cursor == t++)
		Cvar_SetValueQuick (&gl_texture_anisotropy, bound(1, gl_texture_anisotropy.value * (dir < 0 ? 0.5 : 2.0), vid.max_anisotropy));
	else if (video_cursor == t++)
		Cvar_SetValueQuick (&gl_picmip, bound(0, gl_picmip.value - dir, 3));
	else if (video_cursor == t++)
		Cvar_SetValueQuick (&gl_texturecompression, !gl_texturecompression.integer);
}


static void M_Video_Key (int key, int ascii)
{
	switch (key)
	{
		case K_ESCAPE:
			// vid_shared.c has a copy of the current video config. We restore it
			Cvar_SetValueQuick(&vid_fullscreen, vid.fullscreen);
			Cvar_SetValueQuick(&vid_bitsperpixel, vid.bitsperpixel);
			Cvar_SetValueQuick(&vid_samples, vid.samples);
			if (vid_supportrefreshrate) Cvar_SetValueQuick(&vid_refreshrate, vid.refreshrate);
			Cvar_SetValueQuick(&vid_userefreshrate, vid.userefreshrate);

			S_LocalSound ("sound/menu/menu1.wav");
			M_Menu_Options_f ();
			break;

		case K_MOUSE1:
		case K_ENTER:
			m_entersound = true;
			switch (video_cursor)
			{
				case (VIDEO_ITEMS - 1):
					Cvar_SetValueQuick (&vid_width, menu_video_resolutions[menu_video_resolution].width);
					Cvar_SetValueQuick (&vid_height, menu_video_resolutions[menu_video_resolution].height);
					Cvar_SetValueQuick (&vid_conwidth, menu_video_resolutions[menu_video_resolution].conwidth);
					Cvar_SetValueQuick (&vid_conheight, menu_video_resolutions[menu_video_resolution].conheight);
					Cvar_SetValueQuick (&vid_pixelheight, menu_video_resolutions[menu_video_resolution].pixelheight);
					Cbuf_AddText ("vid_restart\n");
					M_Menu_Options_f ();
					break;
				default:
					M_Menu_Video_AdjustSliders (1);
			}
			break;

		case K_UPARROW:
			S_LocalSound ("sound/menu/menu1.wav");
			video_cursor--;
			if (video_cursor < 0)
				video_cursor = VIDEO_ITEMS-1;
			break;

		case K_DOWNARROW:
			S_LocalSound ("sound/menu/menu1.wav");
			video_cursor++;
			if (video_cursor >= VIDEO_ITEMS)
				video_cursor = 0;
			break;

		case K_MOUSE2:
		case K_LEFTARROW:
			M_Menu_Video_AdjustSliders (-1);
			break;

		case K_RIGHTARROW:
			M_Menu_Video_AdjustSliders (1);
			break;
	}
}

//=============================================================================
/* HELP MENU */

static int		help_page;
#define	NUM_HELP_PAGES	6


void M_Menu_Help_f (void)
{
	key_dest = key_menu;
	m_state = m_help;
	m_entersound = true;
	help_page = 0;
}



static void M_Help_Draw (void)
{
	char vabuf[1024];
	M_Background2(320, 200);
	M_DrawPic (0, 0, va(vabuf, sizeof(vabuf), "gfx/help%i", help_page));
}


static void M_Help_Key (int key, int ascii)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_MOUSE1:
	case K_UPARROW:
	case K_RIGHTARROW:
		m_entersound = true;
		if (++help_page >= NUM_HELP_PAGES)
			help_page = 0;
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		m_entersound = true;
		if (--help_page < 0)
			help_page = NUM_HELP_PAGES-1;
		break;
	}

}

//=============================================================================
/* LAN CONFIG MENU */

static int		lanConfig_cursor = -1;
static int		lanConfig_cursor_table [] = {60, 75, 90};
#define NUM_LANCONFIG_CMDS	3

static int 	lanConfig_port;
static char	lanConfig_portname[6];
static char	lanConfig_joinname[40];

void M_Menu_LanConfig_f (void)
{
	key_dest = key_menu;
	m_state = m_lanconfig;
	m_entersound = true;
	if (lanConfig_cursor == -1) if (JoiningGame) lanConfig_cursor = 1;
	if (StartingGame) lanConfig_cursor = 1;
	lanConfig_port = 26000;
	dpsnprintf(lanConfig_portname, sizeof(lanConfig_portname), "%u", (unsigned int) lanConfig_port);

	M_Update_Return_Reason("");
}


static void M_LanConfig_Draw (void)
{
	cachepic_t	*p;
	int		basex;
	const char	*startJoin;
	const char	*protocol;
	char vabuf[1024];

	M_Background(320, 200);

	//M_DrawPic (16, 4, "gfx/qplaque");
	//p = Draw_CachePic ("gfx/p_multi");
	//basex = (320-Draw_GetPicWidth(p))/2;
	//M_DrawPic (basex, 4, "gfx/p_multi");

	basex = 80;

	if (StartingGame) M_Print(basex, 40, "HOST NEW GAME");
	else M_Print(basex, 40, "JOIN GAME");

	M_Print(basex, lanConfig_cursor_table[0], "Port:");
	M_DrawTextBox (basex+40, lanConfig_cursor_table[0]-8, sizeof(lanConfig_portname), 1);
	M_Print(basex + 50, lanConfig_cursor_table[0], lanConfig_portname);

	if (JoiningGame)
	{
		M_Print(basex, lanConfig_cursor_table[1], "Search for games...");
		M_Print(basex, lanConfig_cursor_table[2], "Join game at:");
		M_DrawTextBox (basex + 100, lanConfig_cursor_table[2]-8, 20, 1);
		M_Print(basex + 100 + 14, lanConfig_cursor_table[2], lanConfig_joinname);
	}
	else
	{
		M_DrawTextBox (basex-5, lanConfig_cursor_table[1]-8, 15, 1);
		M_Print(basex+8, lanConfig_cursor_table[1], "START NEW GAME");
	}

	M_DrawCharacter (basex-8, lanConfig_cursor_table[lanConfig_cursor], 12+((int)(realtime*4)&1));

	if (lanConfig_cursor == 0) M_DrawCharacter (basex+50 + 8*strlen(lanConfig_portname), lanConfig_cursor_table [lanConfig_cursor], 10+((int)(realtime*4)&1));
	if (lanConfig_cursor == 2) M_DrawCharacter (basex+116 + 8*strlen(lanConfig_joinname), lanConfig_cursor_table [lanConfig_cursor], 10+((int)(realtime*4)&1));

	if (*m_return_reason) M_Print(basex, 168, m_return_reason);
}


static void M_LanConfig_Key (int key, int ascii)
{
	int		l;
	char vabuf[1024];

	switch (key)
	{
	case K_MOUSE1:
		GetMyCursorPos();

		if (CursorX > 185 && CursorX < 500)
		{
			if (CursorY > 550 && CursorY <= 595)
			{
				m_entersound = true;
				lanConfig_cursor = 0;
			}
			if (CursorY > 595 && CursorY <= 645)
			{
				m_entersound = true;
				if (StartingGame)
				{
					M_Menu_GameOptions_f();
					break;
				}
				M_Menu_ServerList_f();
			}
			else if (CursorY > 645 && CursorY <= 690)
			{
				m_entersound = true;
				lanConfig_cursor = 2;
			}
		}
		break;
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		lanConfig_cursor--;
		if (lanConfig_cursor < 0) lanConfig_cursor = NUM_LANCONFIG_CMDS-1;
		// when in start game menu, skip the unused search qw servers item
		if (StartingGame && lanConfig_cursor == 2) lanConfig_cursor = 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		lanConfig_cursor++;
		if (lanConfig_cursor >= NUM_LANCONFIG_CMDS) lanConfig_cursor = 0;
		// when in start game menu, skip the unused search qw servers item
		if (StartingGame)
		{
			if (lanConfig_cursor == 1) lanConfig_cursor = 0;
			if (lanConfig_cursor == 0) lanConfig_cursor = 1;
		}
		break;

	case K_ENTER:
		if (lanConfig_cursor == 0) break;

		m_entersound = true;

		Cbuf_AddText ("stopdemo\n");
		Cvar_SetValue("port", lanConfig_port);

		if (lanConfig_cursor == 1)
		{
			if (StartingGame)
			{
				M_Menu_GameOptions_f ();
				break;
			}
			M_Menu_ServerList_f();
			break;
		}

		if (lanConfig_cursor == 2) Cbuf_AddText(va(vabuf, sizeof(vabuf), "connect \"%s\"\n", lanConfig_joinname) );
		break;

	case K_BACKSPACE:
		if (lanConfig_cursor == 0)
		{
			if (strlen(lanConfig_portname))
				lanConfig_portname[strlen(lanConfig_portname)-1] = 0;
		}

		if (lanConfig_cursor == 2)
		{
			if (strlen(lanConfig_joinname))
				lanConfig_joinname[strlen(lanConfig_joinname)-1] = 0;
		}
		break;

	default:
		if (ascii < 32)
			break;

		if (lanConfig_cursor == 2)
		{
			l = (int)strlen(lanConfig_joinname);
			if (l < (int)sizeof(lanConfig_joinname) - 1)
			{
				lanConfig_joinname[l+1] = 0;
				lanConfig_joinname[l] = ascii;
			}
		}

		if (ascii < '0' || ascii > '9')
			break;
		if (lanConfig_cursor == 0)
		{
			l = (int)strlen(lanConfig_portname);
			if (l < (int)sizeof(lanConfig_portname) - 1)
			{
				lanConfig_portname[l+1] = 0;
				lanConfig_portname[l] = ascii;
			}
		}
	}

	if (StartingGame && lanConfig_cursor == 2)
	{
		if (key == K_UPARROW) lanConfig_cursor = 1;
		else lanConfig_cursor = 0;
	}

	l =  atoi(lanConfig_portname);
	if (l <= 65535) lanConfig_port = l;
	dpsnprintf(lanConfig_portname, sizeof(lanConfig_portname), "%u", (unsigned int) lanConfig_port);
}

//=============================================================================
/* GAME OPTIONS MENU */

typedef struct level_s
{
	const char	*name;
	const char	*description;
} level_t;

typedef struct episode_s
{
	const char	*description;
	int		firstLevel;
	int		levels;
} episode_t;

typedef struct gamelevels_s
{
	const char *gamename;
	level_t *levels;
	episode_t *episodes;
	int numepisodes;
}
gamelevels_t;

static level_t quakelevels[] =
{
	{"start", "Entrance"},	// 0

	{"e1m1", "North entrance"},				// 1
	{"e1m2", "Loading area"},
	{"e1m3", "Train"},
	{"e1m4", "Sorting Center"},
	{"e1m5", "Corridors"},
	{"e1m6", "Base"},

	{"dusty",	"Dusty"},					// 7
	{"de_train", "Train station"},
	{"terrain", "Mountain terrain"},
	{"fy_snow", "fy_snow"},

	{"de_train", "Train station"},			// 11

	{"hns_multiply", "hns_multiply"},		// 12

	{"kz_cliff",	"Cliff"},					// 13
	{"bhop_wedge",	"Wedge"},					// 14
};

static episode_t quakeepisodes[] =
{
	{"Training room", 0, 1},
	{"Campaign (coop)", 1, 6},
	{"DeathMatch/Team DeathMatch ", 7, 4},
	{"Demolition", 11, 1},
	{"Hide 'N Seek", 12, 1},
	{"KreedZ", 13, 2},
	//{"Bhop", 23, 8},
	//{"Survival", 31, 1}
	//{"Deathmatch Arena", 32, 6}
};

static gamelevels_t sharewarequakegame = {"Shareware Quake", quakelevels, quakeepisodes, 2};
static gamelevels_t registeredquakegame = {"Quake", quakelevels, quakeepisodes, 6};

typedef struct gameinfo_s
{
	gamemode_t gameid;
	gamelevels_t *notregistered;
	gamelevels_t *registered;
}
gameinfo_t;

static gameinfo_t gamelist[] =
{
	{GAME_NORMAL, &sharewarequakegame, &registeredquakegame},
};

static gamelevels_t *gameoptions_levels  = NULL;

static int	startepisode;
static int	startlevel;
static int maxplayers;

void M_Menu_GameOptions_f (void)
{
	int i;
	key_dest = key_menu;
	m_state = m_gameoptions;
	m_entersound = true;
	if (maxplayers == 0) maxplayers = svs.maxclients;
	if (maxplayers < 2) maxplayers = min(8, MAX_SCOREBOARD);
	// pick game level list based on gamemode (use GAME_NORMAL if no matches)
	gameoptions_levels = registered.integer ? gamelist[0].registered : gamelist[0].notregistered;
	for (i = 0;i < (int)(sizeof(gamelist)/sizeof(gamelist[0]));i++)
		if (gamelist[i].gameid == gamemode)
			gameoptions_levels = registered.integer ? gamelist[i].registered : gamelist[i].notregistered;
}


static int gameoptions_cursor_table[] = {40, 56, 64, 72, 80, 88, 96, 104, 112, 124, 136, 144};
#define	NUM_GAMEOPTIONS	12
static int		gameoptions_cursor;

void M_GameOptions_Draw(void)
{
	int		x;
	char vabuf[1024];

	M_Background2(320, 200);

	//M_DrawTextBox(104, 32, 12, 1);
	M_Print(160, 40, "begin game");

	M_Print(0, 56, "      Max players:");
	M_Print(160, 56, va(vabuf, sizeof(vabuf), "%i", maxplayers));


	M_Print(0, 64, "      Game Type  :");
	{
		//if (!coop.integer && !deathmatch.integer) Cvar_SetValue("deathmatch", 1);
		if (deathmatch.integer == 0) M_Print(160, 64, "Cooperative");
		else if (deathmatch.integer == 1)M_Print(160, 64, "Deathmatch/Team deathmatch");
		else if (deathmatch.integer == 2) M_Print(160, 64, "Demolition");
		else if (deathmatch.integer == 3) M_Print(160, 64, "Hide 'N Seek DM");
		else if (deathmatch.integer == 4) M_Print(160, 64, "KreeDz/BunnyHop");
		else if (deathmatch.integer == 5) M_Print(160, 64, "Zombie mode");
	}

	M_Print(0, 72, "      Teamplay   :");
	{
		const char* msg;

		switch (teamplay.integer)
		{
			case 0: msg = "Off"; break;
			case 2: msg = "Friendly Fire"; break;
			default: msg = "No Friendly Fire"; break;
		}
		M_Print(160, 72, msg);
	}
	M_Print(0, 80, "      Difficulty :");
	{
		if (skill.integer == 0) M_Print(160, 80, "Easy");
		else if (skill.integer == 1) M_Print(160, 80, "Normal");
		else if (skill.integer == 2) M_Print(160, 80, "Hard");
		else M_Print(160, 80, "Nightmare difficulty");
	}
	M_Print(0, 88, "      Frag Limit :");
	if (fraglimit.integer == 0) M_Print(160, 88, "none");
	else M_Print(160, 88, va(vabuf, sizeof(vabuf), "%i frags", fraglimit.integer));

	M_Print(0, 96, "      Time Limit :");
	if (timelimit.integer == 0) M_Print(160, 96, "none");
	else M_Print(160, 96, va(vabuf, sizeof(vabuf), "%i minutes", timelimit.integer));

	M_Print(0, 104, "      Public     :");
	M_Print(160, 104, (sv_public.integer == 0) ? "no" : "yes");

	M_Print(0, 112, "      Maxrate    :");
	M_Print(160, 112, va(vabuf, sizeof(vabuf), "%i", sv_maxrate.integer));

	M_Print(0, 124, "      Server name:");
	M_DrawTextBox(156, 116, 15, 1);
	M_Print(164, 124, hostname.string);

	M_Print(0, 136, "      Mode       :");
	M_Print(160, 136, gameoptions_levels->episodes[startepisode].description);
	
	M_Print(0, 144, "      Level      :");
	M_Print(160, 144, gameoptions_levels->levels[gameoptions_levels->episodes[startepisode].firstLevel + startlevel].description);
	//gameoptions_levels->levels[gameoptions_levels->episodes[startepisode].firstLevel + startlevel].name

	// line cursor
	if (gameoptions_cursor == 9) M_DrawCharacter(164 + 8 * strlen(hostname.string), gameoptions_cursor_table[gameoptions_cursor], 10 + ((int)(realtime * 4) & 1));
	else if (gameoptions_cursor == 0) M_DrawCharacter(150, gameoptions_cursor_table[gameoptions_cursor], 12 + ((int)(realtime * 4) & 1));
	else M_DrawCharacter(150, gameoptions_cursor_table[gameoptions_cursor], 12 + ((int)(realtime * 4) & 1));

}


static void M_NetStart_Change (int dir)
{
	int count;

	switch (gameoptions_cursor)
	{
	case 1:
		maxplayers += dir;
		if (maxplayers > MAX_SCOREBOARD) maxplayers = MAX_SCOREBOARD;
		if (maxplayers < 2) maxplayers = 2;
		break;

	case 2:
		count = 5;
		Cvar_SetValueQuick(&deathmatch, deathmatch.integer + dir);
		Cvar_SetValueQuick(&coop, 0);
		if (deathmatch.integer > count)
		{
			Cvar_SetValueQuick(&deathmatch, 0);
			Cvar_SetValueQuick(&coop, 1);
		}
		else if (deathmatch.integer < 0)
		{
			Cvar_SetValueQuick(&deathmatch, 3);
			Cvar_SetValueQuick(&coop, 0);
		}

		//if (deathmatch.integer) // changing from deathmatch to coop
		//{
		//	Cvar_SetValueQuick (&coop, 1);
		//	Cvar_SetValueQuick (&deathmatch, 0);
		//}
		//else // changing from coop to deathmatch
		//{
		//	Cvar_SetValueQuick (&coop, 0);
		//	Cvar_SetValueQuick (&deathmatch, 1);
		//}
		break;

	case 3:
		count = 2;

		Cvar_SetValueQuick (&teamplay, teamplay.integer + dir);
		if (teamplay.integer > count)
			Cvar_SetValueQuick (&teamplay, 0);
		else if (teamplay.integer < 0)
			Cvar_SetValueQuick (&teamplay, count);
		break;

	case 4:
		Cvar_SetValueQuick (&skill, skill.integer + dir);
		if (skill.integer > 3) Cvar_SetValueQuick (&skill, 0);
		if (skill.integer < 0) Cvar_SetValueQuick (&skill, 3);
		break;

	case 5:
		Cvar_SetValueQuick (&fraglimit, fraglimit.integer + dir*10);
		if (fraglimit.integer > 100)
			Cvar_SetValueQuick (&fraglimit, 0);
		if (fraglimit.integer < 0)
			Cvar_SetValueQuick (&fraglimit, 100);
		break;

	case 6:
		Cvar_SetValueQuick (&timelimit, timelimit.value + dir*5);
		if (timelimit.value > 60)
			Cvar_SetValueQuick (&timelimit, 0);
		if (timelimit.value < 0)
			Cvar_SetValueQuick (&timelimit, 60);
		break;

	case 7:
		Cvar_SetValueQuick (&sv_public, !sv_public.integer);
		break;

	case 8:
		Cvar_SetValueQuick (&sv_maxrate, sv_maxrate.integer + dir*500);
		if (sv_maxrate.integer < NET_MINRATE)
			Cvar_SetValueQuick (&sv_maxrate, NET_MINRATE);
		break;

	case 9:
		break;

	case 10:
		startepisode += dir;

		if (startepisode < 0)
			startepisode = gameoptions_levels->numepisodes - 1;

		if (startepisode >= gameoptions_levels->numepisodes)
			startepisode = 0;

		startlevel = 0;
		break;

	case 11:
		startlevel += dir;

		if (startlevel < 0)
			startlevel = gameoptions_levels->episodes[startepisode].levels - 1;

		if (startlevel >= gameoptions_levels->episodes[startepisode].levels)
			startlevel = 0;
		break;
	}
}

static void M_GameOptions_Key (int key, int ascii)
{
	int l;
	char hostnamebuf[128];
	char vabuf[1024];

	switch (key)
	{
	case K_MOUSE1:
		GetMyCursorPos();

		if (CursorX > 200 && CursorX < 820)
		{
			if (CursorY > 300 && CursorY <= 354)
			{
					if (sv.active) Cbuf_AddText("disconnect\n");
					Cbuf_AddText(va(vabuf, sizeof(vabuf), "maxplayers %u\n", maxplayers));
					Cbuf_AddText(va(vabuf, sizeof(vabuf), "map %s\n", gameoptions_levels->levels[gameoptions_levels->episodes[startepisode].firstLevel + startlevel].name));
					return;
			}
			if (CursorY > 354 && CursorY <= 388)
			{
				m_entersound = true;
				gameoptions_cursor = 1;
				M_NetStart_Change(1);
			}
			for (int i = 0; i < 8; i ++)
			{
				if (CursorY > 388 + i*24 && CursorY <= 412 + i * 24)
				{
					m_entersound = true;
					gameoptions_cursor = 2+i;
					M_NetStart_Change(1);
				}
			}
			if (CursorY > 562 && CursorY <= 608)
			{
				m_entersound = true;
				gameoptions_cursor = 9;
			}
			if (CursorY > 610 && CursorY <= 634)
			{
				m_entersound = true;
				gameoptions_cursor = 10;
				M_NetStart_Change(1);
			}			
			if (CursorY > 634 && CursorY <= 662)
			{
				m_entersound = true;
				gameoptions_cursor = 11;
				M_NetStart_Change(1);
			}
		}
		break;

	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		gameoptions_cursor--;
		if (gameoptions_cursor < 0)
			gameoptions_cursor = NUM_GAMEOPTIONS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		gameoptions_cursor++;
		if (gameoptions_cursor >= NUM_GAMEOPTIONS)
			gameoptions_cursor = 0;
		break;

	case K_MOUSE2:
	case K_LEFTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("sound/misc/menu3.wav");
		M_NetStart_Change (-1);
		break;

	case K_RIGHTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("sound/misc/menu3.wav");
		M_NetStart_Change (1);
		break;

	case K_ENTER:
		S_LocalSound ("sound/menu/menu2.wav");
		if (gameoptions_cursor == 0)
		{
			if (sv.active)
				Cbuf_AddText("disconnect\n");
			Cbuf_AddText(va(vabuf, sizeof(vabuf), "maxplayers %u\n", maxplayers) );

			Cbuf_AddText(va(vabuf, sizeof(vabuf), "map %s\n", gameoptions_levels->levels[gameoptions_levels->episodes[startepisode].firstLevel + startlevel].name) );
			return;
		}

		M_NetStart_Change (1);
		break;

	case K_BACKSPACE:
		if (gameoptions_cursor == 9)
		{
			l = (int)strlen(hostname.string);
			if (l)
			{
				l = min(l - 1, 37);
				memcpy(hostnamebuf, hostname.string, l);
				hostnamebuf[l] = 0;
				Cvar_Set("hostname", hostnamebuf);
			}
		}
		break;

	default:
		if (ascii < 32)
			break;
		if (gameoptions_cursor == 9)
		{
			l = (int)strlen(hostname.string);
			if (l < 37)
			{
				memcpy(hostnamebuf, hostname.string, l);
				hostnamebuf[l] = ascii;
				hostnamebuf[l+1] = 0;
				Cvar_Set("hostname", hostnamebuf);
			}
		}
	}
}

//=============================================================================
/* SLIST MENU */

static int slist_cursor;

void M_Menu_ServerList_f (void)
{
	key_dest = key_menu;
	m_state = m_slist;
	m_entersound = true;
	slist_cursor = 0;
	M_Update_Return_Reason("");
	Net_Slist_f();
}


static void M_ServerList_Draw (void)
{
	int n, y, visible, start, end, statnumplayers, statmaxplayers;
	const char *s;
	char vabuf[1024];

	// use as much vertical space as available
	M_Background2(320, vid_conheight.integer);
	// scroll the list as the cursor moves
	ServerList_GetPlayerStatistics(&statnumplayers, &statmaxplayers);
	s = va(vabuf, sizeof(vabuf), "%i/%i masters %i/%i servers %i/%i players", masterreplycount, masterquerycount, serverreplycount, serverquerycount, statnumplayers, statmaxplayers);
	M_Print((320 - strlen(s) * 8) / 2, 32, s);
	if (*m_return_reason) M_Print(16, menu_height - 8, m_return_reason);
	y = 48;
	visible = (int)((menu_height - 16 - y) / 8 / 2);
	start = bound(0, slist_cursor - (visible >> 1), serverlist_viewcount - visible);
	end = min(start + visible, serverlist_viewcount);

	CursorMouseMoveServers(&slist_cursor, 150, 950, end);

	//auto p = Draw_CachePic ("gfx/p_multi");
	//M_DrawPic((640 - Draw_GetPicWidth(p)) / 2, 4, "gfx/p_multi");
	DrawQ_String(menu_x+42, 6, "SERVER BROWSER", 15, 16, 16, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
	DrawQ_Fill(menu_x, menu_y + y, 320, menu_height-16 - y  - menu_y, 0.6, 0.53, 0.4, 0.5, 0);

	if (end > start)
	{
		for (n = start;n < end;n++)
		{
			serverlist_entry_t *entry = ServerList_GetViewEntry(n);
			DrawQ_Fill(menu_x, menu_y + y, 320, 16, n == slist_cursor ? (0.5 + 0.2 * sin(realtime * M_PI)) : 0, 0, 0, 0.5, 0);
			M_PrintColored(0, y, entry->line1);y += 8;
			M_PrintColored(0, y, entry->line2);y += 8;
		}
	}
	else if (realtime - masterquerytime > 10)
	{
		if (masterquerycount)
		{
			char str[17] = "NO SERVERS FOUND";
			DrawQ_String(vid_conwidth.integer * 0.5 - strlen(str) * 4, vid_conheight.integer * 0.5, str, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
		}
		else 
		{
			char str[17] = "CAN NOT CONNECT";
			DrawQ_String(vid_conwidth.integer * 0.5 - strlen(str) * 4, vid_conheight.integer * 0.5, str, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
		}
	}
	else
	{
		if (serverquerycount)
		{
			char str[17] = "QUERYING SERVERS";
			DrawQ_String(vid_conwidth.integer * 0.5 - strlen(str) * 4, vid_conheight.integer * 0.5, str, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
		}
		else
		{
			char str[24] = "QUERYING MASTER SERVERS";
			DrawQ_String(vid_conwidth.integer * 0.5 - strlen(str) * 4, vid_conheight.integer * 0.5, str, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
		}
	}
}


static void M_ServerList_Key(int k, int ascii)
{
	char vabuf[1024];
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_LanConfig_f();
		break;

	case K_SPACE:
		Net_Slist_f();
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		slist_cursor--;
		if (slist_cursor < 0) slist_cursor = serverlist_viewcount - 1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("sound/menu/menu1.wav");
		slist_cursor++;
		if (slist_cursor >= serverlist_viewcount) slist_cursor = 0;
		break;

	case K_MOUSE1:
	case K_ENTER:
		S_LocalSound ("sound/menu/menu2.wav");
		if (serverlist_viewcount) Cbuf_AddText(va(vabuf, sizeof(vabuf), "connect \"%s\"\n", ServerList_GetViewEntry(slist_cursor)->info.cname));
		break;

	default:
		break;
	}

}

//=============================================================================
/* Menu Subsystem */

static void M_KeyEvent(int key, int ascii, qboolean downevent);
static void M_Draw(void);
void M_ToggleMenu(int mode);
static void M_Shutdown(void);

static void M_Init (void)
{
	Cmd_AddCommand ("menu_main", M_Menu_Main_f, "open the main menu");
	Cmd_AddCommand ("menu_singleplayer", M_Menu_SinglePlayer_f, "open the singleplayer menu");
	Cmd_AddCommand ("menu_load", M_Menu_Load_f, "open the loadgame menu");
	Cmd_AddCommand ("menu_save", M_Menu_Save_f, "open the savegame menu");
	Cmd_AddCommand ("menu_multiplayer", M_Menu_MultiPlayer_f, "open the multiplayer menu");
	Cmd_AddCommand ("menu_options", M_Menu_Options_f, "open the options menu");
	Cmd_AddCommand ("menu_options_effects", M_Menu_Options_Effects_f, "open the effects options menu");
	Cmd_AddCommand ("menu_options_graphics", M_Menu_Options_Graphics_f, "open the graphics options menu");
	Cmd_AddCommand ("menu_options_colorcontrol", M_Menu_Options_ColorControl_f, "open the color control menu");
	Cmd_AddCommand ("menu_keys", M_Menu_Keys_f, "open the key binding menu");
	Cmd_AddCommand ("menu_video", M_Menu_Video_f, "open the video options menu");
	Cmd_AddCommand ("menu_reset", M_Menu_Reset_f, "open the reset to defaults menu");
	Cmd_AddCommand ("menu_team", M_Menu_Team_f, "select team menu");
	Cmd_AddCommand ("help", M_Menu_Help_f, "open the help menu");
}

void M_Draw (void)
{
	char vabuf[1024];
	if (key_dest != key_menu && key_dest != key_menu_grabbed) m_state = m_none;

	if (m_state == m_none)
	{
		SDL_SetRelativeMouseMode(1);
		SDL_ShowCursor(0);
		return;
	}
	SDL_SetRelativeMouseMode(0);
	SDL_ShowCursor(1);

	
	GetMyCursorPos();
	CursorX = mouseX * 1000 / vid.mode.width;
	CursorY = mouseY * 1000 / vid.mode.height;

	//char str[50];
	//sprintf(str, "X=%d Y=%d\nMX=%0.7f MY=%d", CursorX, CursorY, realtime, mouseY);
	//DrawQ_String(0, 0, str, 50, 10, 10, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);

	//const char* watermark = "KleskBY";
	//const char* watermark_date = __DATE__;
	//DrawQ_String(0, 0, watermark, 50, 5, 5, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
	//DrawQ_String(200, 0, watermark_date, 50, 5, 5, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);

	switch (m_state)
	{
	case m_none:
		break;

	case m_main:
		M_Main_Draw ();
		break;

	case m_singleplayer:
		M_SinglePlayer_Draw ();
		break;

	case m_load:
		M_Load_Draw ();
		break;

	case m_save:
		M_Save_Draw ();
		break;

	case m_multiplayer:
		M_MultiPlayer_Draw ();
		break;

	case m_options:
		M_Options_Draw ();
		break;

	case m_options_effects:
		M_Options_Effects_Draw ();
		break;

	case m_options_graphics:
		M_Options_Graphics_Draw ();
		break;

	case m_options_colorcontrol:
		M_Options_ColorControl_Draw ();
		break;

	case m_keys:
		M_Keys_Draw ();
		break;

	case m_reset:
		M_Reset_Draw ();
		break;

	case m_video:
		M_Video_Draw ();
		break;

	case m_help:
		M_Help_Draw ();
		break;

	case m_lanconfig:
		M_LanConfig_Draw ();
		break;

	case m_gameoptions:
		M_GameOptions_Draw ();
		break;

	case m_slist:
		M_ServerList_Draw ();
		break;
	case m_team:
		M_Team_Draw();
		break;
	case m_class:
		M_Class_Draw();
		break;
	}

	if (m_entersound)
	{
		S_LocalSound ("sound/menu/menu2.wav");
		m_entersound = false;
	}

	S_ExtraUpdate ();
}


void M_KeyEvent (int key, int ascii, qboolean downevent)
{
	if (!downevent)
		return;
	switch (m_state)
	{
	case m_none:
		return;

	case m_main:
		M_Main_Key (key, ascii);
		return;

	case m_singleplayer:
		M_SinglePlayer_Key (key, ascii);
		return;

	case m_load:
		M_Load_Key (key, ascii);
		return;

	case m_save:
		M_Save_Key (key, ascii);
		return;

	case m_multiplayer:
		M_MultiPlayer_Key (key, ascii);
		return;

	case m_options:
		M_Options_Key (key, ascii);
		return;

	case m_options_effects:
		M_Options_Effects_Key (key, ascii);
		return;

	case m_options_graphics:
		M_Options_Graphics_Key (key, ascii);
		return;

	case m_options_colorcontrol:
		M_Options_ColorControl_Key (key, ascii);
		return;

	case m_keys:
		M_Keys_Key (key, ascii);
		return;

	case m_reset:
		M_Reset_Key (key, ascii);
		return;

	case m_video:
		M_Video_Key (key, ascii);
		return;

	case m_help:
		M_Help_Key (key, ascii);
		return;

	case m_lanconfig:
		M_LanConfig_Key (key, ascii);
		return;

	case m_gameoptions:
		M_GameOptions_Key (key, ascii);
		return;

	case m_slist:
		M_ServerList_Key (key, ascii);
		return;
	case m_team:
		M_Team_Key(key, ascii);
		return;
	case m_class:
		M_Class_Key(key, ascii);
		return;
	}

}

static void M_NewMap(void)
{
}

static int M_GetServerListEntryCategory(const serverlist_entry_t *entry)
{
	return 0;
}

void M_Shutdown(void)
{
	// reset key_dest
	key_dest = key_game;
}

//============================================================================
// Menu prog handling

static const char *m_required_func[] = {
"m_init",
"m_keydown",
"m_draw",
"m_toggle",
"m_shutdown",
};

static int m_numrequiredfunc = sizeof(m_required_func) / sizeof(char*);

static prvm_required_field_t m_required_fields[] =
{
#define PRVM_DECLARE_serverglobalfloat(x)
#define PRVM_DECLARE_serverglobalvector(x)
#define PRVM_DECLARE_serverglobalstring(x)
#define PRVM_DECLARE_serverglobaledict(x)
#define PRVM_DECLARE_serverglobalfunction(x)
#define PRVM_DECLARE_clientglobalfloat(x)
#define PRVM_DECLARE_clientglobalvector(x)
#define PRVM_DECLARE_clientglobalstring(x)
#define PRVM_DECLARE_clientglobaledict(x)
#define PRVM_DECLARE_clientglobalfunction(x)
#define PRVM_DECLARE_menuglobalfloat(x)
#define PRVM_DECLARE_menuglobalvector(x)
#define PRVM_DECLARE_menuglobalstring(x)
#define PRVM_DECLARE_menuglobaledict(x)
#define PRVM_DECLARE_menuglobalfunction(x)
#define PRVM_DECLARE_serverfieldfloat(x)
#define PRVM_DECLARE_serverfieldvector(x)
#define PRVM_DECLARE_serverfieldstring(x)
#define PRVM_DECLARE_serverfieldedict(x)
#define PRVM_DECLARE_serverfieldfunction(x)
#define PRVM_DECLARE_clientfieldfloat(x)
#define PRVM_DECLARE_clientfieldvector(x)
#define PRVM_DECLARE_clientfieldstring(x)
#define PRVM_DECLARE_clientfieldedict(x)
#define PRVM_DECLARE_clientfieldfunction(x)
#define PRVM_DECLARE_menufieldfloat(x) {ev_float, #x},
#define PRVM_DECLARE_menufieldvector(x) {ev_vector, #x},
#define PRVM_DECLARE_menufieldstring(x) {ev_string, #x},
#define PRVM_DECLARE_menufieldedict(x) {ev_entity, #x},
#define PRVM_DECLARE_menufieldfunction(x) {ev_function, #x},
#define PRVM_DECLARE_serverfunction(x)
#define PRVM_DECLARE_clientfunction(x)
#define PRVM_DECLARE_menufunction(x)
#define PRVM_DECLARE_field(x)
#define PRVM_DECLARE_global(x)
#define PRVM_DECLARE_function(x)
#include "prvm_offsets.h"
#undef PRVM_DECLARE_serverglobalfloat
#undef PRVM_DECLARE_serverglobalvector
#undef PRVM_DECLARE_serverglobalstring
#undef PRVM_DECLARE_serverglobaledict
#undef PRVM_DECLARE_serverglobalfunction
#undef PRVM_DECLARE_clientglobalfloat
#undef PRVM_DECLARE_clientglobalvector
#undef PRVM_DECLARE_clientglobalstring
#undef PRVM_DECLARE_clientglobaledict
#undef PRVM_DECLARE_clientglobalfunction
#undef PRVM_DECLARE_menuglobalfloat
#undef PRVM_DECLARE_menuglobalvector
#undef PRVM_DECLARE_menuglobalstring
#undef PRVM_DECLARE_menuglobaledict
#undef PRVM_DECLARE_menuglobalfunction
#undef PRVM_DECLARE_serverfieldfloat
#undef PRVM_DECLARE_serverfieldvector
#undef PRVM_DECLARE_serverfieldstring
#undef PRVM_DECLARE_serverfieldedict
#undef PRVM_DECLARE_serverfieldfunction
#undef PRVM_DECLARE_clientfieldfloat
#undef PRVM_DECLARE_clientfieldvector
#undef PRVM_DECLARE_clientfieldstring
#undef PRVM_DECLARE_clientfieldedict
#undef PRVM_DECLARE_clientfieldfunction
#undef PRVM_DECLARE_menufieldfloat
#undef PRVM_DECLARE_menufieldvector
#undef PRVM_DECLARE_menufieldstring
#undef PRVM_DECLARE_menufieldedict
#undef PRVM_DECLARE_menufieldfunction
#undef PRVM_DECLARE_serverfunction
#undef PRVM_DECLARE_clientfunction
#undef PRVM_DECLARE_menufunction
#undef PRVM_DECLARE_field
#undef PRVM_DECLARE_global
#undef PRVM_DECLARE_function
};

static int m_numrequiredfields = sizeof(m_required_fields) / sizeof(m_required_fields[0]);

static prvm_required_field_t m_required_globals[] =
{
#define PRVM_DECLARE_serverglobalfloat(x)
#define PRVM_DECLARE_serverglobalvector(x)
#define PRVM_DECLARE_serverglobalstring(x)
#define PRVM_DECLARE_serverglobaledict(x)
#define PRVM_DECLARE_serverglobalfunction(x)
#define PRVM_DECLARE_clientglobalfloat(x)
#define PRVM_DECLARE_clientglobalvector(x)
#define PRVM_DECLARE_clientglobalstring(x)
#define PRVM_DECLARE_clientglobaledict(x)
#define PRVM_DECLARE_clientglobalfunction(x)
#define PRVM_DECLARE_menuglobalfloat(x) {ev_float, #x},
#define PRVM_DECLARE_menuglobalvector(x) {ev_vector, #x},
#define PRVM_DECLARE_menuglobalstring(x) {ev_string, #x},
#define PRVM_DECLARE_menuglobaledict(x) {ev_entity, #x},
#define PRVM_DECLARE_menuglobalfunction(x) {ev_function, #x},
#define PRVM_DECLARE_serverfieldfloat(x)
#define PRVM_DECLARE_serverfieldvector(x)
#define PRVM_DECLARE_serverfieldstring(x)
#define PRVM_DECLARE_serverfieldedict(x)
#define PRVM_DECLARE_serverfieldfunction(x)
#define PRVM_DECLARE_clientfieldfloat(x)
#define PRVM_DECLARE_clientfieldvector(x)
#define PRVM_DECLARE_clientfieldstring(x)
#define PRVM_DECLARE_clientfieldedict(x)
#define PRVM_DECLARE_clientfieldfunction(x)
#define PRVM_DECLARE_menufieldfloat(x)
#define PRVM_DECLARE_menufieldvector(x)
#define PRVM_DECLARE_menufieldstring(x)
#define PRVM_DECLARE_menufieldedict(x)
#define PRVM_DECLARE_menufieldfunction(x)
#define PRVM_DECLARE_serverfunction(x)
#define PRVM_DECLARE_clientfunction(x)
#define PRVM_DECLARE_menufunction(x)
#define PRVM_DECLARE_field(x)
#define PRVM_DECLARE_global(x)
#define PRVM_DECLARE_function(x)
#include "prvm_offsets.h"
#undef PRVM_DECLARE_serverglobalfloat
#undef PRVM_DECLARE_serverglobalvector
#undef PRVM_DECLARE_serverglobalstring
#undef PRVM_DECLARE_serverglobaledict
#undef PRVM_DECLARE_serverglobalfunction
#undef PRVM_DECLARE_clientglobalfloat
#undef PRVM_DECLARE_clientglobalvector
#undef PRVM_DECLARE_clientglobalstring
#undef PRVM_DECLARE_clientglobaledict
#undef PRVM_DECLARE_clientglobalfunction
#undef PRVM_DECLARE_menuglobalfloat
#undef PRVM_DECLARE_menuglobalvector
#undef PRVM_DECLARE_menuglobalstring
#undef PRVM_DECLARE_menuglobaledict
#undef PRVM_DECLARE_menuglobalfunction
#undef PRVM_DECLARE_serverfieldfloat
#undef PRVM_DECLARE_serverfieldvector
#undef PRVM_DECLARE_serverfieldstring
#undef PRVM_DECLARE_serverfieldedict
#undef PRVM_DECLARE_serverfieldfunction
#undef PRVM_DECLARE_clientfieldfloat
#undef PRVM_DECLARE_clientfieldvector
#undef PRVM_DECLARE_clientfieldstring
#undef PRVM_DECLARE_clientfieldedict
#undef PRVM_DECLARE_clientfieldfunction
#undef PRVM_DECLARE_menufieldfloat
#undef PRVM_DECLARE_menufieldvector
#undef PRVM_DECLARE_menufieldstring
#undef PRVM_DECLARE_menufieldedict
#undef PRVM_DECLARE_menufieldfunction
#undef PRVM_DECLARE_serverfunction
#undef PRVM_DECLARE_clientfunction
#undef PRVM_DECLARE_menufunction
#undef PRVM_DECLARE_field
#undef PRVM_DECLARE_global
#undef PRVM_DECLARE_function
};

static int m_numrequiredglobals = sizeof(m_required_globals) / sizeof(m_required_globals[0]);

void MR_SetRouting (qboolean forceold);

void MVM_error_cmd(const char *format, ...) DP_FUNC_PRINTF(1);
void MVM_error_cmd(const char *format, ...)
{
	prvm_prog_t *prog = MVM_prog;
	static qboolean processingError = false;
	char errorstring[MAX_INPUTLINE];
	va_list argptr;

	va_start (argptr, format);
	dpvsnprintf (errorstring, sizeof(errorstring), format, argptr);
	va_end (argptr);
	Con_Printf( "Menu_Error: %s\n", errorstring );

	if( !processingError ) 
	{
		processingError = true;
		PRVM_Crash(prog);
		processingError = false;
	} 
	else Con_Printf( "Menu_Error: Recursive call to MVM_error_cmd (from PRVM_Crash)!\n" );

	// fall back to the normal menu

	// say it
	Con_Print("Falling back to normal menu\n");

	key_dest = key_game;

	// init the normal menu now -> this will also correct the menu router pointers
	MR_SetRouting (TRUE);

	// reset the active scene, too (to be on the safe side ;))
   R_SelectScene( RST_CLIENT );

	Host_AbortCurrentFrame();
}

static void MVM_begin_increase_edicts(prvm_prog_t *prog)
{
}

static void MVM_end_increase_edicts(prvm_prog_t *prog)
{
}

static void MVM_init_edict(prvm_prog_t *prog, prvm_edict_t *edict)
{
}

static void MVM_free_edict(prvm_prog_t *prog, prvm_edict_t *ed)
{
}

static void MVM_count_edicts(prvm_prog_t *prog)
{
	int i;
	prvm_edict_t *ent;
	int active;

	active = 0;
	for (i=0 ; i<prog->num_edicts ; i++)
	{
		ent = PRVM_EDICT_NUM(i);
		if (ent->priv.required->free) continue;
		active++;
	}

	Con_Printf("num_edicts:%3i\n", prog->num_edicts);
	Con_Printf("active    :%3i\n", active);
}

static qboolean MVM_load_edict(prvm_prog_t *prog, prvm_edict_t *ent)
{
	return true;
}

static void MP_KeyEvent (int key, int ascii, qboolean downevent)
{
	prvm_prog_t *prog = MVM_prog;

	// pass key
	prog->globals.fp[OFS_PARM0] = (prvm_vec_t) key;
	prog->globals.fp[OFS_PARM1] = (prvm_vec_t) ascii;
	if (downevent)
		prog->ExecuteProgram(prog, PRVM_menufunction(m_keydown),"m_keydown(float key, float ascii) required");
	else if (PRVM_menufunction(m_keyup))
		prog->ExecuteProgram(prog, PRVM_menufunction(m_keyup),"m_keyup(float key, float ascii) required");
}

static void MP_Draw (void)
{
	prvm_prog_t *prog = MVM_prog;
	// declarations that are needed right now

	float oldquality;

	R_SelectScene( RST_MENU );

	// reset the temp entities each frame
	r_refdef.scene.numtempentities = 0;

	// menu scenes do not use reduced rendering quality
	oldquality = r_refdef.view.quality;
	r_refdef.view.quality = 1;
	// TODO: this needs to be exposed to R_SetView (or something similar) ASAP [2/5/2008 Andreas]
	r_refdef.scene.time = realtime;

	// FIXME: this really shouldnt error out lest we have a very broken refdef state...?
	// or does it kill the server too?
	PRVM_G_FLOAT(OFS_PARM0) = vid.width;
	PRVM_G_FLOAT(OFS_PARM1) = vid.height;
	prog->ExecuteProgram(prog, PRVM_menufunction(m_draw),"m_draw() required");

	// TODO: imo this should be moved into scene, too [1/27/2008 Andreas]
	r_refdef.view.quality = oldquality;

	R_SelectScene( RST_CLIENT );
}

static void MP_ToggleMenu(int mode)
{
	prvm_prog_t *prog = MVM_prog;

	prog->globals.fp[OFS_PARM0] = (prvm_vec_t) mode;
	prog->ExecuteProgram(prog, PRVM_menufunction(m_toggle),"m_toggle(float mode) required");
}

static void MP_NewMap(void)
{
	prvm_prog_t *prog = MVM_prog;
	if (PRVM_menufunction(m_newmap))
		prog->ExecuteProgram(prog, PRVM_menufunction(m_newmap),"m_newmap() required");
}

const serverlist_entry_t *serverlist_callbackentry = NULL;
static int MP_GetServerListEntryCategory(const serverlist_entry_t *entry)
{
	prvm_prog_t *prog = MVM_prog;
	serverlist_callbackentry = entry;
	if (PRVM_menufunction(m_gethostcachecategory))
	{
		prog->globals.fp[OFS_PARM0] = (prvm_vec_t) -1;
		prog->ExecuteProgram(prog, PRVM_menufunction(m_gethostcachecategory),"m_gethostcachecategory(float entry) required");
		serverlist_callbackentry = NULL;
		return prog->globals.fp[OFS_RETURN];
	}
	else
	{
		return 0;
	}
}

static void MP_Shutdown (void)
{
	prvm_prog_t *prog = MVM_prog;
	if (prog->loaded)
		prog->ExecuteProgram(prog, PRVM_menufunction(m_shutdown),"m_shutdown() required");

	// reset key_dest
	key_dest = key_game;

	// AK not using this cause Im not sure whether this is useful at all instead :
	PRVM_Prog_Reset(prog);
}

static void MP_Init (void)
{
	prvm_prog_t *prog = MVM_prog;
	PRVM_Prog_Init(prog);

	prog->edictprivate_size = 0; // no private struct used
	prog->name = "menu";
	prog->num_edicts = 1;
	prog->limit_edicts = M_MAX_EDICTS;
	prog->extensionstring = vm_m_extensions;
	prog->builtins = vm_m_builtins;
	prog->numbuiltins = vm_m_numbuiltins;

	// all callbacks must be defined (pointers are not checked before calling)
	prog->begin_increase_edicts = MVM_begin_increase_edicts;
	prog->end_increase_edicts   = MVM_end_increase_edicts;
	prog->init_edict            = MVM_init_edict;
	prog->free_edict            = MVM_free_edict;
	prog->count_edicts          = MVM_count_edicts;
	prog->load_edict            = MVM_load_edict;
	prog->init_cmd              = MVM_init_cmd;
	prog->reset_cmd             = MVM_reset_cmd;
	prog->error_cmd             = MVM_error_cmd;
	prog->ExecuteProgram        = MVM_ExecuteProgram;

	// allocate the mempools
	prog->progs_mempool = Mem_AllocPool(menu_progs.string, 0, NULL);

	PRVM_Prog_Load(prog, menu_progs.string, NULL, 0, m_numrequiredfunc, m_required_func, m_numrequiredfields, m_required_fields, m_numrequiredglobals, m_required_globals);

	// note: OP_STATE is not supported by menu qc, we don't even try to detect
	// it here

	in_client_mouse = true;

	// call the prog init
	prog->ExecuteProgram(prog, PRVM_menufunction(m_init),"m_init() required");

	// Once m_init was called, we consider menuqc code fully initialized.
	prog->inittime = realtime;
}

//============================================================================
// Menu router

void (*MR_KeyEvent) (int key, int ascii, qboolean downevent);
void (*MR_Draw) (void);
void (*MR_ToggleMenu) (int mode);
void (*MR_Shutdown) (void);
void (*MR_NewMap) (void);
int (*MR_GetServerListEntryCategory) (const serverlist_entry_t *entry);

void MR_SetRouting(qboolean forceold)
{
	// if the menu prog isnt available or forceqmenu ist set, use the old menu
	if(!FS_FileExists(menu_progs.string) || forceqmenu.integer || forceold)
	{
		// set menu router function pointers
		MR_KeyEvent = M_KeyEvent;
		MR_Draw = M_Draw;
		MR_ToggleMenu = M_ToggleMenu;
		MR_Shutdown = M_Shutdown;
		MR_NewMap = M_NewMap;
		MR_GetServerListEntryCategory = M_GetServerListEntryCategory;
		M_Init();
	}
	else
	{
		// set menu router function pointers
		MR_KeyEvent = MP_KeyEvent;
		MR_Draw = MP_Draw;
		MR_ToggleMenu = MP_ToggleMenu;
		MR_Shutdown = MP_Shutdown;
		MR_NewMap = MP_NewMap;
		MR_GetServerListEntryCategory = MP_GetServerListEntryCategory;
		MP_Init();
	}
}

void MR_Restart(void)
{
	if(MR_Shutdown) MR_Shutdown();
	MR_SetRouting (FALSE);
}

static void Call_MR_ToggleMenu_f(void)
{
	int m;
	m = ((Cmd_Argc() < 2) ? -1 : atoi(Cmd_Argv(1)));
	Host_StartVideo();
	if(MR_ToggleMenu)
		MR_ToggleMenu(m);
}

void MR_Init_Commands(void)
{
	// set router console commands
	Cvar_RegisterVariable (&forceqmenu);
	Cvar_RegisterVariable (&menu_options_colorcontrol_correctionvalue);
	Cvar_RegisterVariable (&menu_progs);
	Cmd_AddCommand ("menu_restart",MR_Restart, "restart menu system (reloads menu.dat)");
	Cmd_AddCommand ("togglemenu", Call_MR_ToggleMenu_f, "opens or closes menu");
}

void MR_Init(void)
{
	vid_mode_t res[1024];
	size_t res_count, i;

	res_count = VID_ListModes(res, sizeof(res) / sizeof(*res));
	res_count = VID_SortModes(res, res_count, false, false, true);
	if(res_count)
	{
		video_resolutions_count = (int)res_count;
		video_resolutions = (video_resolution_t *) Mem_Alloc(cls.permanentmempool, sizeof(*video_resolutions) * (video_resolutions_count + 1));
		memset(&video_resolutions[video_resolutions_count], 0, sizeof(video_resolutions[video_resolutions_count]));
		for(i = 0; i < res_count; ++i)
		{
			int n, d, t;
			video_resolutions[i].type = "Detected mode"; // FIXME make this more dynamic
			video_resolutions[i].width = res[i].width;
			video_resolutions[i].height = res[i].height;
			video_resolutions[i].pixelheight = res[i].pixelheight_num / (double) res[i].pixelheight_denom;
			n = res[i].pixelheight_denom * video_resolutions[i].width;
			d = res[i].pixelheight_num * video_resolutions[i].height;
			while(d)
			{
				t = n;
				n = d;
				d = t % d;
			}
			d = (res[i].pixelheight_num * video_resolutions[i].height) / n;
			n = (res[i].pixelheight_denom * video_resolutions[i].width) / n;
			switch(n * 0x10000 | d)
			{
				case 0x00040003:
					video_resolutions[i].conwidth = 640;
					video_resolutions[i].conheight = 480;
					video_resolutions[i].type = "Standard 4x3";
					break;
				case 0x00050004:
					video_resolutions[i].conwidth = 640;
					video_resolutions[i].conheight = 512;
					if(res[i].pixelheight_denom == res[i].pixelheight_num)
						video_resolutions[i].type = "Square Pixel (LCD) 5x4";
					else
						video_resolutions[i].type = "Short Pixel (CRT) 5x4";
					break;
				case 0x00080005:
					video_resolutions[i].conwidth = 640;
					video_resolutions[i].conheight = 400;
					if(res[i].pixelheight_denom == res[i].pixelheight_num)
						video_resolutions[i].type = "Widescreen 8x5";
					else
						video_resolutions[i].type = "Tall Pixel (CRT) 8x5";

					break;
				case 0x00050003:
					video_resolutions[i].conwidth = 640;
					video_resolutions[i].conheight = 384;
					video_resolutions[i].type = "Widescreen 5x3";
					break;
				case 0x000D0009:
					video_resolutions[i].conwidth = 640;
					video_resolutions[i].conheight = 400;
					video_resolutions[i].type = "Widescreen 14x9";
					break;
				case 0x00100009:
					video_resolutions[i].conwidth = 640;
					video_resolutions[i].conheight = 480;
					video_resolutions[i].type = "Widescreen 16x9";
					break;
				case 0x00030002:
					video_resolutions[i].conwidth = 720;
					video_resolutions[i].conheight = 480;
					video_resolutions[i].type = "NTSC 3x2";
					break;
				case 0x000D000B:
					video_resolutions[i].conwidth = 720;
					video_resolutions[i].conheight = 566;
					video_resolutions[i].type = "PAL 14x11";
					break;
				case 0x00080007:
					if(video_resolutions[i].width >= 512)
					{
						video_resolutions[i].conwidth = 512;
						video_resolutions[i].conheight = 448;
						video_resolutions[i].type = "SNES 8x7";
					}
					else
					{
						video_resolutions[i].conwidth = 256;
						video_resolutions[i].conheight = 224;
						video_resolutions[i].type = "NES 8x7";
					}
					break;
				default:
					video_resolutions[i].conwidth = 640;
					video_resolutions[i].conheight = 640 * d / n;
					video_resolutions[i].type = "Detected mode";
					break;
			}
			if(video_resolutions[i].conwidth > video_resolutions[i].width || video_resolutions[i].conheight > video_resolutions[i].height)
			{
				int f1, f2;
				f1 = video_resolutions[i].conwidth > video_resolutions[i].width;
				f2 = video_resolutions[i].conheight > video_resolutions[i].height;
				if(f1 > f2)
				{
					video_resolutions[i].conwidth = video_resolutions[i].width;
					video_resolutions[i].conheight = video_resolutions[i].conheight / f1;
				}
				else
				{
					video_resolutions[i].conwidth = video_resolutions[i].conwidth / f2;
					video_resolutions[i].conheight = video_resolutions[i].height;
				}
			}
			video_resolutions[i].conwidth = 640;
			video_resolutions[i].conheight = 320;
		}
	}
	else
	{
		video_resolutions = video_resolutions_hardcoded;
		video_resolutions_count = sizeof(video_resolutions_hardcoded) / sizeof(*video_resolutions_hardcoded) - 1;
	}

	menu_video_resolutions_forfullscreen = !!vid_fullscreen.integer;
	M_Menu_Video_FindResolution(vid.width, vid.height, vid_pixelheight.value);

	// use -forceqmenu to use always the normal quake menu (it sets forceqmenu to 1)
// COMMANDLINEOPTION: Client: -forceqmenu disables menu.dat (same as +forceqmenu 1)
	if(COM_CheckParm("-forceqmenu")) Cvar_SetValueQuick(&forceqmenu,1);
	// use -useqmenu for debugging proposes, cause it starts
	// the normal quake menu only the first time
// COMMANDLINEOPTION: Client: -useqmenu causes the first time you open the menu to use the quake menu, then reverts to menu.dat (if forceqmenu is 0)
	if(COM_CheckParm("-useqmenu"))
		MR_SetRouting (TRUE);
	else
		MR_SetRouting (FALSE);
}
