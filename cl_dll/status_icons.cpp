/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// status_icons.cpp
//
#include "hud.h"
#include "cl_util.h"
#include "draw_util.h"
#include "triangleapi.h"
#include "r_efx.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "parsemsg.h"
#include "event_api.h"
#include "com_weapons.h"

int CHudStatusIcons::Init( void )
{
	HOOK_MESSAGE( gHUD.m_StatusIcons, StatusIcon );
	HOOK_MESSAGE( gHUD.m_StatusIcons, HudIcons );
	HOOK_MESSAGE( gHUD.m_StatusIcons, CzTool );
	HOOK_MESSAGE( gHUD.m_StatusIcons, CzFiber );

	gHUD.AddHudElem( this );

	Reset();

	return 1;
}

int CHudStatusIcons::VidInit( void )
{
	return 1;
}

int g_czTool = 0;
static int   g_czCount = 0, g_czHold = 0;
static float g_czPlateT = 0;
static int   g_czZoneShown = 0;
int g_czFiber = 0;
static int   g_czZoom = 0;

void CHudStatusIcons::Reset( void )
{
	g_czTool = 0; g_czFiber = 0; g_czZoom = 0; g_czZoneShown = 0; g_czPlateT = 0;
	memset( m_IconList, 0, sizeof m_IconList );
	m_iFlags &= ~HUD_DRAW;
}

// ---- CZ mission tools HUD ----

int CHudStatusIcons::MsgFunc_CzTool( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	int tool = reader.ReadByte();
	g_czCount = reader.ReadByte();
	g_czHold = reader.ReadByte();
	if( tool != g_czTool && tool )
		g_czPlateT = gHUD.m_flTime + 3.0f;
	g_czTool = tool;
	m_iFlags |= HUD_DRAW;
	return 1;
}

int CHudStatusIcons::MsgFunc_CzFiber( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	g_czFiber = reader.ReadByte();
	g_czZoom = reader.ReadByte();
	m_iFlags |= HUD_DRAW;
	return 1;
}

static void CZ_DrawSprite( const char *name, int x, int y, int r, int g, int b )
{
	int idx = gHUD.GetSpriteIndex( name );
	if( idx < 0 )
		return;
	HSPRITE spr = gHUD.GetSprite( idx );
	if( !spr )
		return;
	SPR_Set( spr, r, g, b );
	SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( idx ) );
}

static void CZ_DrawTools( float flTime )
{
	static const char *toolIcons[] = { "", "icon_camera", "icon_fibercamera", "icon_blowtorch", "icon_radiocontrolledbomb" };

	// fiber optic camera: round red-tinted view with scan lines
	if( g_czFiber )
	{
		int W = ScreenWidth, H = ScreenHeight;
		int cx = W / 2, cy = H / 2;
		int R = (int)( ( W < H ? W : H ) * 0.46f );
		FillRGBABlend( 0, 0, W, H, 220, 0, 0, 40 );
		const int step = 3;
		for( int y = 0; y < H; y += step )
		{
			int dy = y + step / 2 - cy;
			int half = 0;
			if( dy > -R && dy < R )
				half = (int)sqrtf( (float)( R * R - dy * dy ) );
			if( half <= 0 )
			{
				FillRGBABlend( 0, y, W, step, 0, 0, 0, 255 );
				continue;
			}
			FillRGBABlend( 0, y, cx - half, step, 0, 0, 0, 255 );
			FillRGBABlend( cx + half, y, W - ( cx + half ), step, 0, 0, 0, 255 );
			// scan line
			FillRGBABlend( cx - half, y, half * 2, 1, 0, 0, 0, 70 );
		}
		return;
	}

	// "you are in a ... zone" hint
	if( g_czZoneShown )
	{
		static const struct { int bit; int tool; const char *text; } zones[] =
		{
			{ 4,   1, "You are in a camera zone" },
			{ 8,   2, "You are in a fiber optic camera zone" },
			{ 32,  4, "You are in a radio controlled bomb zone" },
			{ 64,  3, "You are in a blowtorch zone" },
			{ 512, 0, "You are in a bomb defuse zone" },
		};
		for( size_t i = 0; i < sizeof( zones ) / sizeof( zones[0] ); i++ )
		{
			if( ( g_czZoneShown & zones[i].bit ) && ( !zones[i].tool || g_czTool == zones[i].tool ) )
			{
				int len = DrawUtils::HudStringLen( zones[i].text );
				DrawUtils::DrawHudString( ScreenWidth / 2 - len / 2, (int)( ScreenHeight * 0.14f ), ScreenWidth, zones[i].text, 90, 150, 255 );
				break;
			}
		}
	}

	// blowtorch flame: a small pink-white spot with a soft halo, projected onto the screen so the view model cannot hide it
	{
		extern vec3_t g_czTorchPos;
		extern bool g_czTorchOn, g_czTorchAtk;
		extern float g_czTorchSize;
		static model_t *flameSpr = nullptr;
		if( g_czTorchOn && g_czTool == 3 && !g_czFiber )
		{
			if( !flameSpr )
				flameSpr = gEngfuncs.LoadMapSprite( "sprites/glow01.spr" );
			vec3_t scr;
			if( flameSpr && !gEngfuncs.pTriAPI->WorldToScreen( g_czTorchPos, scr ) )
			{
				float cx = ( 1.0f + scr[0] ) * 0.5f * ScreenWidth;
				float cy = ( 1.0f - scr[1] ) * 0.5f * ScreenHeight;
				float u = ScreenHeight / 480.0f;
				gEngfuncs.pTriAPI->SpriteTexture( flameSpr, 0 );
				gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
				gEngfuncs.pTriAPI->CullFace( TRI_NONE );
				float halo = 26.0f * u * g_czTorchSize, core = 7.0f * u * g_czTorchSize;
				gEngfuncs.pTriAPI->Color4f( 0.92f, 0.55f, 0.75f, g_czTorchAtk ? 0.55f : 0.40f );
				DrawUtils::Draw2DQuad( cx - halo, cy - halo, cx + halo, cy + halo );
				gEngfuncs.pTriAPI->Color4f( 1.0f, 0.85f, 0.92f, 1.0f );
				DrawUtils::Draw2DQuad( cx - core, cy - core, cx + core, cy + core );
			}
		}
	}

	if( g_czTool < 1 || g_czTool > 4 )
		return;

	// "EQUIPMENT" plate on the right for a few seconds after a tool was taken
	if( flTime < g_czPlateT )
	{
		int w = 110, h = 96;
		int x = ScreenWidth - w - 10, y = ScreenHeight / 2 - h / 2;
		FillRGBABlend( x, y, w, h, 0, 0, 0, 90 );
		DrawUtils::DrawHudString( x + 12, y + 4, ScreenWidth, "EQUIPMENT", 50, 250, 50 );
		CZ_DrawSprite( toolIcons[g_czTool], x + 4, y + 22, 50, 250, 50 );
		if( g_czTool == 4 )
			DrawUtils::DrawHudNumberString( x + w - 8, y + h - 24, 0, g_czCount, 50, 250, 50 );
	}

	// mines: icon and counter in the bottom right corner
	if( g_czTool == 4 )
	{
		int x = ScreenWidth - 150, y = ScreenHeight - 70;
		CZ_DrawSprite( toolIcons[4], x, y, 50, 250, 50 );
		DrawUtils::DrawHudNumberString( x + 130, y + 20, 0, g_czCount, 50, 250, 50 );
		if( g_czHold )
			DrawUtils::DrawHudString( x - 4, y - 14, ScreenWidth, "DETONATOR", 50, 250, 50 );
	}
}

// Draw status icons along the left-hand side of the screen
int CHudStatusIcons::Draw( float flTime )
{
	if (gEngfuncs.IsSpectateOnly())
		return 1;

	CZ_DrawTools( flTime );
	// find starting position to draw from, along right-hand side of screen
	int x = 5;
	int y = ScreenHeight / 2;

	// loop through icon list, and draw any valid icons drawing up from the middle of screen
	for ( int i = 0; i < MAX_ICONSPRITES; i++ )
	{
		if ( m_IconList[i].spr )
		{
			y -= ( m_IconList[i].rc.Height() ) + 5;
			
			if( g_bInBombZone && !strcmp(m_IconList[i].szSpriteName, "c4") && ((int)(flTime * 10) % 2))
				SPR_Set( m_IconList[i].spr, 255, 16, 16 );
			else SPR_Set( m_IconList[i].spr, m_IconList[i].r, m_IconList[i].g, m_IconList[i].b );
			SPR_DrawAdditive( 0, x, y, &m_IconList[i].rc );
		}
	}
	
	return 1;
}

// Message handler for StatusIcon message
// accepts five values:
//		byte   : TRUE = ENABLE icon, FALSE = DISABLE icon
//		string : the sprite name to display
//		byte   : red
//		byte   : green
//		byte   : blue
int CHudStatusIcons::MsgFunc_StatusIcon( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	int ShouldEnable = reader.ReadByte();
	char *pszIconName = reader.ReadString();

	if ( ShouldEnable )
	{
		int r = reader.ReadByte();
		int g = reader.ReadByte();
		int b = reader.ReadByte();
		EnableIcon( pszIconName, r, g, b );
		m_iFlags |= HUD_DRAW;
	}
	else
	{
		DisableIcon( pszIconName );
	}

	return 1;
}

// CZ: HudIcons = 3 shorts. First = icon mask to show, second/third = tools the player owns.
int CHudStatusIcons::MsgFunc_HudIcons( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	int shown = reader.ReadShort();
	int active = reader.ReadShort();
	int owned = reader.ReadShort();
	g_czZoneShown = active & ( owned | 512 );
	extern int g_czToolsOwned;
	g_czToolsOwned = owned;
	gEngfuncs.Con_DPrintf( "HudIcons: shown %d active %d owned %d\n", shown, active, owned );

	static const struct { int bit; const char *name; } icons[] =
	{
		{ 1,    "icon_stealth" },
		{ 4,    "icon_camera" },
		{ 8,    "icon_fibercamera" },
		{ 32,   "icon_radiocontrolledbomb" },
		{ 64,   "icon_blowtorch" },
		{ 512,  "icon_bombdefuse" },
		{ 1024, "icon_hostagezone" },
	};

	for( size_t k = 0; k < sizeof( icons ) / sizeof( icons[0] ); k++ )
	{
		if( shown & icons[k].bit )
		{
			// tools: green while the player is inside their zone, red otherwise
			if( ( ( active & icons[k].bit ) && ( owned & icons[k].bit ) ) || icons[k].bit == 1 || icons[k].bit == 512 || icons[k].bit == 1024 )
				EnableIcon( icons[k].name, 50, 250, 50 );
			else
				EnableIcon( icons[k].name, 250, 60, 60 );
			m_iFlags |= HUD_DRAW;
		}
		else
			DisableIcon( icons[k].name );
	}

	return 1;
}

// add the icon to the icon list, and set it's drawing color
void CHudStatusIcons::EnableIcon( const char *pszIconName, unsigned char red, unsigned char green, unsigned char blue )
{
	// check to see if the sprite is in the current list
	int i;
	for ( i = 0; i < MAX_ICONSPRITES; i++ )
	{
		if ( !stricmp( m_IconList[i].szSpriteName, pszIconName ) )
			break;
	}

	if ( i == MAX_ICONSPRITES )
	{
		// icon not in list, so find an empty slot to add to
		for ( i = 0; i < MAX_ICONSPRITES; i++ )
		{
			if ( !m_IconList[i].spr )
				break;
		}
	}

	// if we've run out of space in the list, overwrite the first icon
	if ( i == MAX_ICONSPRITES )
	{
		i = 0;
	}

	// Load the sprite and add it to the list
	// the sprite must be listed in hud.txt
	int spr_index = -1;
	{
		// czeror hud.txt uses different icon names
		static const char *alias[][2] = { {"rescue","icon_hostagezone"}, {"defuser","icon_bombdefuse"}, {"nvg","icon_nightvision"} };
		for( int a = 0; a < 3; a++ )
			if( !strcmp( pszIconName, alias[a][0] ) && !strcmp( gEngfuncs.pfnGetGameDirectory(), "czeror" ) )
				spr_index = gHUD.GetSpriteIndex( alias[a][1] );
	}
	if( spr_index < 0 )
		spr_index = gHUD.GetSpriteIndex( pszIconName );
	if( spr_index < 0 )
		return; // no such sprite in this game's hud.txt: do not draw garbage
	m_IconList[i].spr = gHUD.GetSprite( spr_index );
	if( !m_IconList[i].spr )
		gEngfuncs.Con_Printf( "StatusIcon: sprite for '%s' is not loaded (index %d)\n", pszIconName, spr_index );
	m_IconList[i].rc = gHUD.GetSpriteRect( spr_index );
	m_IconList[i].r = red;
	m_IconList[i].g = green;
	m_IconList[i].b = blue;
	strncpy( m_IconList[i].szSpriteName, pszIconName, MAX_ICONSPRITENAME_LENGTH );
	m_IconList[i].szSpriteName[MAX_ICONSPRITENAME_LENGTH-1]=0;
}

void CHudStatusIcons::DisableIcon( const char *pszIconName )
{
	// find the sprite is in the current list
	for ( int i = 0; i < MAX_ICONSPRITES; i++ )
	{
		if ( !stricmp( m_IconList[i].szSpriteName, pszIconName ) )
		{
			// clear the item from the list
			memset( &m_IconList[i], 0, sizeof( icon_sprite_t ) );
			return;
		}
	}
}
