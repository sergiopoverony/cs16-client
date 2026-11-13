# CZ:DS: small source patches of the menu library (done at configure time, safe to run twice)
# usage: include(patch_mainui.cmake) with MAINUI_DIR set
function(czds_patch file from to)
	file(READ "${MAINUI_DIR}/${file}" content)
	string(FIND "${content}" "${from}" pos)
	if(pos GREATER -1)
		string(REPLACE "${from}" "${to}" content "${content}")
		file(WRITE "${MAINUI_DIR}/${file}" "${content}")
		message(STATUS "CZ:DS patched ${file}")
	endif()
endfunction()

# buttons are drawn as text, not with the sprites of btns_main.bmp (they are Counter-Strike's lettering)
czds_patch("BaseMenu.cpp"
	"uiStatic.renderPicbuttonText = gMenu.m_gameinfo.flags & GFL_RENDER_PICBUTTON_TEXT;"
	"uiStatic.renderPicbuttonText = true; // CZ:DS: always text")

# no banner titles (the sprite titles of the menu pages)
czds_patch("controls/Framework.cpp"
	"void CMenuFramework::CMenuBannerBitmap::Draw( Point pt, Size sz )\n{\n"
	"void CMenuFramework::CMenuBannerBitmap::Draw( Point pt, Size sz )\n{\n\t// CZ:DS: no banner titles\n\tif( true )\n\t\treturn;\n")
