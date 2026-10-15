# CZ:DS port (branch czds)

Work in progress port of Counter-Strike: Condition Zero Deleted Scenes to Xash3D FWGS,
built on top of cs16-client + ReGameDLL_CS (submodule, branch czds of the fork).
Game assets come from the user's own copy of the game (czeror folder).

Done so far (server, 3rdparty/ReGameDLL_CS/regamedll/dlls/cz):
- .seq sequence parser, trigger_sequence
- trigger_changekeyvalue / hud / freezeplayer / killtarget / objective
- item_generic, monster_npc
