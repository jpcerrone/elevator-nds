#include <nds.h>
#include <stdio.h>

#include "game.h"
#include "graphics.h"
#include "vector2i.h"

#include <maxmod9.h>
#include "soundbank_bin.h"
#include "soundbank.h"

// Sprites
#include <button_big.h>

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))

int main(void) {
	GameState state = {};

	mmInitDefaultMem((mm_addr)soundbank_bin);
		
	mmLoad(MOD_MUSIC);
	mmLoadEffect(SFX_CLICK);
	mmLoadEffect(SFX_ARRIVAL);
	mmLoadEffect(SFX_BRAKE);
	mmLoadEffect(SFX_DOOR_CLOSE);
	mmLoadEffect(SFX_DOOR_OPEN);
	mmLoadEffect(SFX_FAIL);
	mmLoadEffect(SFX_PASSING);

	powerOn(POWER_ALL_2D);
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	//vramSetBankA(VRAM_A_MAIN_SPRITE_0x06400000);
	//vramSetBankB(VRAM_B_MAIN_SPRITE_0x06420000);
	vramSetBankB(VRAM_B_MAIN_SPRITE);
	vramSetBankC(VRAM_C_SUB_BG);
	vramSetBankD(VRAM_D_SUB_SPRITE);
/*
 *
 *vramSetBankA(VRAM_A_MAIN_SPRITE_0x06400000);
vramSetBankB(VRAM_B_MAIN_SPRITE_0x06420000);
vramSetBankC(VRAM_C_MAIN_BG_0x06000000);
 *
 *
 */
	//consoleDemoInit();

		// Sprites
	oamInit(&oamSub, SpriteMapping_1D_128, false); // Why 128? -> https://www.tumblr.com/altik-0/24833858095/nds-development-some-info-on-sprites?redirect_to=%2Faltik-0%2F24833858095%2Fnds-development-some-info-on-sprites&source=blog_view_login_wall
	oamInit(&oamMain, SpriteMapping_1D_128, false); 
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

		updateAndRender(&input, &state);
		// Rendering
		swiWaitForVBlank(); // TODO Check the orer of evth that follows
		//consoleClear();
		
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

