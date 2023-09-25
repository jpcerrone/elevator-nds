#include <nds.h>
#include <stdio.h>

#include "game.h"
#include "graphics.h"

// BG
#include <floor_b.h>

// Sprites
#include <button_big.h>

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))

typedef struct Vector2i{
union {
        int x;
        int width;
    };
    union {
        int y;
        int height;
    };
} Vector2i;

int main(void) {
	GameState state = {};

	powerOn(POWER_ALL_2D);
	videoSetMode(MODE_5_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankD(VRAM_D_SUB_SPRITE);
	
	consoleDemoInit();

	int bg3 = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0 /*the 2k offset into vram the tile map will be placed*/,0 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaCopy(floor_bBitmap, bgGetGfxPtr(bg3), floor_bBitmapLen);
	dmaCopy(floor_bPal, BG_PALETTE, floor_bPalLen); 

	// Sprites
	
	oamInit(&oamSub, SpriteMapping_1D_64, false); // Note: I think this means the first row is 32bits wide 
	dmaCopy(buttonBigPal, SPRITE_PALETTE_SUB, buttonBigPalLen);

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
		//iprintf("\x1b[6;5HTouch x = %04X, %04X\n", touch.rawx, touch.px);

		// Rendering
		swiWaitForVBlank(); // TODO Check the orer of evth that follows
		scanKeys();
		int pressed = keysDown();
		if(pressed & KEY_UP){
			bgScroll(bg3, 0,1);
		}
		bgUpdate();
		updateAndRender(&input, &state);
		for(int i=0; i < 10; i++){
			drawSprite(&state.sprites[i]);
		}
		oamUpdate(&oamSub);
	}

}

