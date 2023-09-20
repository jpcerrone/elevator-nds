#include <nds.h>
#include <stdio.h>
#include <floor_b.h>

int main(void) {
	powerOn(POWER_ALL_2D);
	videoSetMode(MODE_5_2D);
	vramSetBankA(VRAM_A_MAIN_BG);

	//consoleDemoInit();
	int bg3 = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0 /*the 2k offset into vram the tile map will be placed*/,0 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaCopy(floor_bBitmap, bgGetGfxPtr(bg3), floor_bBitmapLen);
	dmaCopy(floor_bPal, BG_PALETTE, floor_bPalLen); 
	
	while(1) {
		swiWaitForVBlank();
		bgScroll(bg3, 0,1);
		bgUpdate();

		scanKeys();
		int pressed = keysDown();
		if(pressed & KEY_START) break;
	}

}

