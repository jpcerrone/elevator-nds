#include <nds.h>
#include <stdio.h>

#include "game.h"
#include "graphics.h"
#include "vector2i.c"

// BG
#include <floor_b.c>
#include <floor_b.h>
#include <topScreen.h>

// Sprites
#include <button_big.h>

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))

int main(void) {
	GameState state = {};

	powerOn(POWER_ALL_2D);
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankB(VRAM_B_MAIN_SPRITE);
	vramSetBankD(VRAM_D_SUB_SPRITE);
	
	consoleDemoInit();

	dmaCopy(gameColors, BG_PALETTE, sizeof(uint16_t) * ARRAY_SIZE(gameColors));

	int bg3 = bgInit(3, BgType_Text4bpp, BgSize_T_256x512, 0 /*the 2k offset into vram the tile map will be placed*/,1 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaCopy(floor_bTiles, bgGetGfxPtr(bg3), floor_bTilesLen);
	dmaCopy(floor_bMap, bgGetMapPtr(bg3),  floor_bMapLen);

	int bg2 = bgInit(2, BgType_Text4bpp, BgSize_T_256x256, 2 /*the 2k offset into vram the tile map will be placed*/,2 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaCopy(topScreenTiles, bgGetGfxPtr(bg2), topScreenTilesLen);
	dmaCopy(topScreenMap, bgGetMapPtr(bg2),  topScreenMapLen);

	bgSetPriority(bg3, 3);
	bgSetPriority(bg2, 2);

	// Sprites
	oamInit(&oamSub, SpriteMapping_1D_128, false); // Why 128? -> https://www.tumblr.com/altik-0/24833858095/nds-development-some-info-on-sprites?redirect_to=%2Faltik-0%2F24833858095%2Fnds-development-some-info-on-sprites&source=blog_view_login_wall
	oamInit(&oamMain, SpriteMapping_1D_128, false); 

	dmaCopy(gameColors, SPRITE_PALETTE_SUB, sizeof(uint16_t) * ARRAY_SIZE(gameColors)); // TODO move to game code
	dmaCopy(gameColors, SPRITE_PALETTE, sizeof(uint16_t) * ARRAY_SIZE(gameColors));
	dmaCopy(gameColorsInv, SPRITE_PALETTE + 16, sizeof(uint16_t) * ARRAY_SIZE(gameColorsInv));
//Backdrop Color
	//float delta = 0.01666666666;
	touchPosition touch;
	bool lastFramePenDown = false;
	while(1) {
		GameInput input = {};

		// Touch controls
		touchRead(&touch);
		bool penDown = (touch.px != 0) || (touch.py != 0);
		
		if (penDown && !lastFramePenDown){
			for(int i=0; i < 10; i++){ // TODO: This could be done with a radius for more circular precision
				if ((touch.px >= state.buttonSprites[i]->x) && (touch.px <= state.buttonSprites[i]->x + state.buttonSprites[i]->image->width)){
					if ((touch.py >= state.buttonSprites[i]->y) && (touch.py <= state.buttonSprites[i]->y + state.buttonSprites[i]->image->height)){
						input.buttons[i] = true; 
					}
				}
			}
		}
		
		lastFramePenDown = penDown;	

		// Rendering
		swiWaitForVBlank(); // TODO Check the orer of evth that follows
		//consoleClear();
		updateAndRender(&input, &state);
		
		for(int i=0; i < state.spriteCountMain; i++){
			drawSprite(&state.spritesMain[i]);
		}
		for(int i=0; i < state.spriteCountSub; i++){
			drawSprite(&state.spritesSub[i]);
		}
		
		bgUpdate();
		oamUpdate(&oamSub);
		oamUpdate(&oamMain);
	}

}

