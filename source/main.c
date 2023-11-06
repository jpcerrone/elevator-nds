#include <nds.h>
#include <stdio.h>

#include "game.h"
#include "graphics.h"
#include "vector2i.h"

#include <maxmod9.h>
#include "soundbank_bin.h"
#include "soundbank.h"

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))

int main(void) {
	struct GameState state = {};
	initGameState(&state);

	bool lastFramePenDown = false;
	while(1) {
		struct GameInput input = {};

		// Touch controls
		touchPosition touch;
		touchRead(&touch);
		bool penDown = (touch.px != 0) || (touch.py != 0);
		if (penDown && !lastFramePenDown){
			struct Vector2i touchVec = {{touch.px}, {touch.py}};
			int radiusSquared = state.sprites.button[0]->image->width/2 * state.sprites.button[0]->image->width/2;
			for(int i=0; i < 10; i++){ 
				struct Vector2i buttonMiddle = {{state.sprites.button[i]->x + state.sprites.button[i]->image->width/2}, {state.sprites.button[i]->y + state.sprites.button[i]->image->height/2}};
				if (distanceSquared(touchVec, buttonMiddle) <= radiusSquared){
						input.buttons[i] = true; 
				}
			}
		}
		lastFramePenDown = penDown;	

		// Rendering
		updateAndRender(&input, &state);
		swiWaitForVBlank();
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

