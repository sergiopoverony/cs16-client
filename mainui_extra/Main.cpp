/*
Copyright (C) 1997-2001 Id Software, Inc.

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


#include "Framework.h"
#include "Action.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "YesNoMessageBox.h"
#include "keydefs.h"
#include "MenuStrings.h"
#include "PlayerIntroduceDialog.h"
#include "gameinfo.h"
#include "AnimatedBanner.h"
#include "MovieBanner.h"

// CZ:DS: the world map window (WorldMap.cpp)
void UI_WorldMap_Menu( void );

#define ART_MINIMIZE_N	"gfx/shell/min_n"
#define ART_MINIMIZE_F	"gfx/shell/min_f"
#define ART_MINIMIZE_D	"gfx/shell/min_d"
#define ART_CLOSEBTN_N	"gfx/shell/cls_n"
#define ART_CLOSEBTN_F	"gfx/shell/cls_f"
#define ART_CLOSEBTN_D	"gfx/shell/cls_d"

class CMenuMain: public CMenuFramework
{
public:
	CMenuMain() : CMenuFramework( "CMenuMain" ) { }

	bool KeyDown( int key ) override;

	void QuitDialogCb();

private:
	void _Init() override;
	void _VidInit( ) override;
	void Think() override;

	void VidInit(bool connected);

	void DisconnectCb();
	void DisconnectDialogCb();
	void HazardCourseDialogCb();
	void HazardCourseCb();

	CMenuAnimatedBanner animatedBanner;
	CMenuMovieBanner movieBanner;

	CMenuPicButton	console;
	CMenuPicButton	resumeGame;
	CMenuPicButton	disconnect;
	CMenuPicButton	newGame;
	CMenuPicButton	hazardCourse;
	CMenuPicButton	configuration;
	CMenuPicButton	saveRestore;
	CMenuPicButton	saveGame;
	CMenuPicButton	worldMap;
	CMenuPicButton	multiPlayer;
	CMenuPicButton	customGame;
	CMenuPicButton	readme;
	CMenuPicButton	previews;
	CMenuPicButton	quit;

	// buttons on top right. Maybe should be drawn if fullscreen == 1?
	CMenuBitmap	minimizeBtn;
	CMenuBitmap	quitButton;

	// quit dialog
	CMenuYesNoMessageBox dialog;

	bool bTrainMap;
	bool bCustomGame;
};

void CMenuMain::QuitDialogCb()
{
	if( CL_IsActive() && EngFuncs::GetCvarFloat( "host_serverstate" ) && EngFuncs::GetCvarFloat( "maxplayers" ) == 1.0f )
		dialog.SetMessage( L( "StringsList_235" ) );
	else
		dialog.SetMessage( L( "GameUI_QuitConfirmationText" ) );

	dialog.onPositive.SetCommand( false, "quit \"menu dialog\"\n" );
	dialog.Show();
}

void CMenuMain::DisconnectCb()
{
	EngFuncs::ClientCmd( false, "disconnect\n" );
	VidInit( false );
	CalcPosition();
	CalcSizes();
	VidInitItems();
}

void CMenuMain::DisconnectDialogCb()
{
	dialog.onPositive = VoidCb( &CMenuMain::DisconnectCb );
	dialog.SetMessage( L( "Really disconnect?" ) );
	dialog.Show();
}

void CMenuMain::HazardCourseDialogCb()
{
	dialog.onPositive = VoidCb( &CMenuMain::HazardCourseCb );;
	dialog.SetMessage( L( "StringsList_234" ) );
	dialog.Show();
}

/*
=================
CMenuMain::Key
=================
*/
bool CMenuMain::KeyDown( int key )
{
	if( UI::Key::IsEscape( key ) )
	{
		if ( CL_IsActive( ))
		{
			if( !dialog.IsVisible() )
				UI_CloseMenu();
		}
		else
		{
			QuitDialogCb( );
		}
		return true;
	}
	return CMenuFramework::KeyDown( key );
}

/*
=================
UI_Main_HazardCourse
=================
*/
void CMenuMain::HazardCourseCb()
{
	if( EngFuncs::GetCvarFloat( "host_serverstate" ) && EngFuncs::GetCvarFloat( "maxplayers" ) > 1 )
		EngFuncs::HostEndGame( "end of the game" );

	EngFuncs::CvarSetValue( "skill", 1.0f );
	EngFuncs::CvarSetValue( "deathmatch", 0.0f );
	EngFuncs::CvarSetValue( "teamplay", 0.0f );
	EngFuncs::CvarSetValue( "pausable", 1.0f ); // singleplayer is always allowing pause
	EngFuncs::CvarSetValue( "coop", 0.0f );
	EngFuncs::CvarSetValue( "maxplayers", 1.0f ); // singleplayer

	EngFuncs::PlayBackgroundTrack( NULL, NULL );

	EngFuncs::ClientCmd( false, "hazardcourse\n" );
}

void CMenuMain::_Init( void )
{
	if( gMenu.m_gameinfo.trainmap[0] && stricmp( gMenu.m_gameinfo.trainmap, gMenu.m_gameinfo.startmap ) != 0 )
		bTrainMap = true;
	else bTrainMap = false;

	if( EngFuncs::GetCvarFloat( "host_allow_changegame" ))
		bCustomGame = true;
	else bCustomGame = false;

	// console
	console.SetNameAndStatus( L( "GameUI_Console" ), NULL );
	console.iFlags |= QMF_NOTIFY;
	// text only (no CS sprites)
	console.SetVisibility( gpGlobals->developer );
	SET_EVENT_MULTI( console.onReleased,
	{
		UI_SetActiveMenu( false );
		EngFuncs::KEY_SetDest( KEY_CONSOLE );
	});

	resumeGame.SetNameAndStatus( L( "GameUI_GameMenu_ResumeGame" ), L( "StringsList_188" ) );
	// text only (no CS sprites)
	resumeGame.iFlags |= QMF_NOTIFY;
	resumeGame.onReleased = UI_CloseMenu;

	disconnect.SetNameAndStatus( L( "GameUI_GameMenu_Disconnect" ), L( "Disconnect from server." ) );
	// text only (no CS sprites)
	disconnect.iFlags |= QMF_NOTIFY;
	disconnect.onReleased = VoidCb( &CMenuMain::DisconnectDialogCb );

	newGame.SetNameAndStatus( L( "GameUI_NewGame" ), L( "StringsList_189" ) );
	// text only (no CS sprites)
	newGame.iFlags |= QMF_NOTIFY;
	newGame.onReleased = UI_NewGame_Menu;

	hazardCourse.SetNameAndStatus( L( "GameUI_TrainingRoom" ), L( "StringsList_190" ) );
	// text only (no CS sprites)
	hazardCourse.iFlags |= QMF_NOTIFY;
	hazardCourse.onReleasedClActive = VoidCb( &CMenuMain::HazardCourseDialogCb );
	hazardCourse.onReleased = VoidCb( &CMenuMain::HazardCourseCb );

	multiPlayer.SetNameAndStatus( L( "GameUI_Multiplayer" ), L( "StringsList_198" ) );
	// text only (no CS sprites)
	multiPlayer.iFlags |= QMF_NOTIFY;
	multiPlayer.onReleased = UI_MultiPlayer_Menu;

	configuration.SetNameAndStatus( L( "GameUI_Options" ), L( "StringsList_193" ) );
	// text only (no CS sprites)
	configuration.iFlags |= QMF_NOTIFY;
	configuration.onReleased = UI_Options_Menu;

	saveRestore.iFlags |= QMF_NOTIFY;

	saveGame.SetNameAndStatus( L( "GameUI_SaveGame" ), L( "StringsList_192" ) );
	// text only (no CS sprites)
	saveGame.iFlags |= QMF_NOTIFY;
	saveGame.onReleased = UI_SaveGame_Menu;

	// no picture: the text of the button is drawn
	worldMap.SetNameAndStatus( L( "CZERO_WorldMap" ), L( "Choose a mission" ) );
	worldMap.iFlags |= QMF_NOTIFY;
	worldMap.onReleased = UI_WorldMap_Menu;

	customGame.SetNameAndStatus( L( "GameUI_ChangeGame" ), L( "StringsList_530" ) );
	// text only (no CS sprites)
	customGame.iFlags |= QMF_NOTIFY;
	customGame.onReleased = UI_CustomGame_Menu;

	// TODO: add readme screen later
	readme.SetNameAndStatus( L("View Readme"), L( "StringsList_194" ) );
	// text only (no CS sprites)
	readme.iFlags |= QMF_NOTIFY;

	previews.SetNameAndStatus( L( "Previews" ), L( "StringsList_400" ) );
	// text only (no CS sprites)
	previews.iFlags |= QMF_NOTIFY;
	SET_EVENT( previews.onReleased, EngFuncs::ShellExecute( MenuStrings[ IDS_MEDIA_PREVIEWURL ], NULL, false ) );

	quit.SetNameAndStatus( L( "GameUI_GameMenu_Quit" ), L( "GameUI_QuitConfirmationText" ) );
	// text only (no CS sprites)
	quit.iFlags |= QMF_NOTIFY;
	quit.onReleased = VoidCb( &CMenuMain::QuitDialogCb );

	quitButton.SetPicture( ART_CLOSEBTN_N, ART_CLOSEBTN_F, ART_CLOSEBTN_D );
	quitButton.iFlags = QMF_MOUSEONLY;
	quitButton.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	quitButton.onReleased = VoidCb( &CMenuMain::QuitDialogCb );

	minimizeBtn.SetPicture( ART_MINIMIZE_N, ART_MINIMIZE_F, ART_MINIMIZE_D );
	minimizeBtn.iFlags = QMF_MOUSEONLY;
	minimizeBtn.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	minimizeBtn.onReleased.SetCommand( false, "minimize\n" );

	// CZ:DS: New Game is never grayed by the startmap of the game info

	if ( gMenu.m_gameinfo.gamemode == GAME_SINGLEPLAYER_ONLY )
		multiPlayer.SetGrayed( true );

	// if ( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY )
	// {
	// 	saveRestore.SetGrayed( true );
	// 	hazardCourse.SetGrayed( true );
	// }

	// too short execute string - not a real command
	if( strlen( MenuStrings[IDS_MEDIA_PREVIEWURL] ) <= 3 )
	{
		previews.SetGrayed( true );
	}

	// server.dll needs for reading savefiles or startup newgame
	if( !EngFuncs::CheckGameDll( ))
	{
		saveRestore.SetGrayed( true );
		hazardCourse.SetGrayed( true );
		newGame.SetGrayed( true );
	}

	if( FBitSet( gMenu.m_gameinfo.flags, GFL_ANIMATED_TITLE ))
	{
		if( animatedBanner.TryLoad())
			AddItem( animatedBanner );
	}
	else
	{
		AddItem( movieBanner );
	}

	dialog.Link( this );

	AddItem( banner );
	AddItem( console );
	AddItem( disconnect );
	AddItem( resumeGame );
	AddItem( newGame );
	AddItem( worldMap );
	AddItem( saveRestore );
	AddItem( saveGame );
	AddItem( configuration );
	AddItem( quit );
	AddItem( minimizeBtn );
	AddItem( quitButton );
}

/*
=================
UI_Main_Init
=================
*/
void CMenuMain::VidInit( bool connected )
{
	int hoffset = ( 70 / 640.0 ) * 1024.0;

	// in original menu Previews is located at specific point
	int previews_voffset = ( 404 / 480.0 ) * 768.0;

	// no visible console button gap
	int ygap = (( 404 - 373 ) / 480.0 ) * 768.0;

	// statically positioned items
	minimizeBtn.SetRect( uiStatic.width - 72, 13, 32, 32 );
	quitButton.SetRect( uiStatic.width - 36, 13, 32, 32 );

	// CZ:DS: the menu of the original GameMenu.res: Resume Game, New Game, World Map, Load Game,
	// Save Game, Disconnect, Options, Quit (the buttons are placed from the bottom to the top)
	bool single = gpGlobals->maxClients < 2;

	quit.SetCoord( hoffset, previews_voffset + ygap );
	int yoffset = previews_voffset;

	configuration.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	if( connected && single )
	{
		saveGame.SetCoord( hoffset, yoffset );
		yoffset -= ygap;
	}

	saveRestore.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	worldMap.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	newGame.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	if( connected )
	{
		resumeGame.SetCoord( hoffset, yoffset );
		yoffset -= ygap;

		disconnect.SetCoord( hoffset, yoffset );
		yoffset -= ygap;
	}

	console.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	resumeGame.SetVisibility( connected );
	disconnect.SetVisibility( connected );
	newGame.SetVisibility( true );
	worldMap.SetVisibility( true );
	saveGame.SetVisibility( connected && single );

	saveRestore.SetNameAndStatus( L( "GameUI_LoadGame" ), L( "StringsList_191" ) );
	// text only (no CS sprites)
	saveRestore.onReleased = UI_LoadGame_Menu;
}

void CMenuMain::_VidInit()
{
	VidInit( CL_IsActive() );
}

void CMenuMain::Think()
{
	if( gpGlobals->developer )
	{
		if( !console.IsVisible( ))
			console.Show();
	}
	else
	{
		if( console.IsVisible( ))
			console.Hide();
	}

	CMenuFramework::Think();
}

ADD_MENU( menu_main, CMenuMain, UI_Main_Menu );

/*
=================
UI_QuitDialog_f

called by the engine when the OS asks to quit (window close button, Cmd+Q on macOS...)
=================
*/
static void UI_QuitDialog_f( void )
{
	// nothing to lose, quit immediately
	if( !CL_IsActive( ))
	{
		EngFuncs::ClientCmd( false, "quit \"menu\"\n" );
		return;
	}

	if( !UI_IsVisible( ))
		UI_Main_Menu();

	menu_main->QuitDialogCb();
}
ADD_COMMAND( menu_quit, UI_QuitDialog_f );
