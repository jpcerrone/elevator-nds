#include <nds.h>
#include <stdio.h>

// BG
#include <floor_b.h>

// Sprites
#include <button.h>

typedef struct Sprite{
	int x;
	int y;
	int frame;
	uint16_t* oamPtr; // TODO check what's the standard (u16 vs uint16_t)
	uint8_t* dataPtr;
} Sprite;

int main(void) {
	powerOn(POWER_ALL_2D);
	videoSetMode(MODE_5_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankD(VRAM_D_SUB_SPRITE);

	//consoleDemoInit();
	int bg3 = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0 /*the 2k offset into vram the tile map will be placed*/,0 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaCopy(floor_bBitmap, bgGetGfxPtr(bg3), floor_bBitmapLen);
	dmaCopy(floor_bPal, BG_PALETTE, floor_bPalLen); 

	// Sprites
	Sprite buttonSprite = {20, 50, 1}; 
	oamInit(&oamSub, SpriteMapping_1D_32, false); // Note: I think this means the first row is 32bits wide 
	buttonSprite.oamPtr = oamAllocateGfx(&oamSub, SpriteSize_32x16, SpriteColorFormat_16Color);
	buttonSprite.dataPtr = (uint8_t*)buttonTiles;	
	dmaCopy(buttonPal, SPRITE_PALETTE_SUB, buttonPalLen);
	//float delta = 0.01666666666;
	while(1) {
		dmaCopy(buttonSprite.dataPtr + buttonSprite.frame*16*16/2, buttonSprite.oamPtr, 16*16); // Here i divide by 2 cuz at 16colors (4bit) each byte of dataPtr contains 2 pixels
		oamSet(&oamSub, 0, buttonSprite.x, buttonSprite.y, 0 /* priority*/, 0, SpriteSize_16x16, SpriteColorFormat_16Color, 
			buttonSprite.oamPtr, -1, false, false, false, false, false);
		swiWaitForVBlank();
		oamUpdate(&oamSub);
		scanKeys();
		int pressed = keysDown();
		if(pressed & KEY_UP){
			bgScroll(bg3, 0,1);
			buttonSprite.frame = 0;
		}
		bgUpdate();
	}

}

