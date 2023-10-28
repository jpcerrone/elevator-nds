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
	videoSetMode(MODE_3_2D);
	videoSetModeSub(MODE_3_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankB(VRAM_B_MAIN_SPRITE);
	vramSetBankC(VRAM_C_SUB_BG);
	vramSetBankD(VRAM_D_SUB_SPRITE);

	// Sprites
	oamInit(&oamSub, SpriteMapping_1D_128, false); // Why 128? -> https://www.tumblr.com/altik-0/24833858095/nds-development-some-info-on-sprites?redirect_to=%2Faltik-0%2F24833858095%2Fnds-development-some-info-on-sprites&source=blog_view_login_wall
	oamInit(&oamMain, SpriteMapping_1D_128, false); 
	bool lastFramePenDown = false;
	while(1) {
		GameInput input = {};

		// Touch controls
		touchPosition touch;
		touchRead(&touch);
		bool penDown = (touch.px != 0) || (touch.py != 0);
		if (state.isInitialized && penDown && !lastFramePenDown){
			struct Vector2i touchVec = {{touch.px}, {touch.py}};
			int radiusSquared = state.buttonSprites[0]->image->width/2 * state.buttonSprites[0]->image->width/2;
			for(int i=0; i < 10; i++){ 
				struct Vector2i buttonMiddle = {{state.buttonSprites[i]->x + state.buttonSprites[i]->image->width/2}, {state.buttonSprites[i]->y + state.buttonSprites[i]->image->height/2}};
				if (distanceSquared(touchVec, buttonMiddle) <= radiusSquared){
						input.buttons[i] = true; 
				}
			}
		}
		lastFramePenDown = penDown;	

		updateAndRender(&input, &state);

		// Rendering
		swiWaitForVBlank(); // TODO Check the orer of evth that follows
		
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

