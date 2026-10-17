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
#include "com_model.h"
#include "triangleapi.h"
#include "r_efx.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
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
	static const char *toolIcons[] = { "", "icon_camera", "icon_fibercamera", "icon_blowtorch", "icon_radiocontrolledbomb", "icon_radio" };

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
			{ 16,  5, "You are in a radio zone" },
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

	if( g_czTool < 1 || g_czTool > 5 )
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

void CZ_DrawTorchSprites( void );

// Draw status icons along the left-hand side of the screen
int CHudStatusIcons::Draw( float flTime )
{
	if (gEngfuncs.IsSpectateOnly())
		return 1;

	CZ_DrawTools( flTime );
	CZ_DrawTorchSprites();
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
		{ 16,   "icon_radio" },
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


// blowtorch flame: two camera facing sprites (like env_glow) drawn in the transparent pass, after the view model.
// cvars cz_torch1_* and cz_torch2_*: spr (sprite name), fwd/right/up (offset from the nozzle, units), size (half size, 0.1 unit steps), alpha
extern vec3_t g_czTorchPos, v_angles;
extern bool g_czTorchOn, g_czTorchAtk;
extern float g_czTorchSize;

// Blowtorch flame: two sprites (core + halo) projected onto the screen and drawn in the 2D pass,
// so the view model cannot hide them with its depth.
// cz_torch_mode 0 = native-size HUD sprite draw (SPR_DrawAdditive), 1 = scaled 2D quad (cz_torchN_size)
static void CZ_TorchDraw( int pass )
{
	static cvar_t *cv[2][6];
	static cvar_t *cvMode = nullptr;
	static HSPRITE hspr[2] = { 0, 0 };
	static model_t *mspr[2] = { nullptr, nullptr };
	static char sprName[2][64] = { "", "" };
	if( !cv[0][0] )
	{
		static const char *defName[2] = { "sprites/flare3.spr", "sprites/flare2.spr" };
		static const char *defSize[2] = { "10", "30" };
		static const char *defAlpha[2] = { "1.0", "0.2" };
		static const char *defFwd[2] = { "0", "-3" };
		cvMode = gEngfuncs.pfnGetCvarPointer( "cz_torch_mode" ) ? gEngfuncs.pfnGetCvarPointer( "cz_torch_mode" ) : gEngfuncs.pfnRegisterVariable( "cz_torch_mode", "0", 0 );
		for( int k = 0; k < 2; k++ )
		{
			char n[64];
			snprintf( n, sizeof n, "cz_torch%d_spr", k + 1 );   cv[k][0] = gEngfuncs.pfnGetCvarPointer( n ) ? gEngfuncs.pfnGetCvarPointer( n ) : gEngfuncs.pfnRegisterVariable( n, defName[k], 0 );
			snprintf( n, sizeof n, "cz_torch%d_fwd", k + 1 );   cv[k][1] = gEngfuncs.pfnGetCvarPointer( n ) ? gEngfuncs.pfnGetCvarPointer( n ) : gEngfuncs.pfnRegisterVariable( n, defFwd[k], 0 );
			snprintf( n, sizeof n, "cz_torch%d_right", k + 1 ); cv[k][2] = gEngfuncs.pfnGetCvarPointer( n ) ? gEngfuncs.pfnGetCvarPointer( n ) : gEngfuncs.pfnRegisterVariable( n, "0", 0 );
			snprintf( n, sizeof n, "cz_torch%d_up", k + 1 );    cv[k][3] = gEngfuncs.pfnGetCvarPointer( n ) ? gEngfuncs.pfnGetCvarPointer( n ) : gEngfuncs.pfnRegisterVariable( n, "0", 0 );
			snprintf( n, sizeof n, "cz_torch%d_size", k + 1 );  cv[k][4] = gEngfuncs.pfnGetCvarPointer( n ) ? gEngfuncs.pfnGetCvarPointer( n ) : gEngfuncs.pfnRegisterVariable( n, defSize[k], 0 );
			snprintf( n, sizeof n, "cz_torch%d_alpha", k + 1 ); cv[k][5] = gEngfuncs.pfnGetCvarPointer( n ) ? gEngfuncs.pfnGetCvarPointer( n ) : gEngfuncs.pfnRegisterVariable( n, defAlpha[k], 0 );
		}
	}

	extern vec3_t g_czTorchPos;
	extern bool g_czTorchOn, g_czTorchAtk;
	extern float g_czTorchSize;
	if( !g_czTorchOn || g_czTool != 3 || g_czFiber )
		return;

	// mode 0: 2D pass over everything (native size); other modes: camera facing quad drawn BEFORE the view model,
	// so the hands and the torch cover it
	// pass 0 = 2D (mode 0), 1 = TriAPI quad (modes 1, 2), 2 = real sprite entities (mode 3: engine draws them, scalable, behind the hands)
	{
		int m = (int)( cvMode->value + 0.5f );
		int want = ( m <= 0 ) ? 0 : ( m >= 3 ? 2 : 1 );
		if( pass != want )
			return;
	}
	bool worldPass = ( pass == 1 );

	vec3_t fw, rt, up;
	gEngfuncs.pfnAngleVectors( v_angles, fw, rt, up );

	for( int k = 0; k < 2; k++ )
	{
		const char *nm = cv[k][0]->string;
		if( strcmp( nm, sprName[k] ) )
		{
			snprintf( sprName[k], sizeof sprName[k], "%s", nm );
			hspr[k] = SPR_Load( sprName[k] );
			mspr[k] = gEngfuncs.LoadMapSprite( sprName[k] );
			if( hspr[k] )
				gEngfuncs.Con_Printf( "torch sprite %d: %s %dx%d, frames %d\n", k + 1, sprName[k], SPR_Width( hspr[k], 0 ), SPR_Height( hspr[k], 0 ), gEngfuncs.pfnSPR_Frames( hspr[k] ));
			else
				gEngfuncs.Con_Printf( "torch sprite %d: %s not found\n", k + 1, sprName[k] );
		}

		vec3_t c, scr;
		for( int i = 0; i < 3; i++ )
			c[i] = g_czTorchPos[i] + fw[i] * cv[k][1]->value + rt[i] * cv[k][2]->value + up[i] * cv[k][3]->value;
		float al = cv[k][5]->value * ( g_czTorchAtk ? 1.3f : 1.0f );
		if( al > 1.0f ) al = 1.0f;
		if( al < 0.0f ) al = 0.0f;

		if( pass == 2 )
		{
			static cl_entity_t ents[2];
			const model_s *m = hspr[k] ? gEngfuncs.GetSpritePointer( hspr[k] ) : nullptr;
			if( !m )
				continue;
			cl_entity_t *e = &ents[k];
			memset( e, 0, sizeof( *e ));
			e->model = (model_s *)m;
			VectorCopy( c, e->origin );
			VectorCopy( c, e->curstate.origin );
			e->curstate.rendermode = kRenderTransAdd;
			e->curstate.renderamt = (int)( 255 * al );
			e->curstate.rendercolor.r = 255;
			e->curstate.rendercolor.g = 217;
			e->curstate.rendercolor.b = 235;
			e->curstate.renderfx = 0;
			e->curstate.frame = 0;
			int w = SPR_Width( hspr[k], 0 );
			float half = cv[k][4]->value * 0.1f * g_czTorchSize; // units
			e->curstate.scale = ( w > 0 ) ? ( half * 2.0f / (float)w ) : 1.0f;
			gEngfuncs.CL_CreateVisibleEntity( ET_NORMAL, e );
		}
		else if( !worldPass )
		{
			if( gEngfuncs.pTriAPI->WorldToScreen( c, scr ))
				continue; // behind the camera
			float cx = ( 1.0f + scr[0] ) * 0.5f * ScreenWidth;
			float cy = ( 1.0f - scr[1] ) * 0.5f * ScreenHeight;
			if( !hspr[k] )
				continue;
			int w = SPR_Width( hspr[k], 0 ), h = SPR_Height( hspr[k], 0 );
			SPR_Set( hspr[k], (int)( 255 * al ), (int)( 217 * al ), (int)( 235 * al ));
			SPR_DrawAdditive( 0, (int)cx - w / 2, (int)cy - h / 2, nullptr );
		}
		else
		{
			if( !mspr[k] )
				continue;
			// half size in world units: cz_torchN_size is in 0.1 unit steps
			float half = cv[k][4]->value * 0.1f * g_czTorchSize;
			vec3_t a, b, p0, p1, p2, p3;
			for( int i = 0; i < 3; i++ )
			{
				a[i] = rt[i] * half;
				b[i] = up[i] * half;
				p0[i] = c[i] - a[i] + b[i];
				p1[i] = c[i] + a[i] + b[i];
				p2[i] = c[i] + a[i] - b[i];
				p3[i] = c[i] - a[i] - b[i];
			}
			gEngfuncs.pTriAPI->SpriteTexture( mspr[k], 0 );
			gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
			gEngfuncs.pTriAPI->CullFace( TRI_NONE );
			gEngfuncs.pTriAPI->Begin( TRI_QUADS );
			gEngfuncs.pTriAPI->Color4f( 1.0f, 0.85f, 0.92f, al );
			gEngfuncs.pTriAPI->TexCoord2f( 0, 0 ); gEngfuncs.pTriAPI->Vertex3fv( p0 );
			gEngfuncs.pTriAPI->TexCoord2f( 1, 0 ); gEngfuncs.pTriAPI->Vertex3fv( p1 );
			gEngfuncs.pTriAPI->TexCoord2f( 1, 1 ); gEngfuncs.pTriAPI->Vertex3fv( p2 );
			gEngfuncs.pTriAPI->TexCoord2f( 0, 1 ); gEngfuncs.pTriAPI->Vertex3fv( p3 );
			gEngfuncs.pTriAPI->End();
			gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
		}
	}
}

void CZ_DrawTorchSprites( void )
{
	CZ_TorchDraw( 0 );
}

void CZ_TorchEntities( void )
{
	CZ_TorchDraw( 2 );
}

void CZ_DrawTorchWorld( void )
{
	CZ_TorchDraw( 1 );
}
