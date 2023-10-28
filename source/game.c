#include <nds.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <fat.h>

#include "vector2i.h"
#include "game.h"
#include "graphics.h"

// Sprites & BG
#include <all_gfx.h>

#include <maxmod9.h>
#include "soundbank_bin.h"
#include "soundbank.h"

/*
#include "platform.h"
#include "intrinsics.h"
*/
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
static const float DELTA = 0.01666666666;
static const struct Vector2i SCREEN_CENTER = { {SCREEN_WIDTH / 2}, {SCREEN_HEIGHT / 2 }};
static const struct Vector2i elevatorGuysOrigin = {{SCREEN_CENTER.x - 50}, {SCREEN_CENTER.y - 57}}; 

void setNextDirection(GameState *state) {
    // Get next destination
    int currentNext = state->currentFloor;
    int currentMin = INT32_MAX;
    for (int i = 0; i < 11; i++) {
        if (state->floorStates[i] == true) {
            int newMin = abs(state->currentFloor - i);
            if (newMin < currentMin) {
                currentMin = newMin;
                currentNext = i;
            }
        }
    }
    if (state->currentDestination != currentNext) {
        state->currentDestination = currentNext;
        int oldDirection = state->direction;
        if (state->currentDestination < state->currentFloor) {
            state->direction = -1;
        }
        else if (state->currentDestination > state->currentFloor) { // Changed logic from gdscript
            state->direction = 1;
        }
        if (oldDirection != state->direction) {
            state->elevatorSpeed = STARTING_SPEED;
        }
        state->moving = true;
    }
}

void spawnFloatingNumber(struct FloatingNumber* floatingNumbers, int floatingNumbersSize, struct Image* font, int value, int floor){
	for(int i=0; i < floatingNumbersSize; i++){
		if(!floatingNumbers[i].active){
			floatingNumbers[i].active = true;
			floatingNumbers[i].value = value;
			floatingNumbers[i].floor = floor;
			floatingNumbers[i].offsetY = 0;
			floatingNumbers[i].startingPosOffset.x = SCREEN_WIDTH/8 + rand()%15; 
			floatingNumbers[i].startingPosOffset.y = SCREEN_HEIGHT/3 - rand()%20; 
			displayNumber(value, floatingNumbers[i].sprites, ARRAY_SIZE(floatingNumbers[i].sprites), font, floatingNumbers[i].startingPosOffset.x, floatingNumbers[i].startingPosOffset.y, 0, 1, true, 7.0);
			break;
		}
	}
}


int getScore(float mood, bool pickingUp) {
    int score = 0;
    switch ((int)(ceil(mood / MOOD_TIME))) {
    case 3: {
        score = pickingUp ? 1000 : 200;
    }break;
    case 2: {
        score = pickingUp ? 500 : 100;
    }break;
    case 1: {
        score = pickingUp ? 200: 50;
    }break;
    }
    return score;
}

bool isElevatorFull(Guy* elevatorSpots[]){
	for (int s = 0; s < ELEVATOR_SPOTS; s++) {
                        if (!elevatorSpots[s]) {
                            return false;
                        }
	}
	return true;
}

// BUG: As this function doesnt really care whether guys are dropped before others are picked up, when having the elevator full, dropping a guy and picking one up at the same floor might somethimes fail.
// Since this happens extremely rarely in the game, it's left unfixed for now.
void pickAndPlaceGuys(GameState* state) {
	for (int i = 0; i < MAX_GUYS_ON_SCREEN; i++) {
		if (state->guys[i].active) {
			if (state->guys[i].onElevator && (state->guys[i].desiredFloor == state->currentFloor)) {

				int score = getScore(state->guys[i].mood, false);
				state->score += score;

				spawnFloatingNumber(state->floatingNumbers, ARRAY_SIZE(state->floatingNumbers), &state->images.numbersFont6px, score, state->currentFloor);
				for(int j=0; j < ARRAY_SIZE(state->elevatorSpots); j++){
					if(state->elevatorSpots[j] == &state->guys[i]){
						state->elevatorSpots[j] = NULL;
						state->elevatorGuySprites[j]->visible = false;
						break;
					}
				}
				state->guys[i].active = false;
				state->guys[i].onElevator = false;
				state->dropOffFloor = state->currentFloor;
				state->dropOffTimer.active = true;
				state->dropOffTimer.time = DROP_OFF_TIME;
			}
			else {
				if (state->guys[i].currentFloor == state->currentFloor && !isElevatorFull(state->elevatorSpots)) {
					int score = getScore(state->guys[i].mood, true);

					state->score += score;
					spawnFloatingNumber(state->floatingNumbers, ARRAY_SIZE(state->floatingNumbers), &state->images.numbersFont6px, score, state->currentFloor);
					state->guys[i].onElevator = true;
					state->guys[i].mood = MOOD_TIME * 3; // 3 to get all 4 possible mood state's ranges [0..3]
					state->fullFloors[state->currentFloor] = false;
					state->guys[i].currentFloor = -1;


					if (state->score >= REQUIRED_SCORE * (state->currentLevel + 1) + (500 * state->currentLevel)) {
						state->currentLevel += 1;
						state->flashTextTimer.active = true;
						state->flashTextTimer.time = FLASH_TIME;
					}

					for (int s = 0; s < ELEVATOR_SPOTS; s++) {
						if (!state->elevatorSpots[s]) {
							iprintf("puto");
							state->elevatorSpots[s] = &state->guys[i];
							state->elevatorGuySprites[s]->visible = true;
							break;
						}
					}
					state->uiGuySprites[state->currentFloor]->visible = false;
					state->arrowSprites[state->currentFloor]->visible = false;

				}
			}

		}
	}
}

bool areAllFloorsSave1Full(bool *fullFloors) {
	bool oneFree = false;
	for (int i = 0; i < 10; i++) {
		if (!fullFloors[i]) {
			if (!oneFree) {
				oneFree = true;
			}
			else {
				return false;
			}
		}
	}
	return true;
}

bool areMaxGuysOnScreen(Guy *guys) {
	for (int i = 0; i < MAX_GUYS_ON_SCREEN; i++) {
		if (!guys[i].active) {
			return false;
		}
	}
	return true;
}

void spawnNewGuy(Guy *guys, bool *fullFloors, int currentFloor) {
    if (areAllFloorsSave1Full(fullFloors) || areMaxGuysOnScreen(guys)) {
        return;
    }
    int randomGuyIdx = rand() % MAX_GUYS_ON_SCREEN;
    int randomCurrent = rand() % 10;
    int randomDest = rand() % 10;

    while (guys[randomGuyIdx].active) { 
        randomGuyIdx = rand() % 10;
    }
    while (fullFloors[randomCurrent] || (randomCurrent == currentFloor)) {
	    randomCurrent = rand() % 10;
    }    
    while (randomDest == randomCurrent) {
        randomDest = rand() % 10;
    }

    guys[randomGuyIdx].active = true;
    guys[randomGuyIdx].mood = MOOD_TIME * 3; // 3 to get all 3 possible mood state's ranges + dead [0..3 + 4]
    guys[randomGuyIdx].currentFloor = randomCurrent;
    guys[randomGuyIdx].desiredFloor = randomDest;
    fullFloors[randomCurrent] = true;
}

void loadMenuGfx(GameState* state){

    	dmaCopy(gameColors, BG_PALETTE, sizeof(uint16_t) * ARRAY_SIZE(gameColors));
	dmaCopy(gameColors, BG_PALETTE_SUB, sizeof(uint16_t) * ARRAY_SIZE(gameColors));
    	
	dmaCopy(gameColors, SPRITE_PALETTE_SUB, sizeof(uint16_t) * ARRAY_SIZE(gameColors));
	dmaCopy(gameColorsInv, SPRITE_PALETTE_SUB + 16, sizeof(uint16_t) * ARRAY_SIZE(gameColorsInv));
	dmaCopy(gameColors, SPRITE_PALETTE, sizeof(uint16_t) * ARRAY_SIZE(gameColors));
	dmaCopy(gameColorsInv, SPRITE_PALETTE + 16, sizeof(uint16_t) * ARRAY_SIZE(gameColorsInv));
	dmaCopy(gameColorsNoTransp, SPRITE_PALETTE + 32, sizeof(uint16_t) * ARRAY_SIZE(gameColorsNoTransp));

    	// Background loading
	state->bg2 = bgInit(2, BgType_Text4bpp, BgSize_T_256x512, 0 /*the 2k offset into vram the tile map will be placed*/,1 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaCopy(menuBGTiles, bgGetGfxPtr(state->bg2), menuBGTilesLen);
	dmaCopy(menuBGMap, bgGetMapPtr(state->bg2),  menuBGMapLen);

	state->bg3 = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 4 /*the 2k offset into vram the tile map will be placed*/,0 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaFillHalfWords( 0x101, bgGetGfxPtr( state->bg3 ), 256 * 192 ); // VRAM can only be accessed with a 16b pointer, so we have to handle two pixels at a time
	bgSetScroll(state->bg3,0, -192);

	state->subBg1 = bgInitSub(1, BgType_Text4bpp, BgSize_T_256x256, 0 /*the 2k offset into vram the tile map will be placed*/,1 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaCopy(bottomScreenBGTiles, bgGetGfxPtr(state->subBg1), bottomScreenBGTilesLen);
	dmaCopy(bottomScreenBGMap, bgGetMapPtr(state->subBg1),  bottomScreenBGMapLen);
	
	state->subBg3 = bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 2 ,0 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaFillHalfWords( 0x0, bgGetGfxPtr( state->subBg3 ), 256 * 256 ); // VRAM can only be accessed with a 16b pointer, so we have to handle two pixels at a time

	bgSetPriority(state->bg3, 0); // Transitions
	bgSetPriority(state->bg2, 1); 
	bgSetPriority(state->subBg1, 2);
	bgSetPriority(state->subBg3, 0);
	bgHide(1);
	for(int i = 0; i < 10; i++){
		state->buttonSprites[i]->visible = true;
		state->buttonSprites[i]->frame = 0;
		state->buttonNumberSprites[i]->visible = true;
		state->buttonNumberSprites[i]->paletteIdx = 1;

	}
	for(int i= 0; i < 6; i++){

		state->scoreSprites[i]->visible = false;
		state->maxScoreSprites[i]->visible = false;
	}
	state->pressAnyButtonSprite[0]->visible = true;
	state->pressAnyButtonSprite[1]->visible = true;
}
void loadGameGfx(GameState* state){
 
	// BG loading

	int bg2 = bgInit(2, BgType_Text4bpp, BgSize_T_256x512, 0 /*the 2k offset into vram the tile map will be placed*/,1 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaCopy(floor_bBGTiles, bgGetGfxPtr(bg2), floor_bBGTilesLen);
	dmaCopy(floor_bBGMap, bgGetMapPtr(bg2),  floor_bBGMapLen);

	int bg1 = bgInit(1, BgType_Text4bpp, BgSize_T_256x256, 2 /*the 2k offset into vram the tile map will be placed*/,2 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaCopy(_topScreenBGTiles, bgGetGfxPtr(bg1), _topScreenBGTilesLen);
	dmaCopy(_topScreenBGMap, bgGetMapPtr(bg1),  _topScreenBGMapLen);

	int bg0 = bgInit(0, BgType_Text4bpp, BgSize_T_256x512, 4 ,3 );
	dmaCopy(frontBGTiles, bgGetGfxPtr(bg0), frontBGTilesLen);
	dmaCopy(frontBGMap, bgGetMapPtr(bg0),  frontBGMapLen);

	//consoleInit(NULL, 1, BgType_Text4bpp, BgSize_T_256x512,31,0,true, true );

	bgSetPriority(bg2, 3);
	bgSetPriority(bg1, 2);
	bgSetPriority(bg0, 1);
	bgSetPriority(state->bg3, 0); // Transitions

	dmaFillHalfWords( 0x101, bgGetGfxPtr( state->bg3 ), 256 * 192 );
	bgSetScroll(state->bg3,0, 0);

	state->doorSpriteBot->visible = true;
	state->doorSpriteTop->visible = true;
	state->levelSprite->visible = true;
	for(int i = 0; i < 10; i++){
		state->buttonSprites[i]->visible = true;
		state->buttonNumberSprites[i]->visible = true;
		state->floorGuySprites[i]->visible = true;
	}
	for(int i = 0; i < ARRAY_SIZE(state->elevatorGuySprites); i++){
		state->elevatorGuySprites[i]->visible = false;
	}
	for(int i= 0; i < 6; i++){

		state->scoreSprites[i]->visible = true;
	}
	for(int i= 0; i < 2; i++){

		state->floorIndicatorSprites[i]->visible = true;
	}
	for(int i = 0; i < ARRAY_SIZE(state->scoreBoardSprites); i++){
		state->scoreBoardSprites[i]->visible = true;
	}
	state->pressAnyButtonSprite[0]->visible = false;
	state->pressAnyButtonSprite[1]->visible = false;


}
void loadScoreGfx(GameState* state){

    	// Background loading
	state->bg2 = bgInit(2, BgType_Text4bpp, BgSize_T_256x512, 0 /*the 2k offset into vram the tile map will be placed*/,1 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaCopy(scoreBGTiles, bgGetGfxPtr(state->bg2), scoreBGTilesLen);
	dmaCopy(scoreBGMap, bgGetMapPtr(state->bg2),  scoreBGMapLen);
	
	dmaFillHalfWords( 0x101, bgGetGfxPtr( state->bg3 ), 256 * 192 );
	bgSetScroll(state->bg3,0, -192);

	bgSetPriority(state->bg2, 2);
	bgHide(1);
	bgHide(0);

	dmaFillHalfWords( 0x101, bgGetGfxPtr( state->subBg3 ), 256 * 192 );

	state->doorSpriteBot->visible = false;
	state->doorSpriteTop->visible = false;
	state->levelSprite->visible = false;
	for(int i = 0; i < 10; i++){
		state->buttonSprites[i]->visible = false;
		state->buttonNumberSprites[i]->visible = false;
		state->uiGuySprites[i]->visible = false;
		state->arrowSprites[i]->visible = false;
		state->floorGuySprites[i]->visible = false;
		state->guys[i].rectangleNumber->visible = false;
		state->guys[i].rectangle->visible = false;

	}
	for(int i = 0; i < ARRAY_SIZE(state->elevatorGuySprites); i++){
		state->elevatorGuySprites[i]->visible = false;	
	}
	state->dropOffGuySprite->visible = false;
	for(int i= 0; i < 2; i++){

		state->floorIndicatorSprites[i]->visible = false;
	}
	for(int i = 0; i < ARRAY_SIZE(state->scoreBoardSprites); i++){
		state->scoreBoardSprites[i]->visible = false;
	}
	displayNumber(state->score, state->scoreSprites, ARRAY_SIZE(state->scoreSprites),  &state->images.numbersFont6px, SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 16, 1, 1, true, 7.0);
	displayNumber(state->maxScore, state->maxScoreSprites, ARRAY_SIZE(state->scoreSprites),  &state->images.numbersFont6px, SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 32, 1, 1, true, 7.0);
}

void initAudioEffect(mm_sound_effect* audioEffect, int id, int volume){
	audioEffect->id = id;
	audioEffect->rate = 1024; // 1024 -> Take input file rate
	audioEffect->handle = 0; // 0 -> Allocate new handle
	audioEffect->volume = volume;
	audioEffect->panning = 127;
}

void initGameState(GameState *state) {
	mmLoad( MOD_MUSIC);
	mmSetModuleVolume( 512 );

	fatInitDefault();
	FILE* saveFile = fopen("score.bin", "rb+"); // NOTE: this method of saving only works for flashcarts
	if(saveFile != NULL){
		if (fread(&state->maxScore, 4, 1, saveFile) == 0)
		{
			state->maxScore = 0;
		};
	}
	else{ // If can't open, try to create it
		saveFile = fopen("score.bin", "wb+");
		state->maxScore = 0;
		fwrite(&state->maxScore, 4, 1, saveFile);
	}
	fclose(saveFile);
	
	initAudioEffect(&state->audioFiles.click, SFX_CLICK, 127);
	initAudioEffect(&state->audioFiles.arrival, SFX_ARRIVAL, 127);
	initAudioEffect(&state->audioFiles.brake, SFX_BRAKE, 127);
	initAudioEffect(&state->audioFiles.doorClose, SFX_DOOR_CLOSE, 127);
	initAudioEffect(&state->audioFiles.doorOpen, SFX_DOOR_OPEN, 127);
	initAudioEffect(&state->audioFiles.fail, SFX_FAIL, 127);
	initAudioEffect(&state->audioFiles.passing, SFX_PASSING, 255);
	
	srand((uint32_t)time(NULL)); // Set random seed
	state->isInitialized = true;
	state->currentScreen = MENU;
/*
    state->transitionToBlackTimer = {};
    state->transitionFromBlackTimer = {};
    state->scoreTimer = {};
    */

	state->flashTextTimer.active = true;
	state->flashTextTimer.time = FLASH_TIME;

	loadMenuGfx(state);

    	memset(state->floatingNumbers, 0, sizeof(state->floatingNumbers));

	// Sprite loading
   	memset(state->spritesSub, 0, sizeof(state->spritesSub)); // CHECK THIS
	memset(state->spritesMain, 0, sizeof(state->spritesMain)); // CHECK THIS

	struct Vector2i doorPos = {{SCREEN_CENTER.x - 68}, {SCREEN_CENTER.y - 51}};
    	state->images.door = loadImage((uint8_t*)doorTiles, 64, 64, 2);
	state->doorSpriteTop = createSprite(&state->images.door, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, doorPos.x, doorPos.y, 0, &oamMain, false, 2);
	state->images.doorBot = loadImage((uint8_t*)doorBotTiles, 64, 64, 2);
	state->doorSpriteBot = createSprite(&state->images.doorBot, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, doorPos.x, doorPos.y +37, 0, &oamMain, false,2);

	state->images.pressAnyButton = loadImage((uint8_t*)pressAnyButtonTiles, 64, 32, 2);
    	state->images.bigButton = loadImage((uint8_t*)button_bigTiles, 32, 32, 2);
	state->images.numbersFont6px = loadImage((uint8_t*)numbersFont6pxTiles, 16, 16, 10);
	state->images.rectangle = loadImage((uint8_t*)rectangleTiles, 16, 16, 1);
	state->images.uiGuy = loadImage((uint8_t*)ui_guyTiles, 16, 16, 4);
	state->images.arrow = loadImage((uint8_t*)arrowTiles, 8, 8, 2);
	state->images.scoreBoard = loadImage((uint8_t*)scoreBoardTiles, 64, 32, 4);

	struct Vector2i pressAnyButtonPos = {{SCREEN_WIDTH/2 - 64}, {SCREEN_HEIGHT/2 + 48}};
	state->pressAnyButtonSprite[0] = createSprite(&state->images.pressAnyButton, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, pressAnyButtonPos.x, pressAnyButtonPos.y , 0, &oamMain, true, 1);
	state->pressAnyButtonSprite[1] = createSprite(&state->images.pressAnyButton, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, pressAnyButtonPos.x + 64, pressAnyButtonPos.y, 1, &oamMain, true, 1);
	state->pressAnyButtonSprite[0]->paletteIdx = 1;
	state->pressAnyButtonSprite[1]->paletteIdx = 1;

	for(int i = 0; i < ARRAY_SIZE(state->scoreBoardSprites); i++){
		state->scoreBoardSprites[i] = createSprite(&state->images.scoreBoard, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, i * 64, SCREEN_HEIGHT - 28, i, &oamMain, false, 1);
	}

	// Can't collapse these two loops since adding a 16x16 sprite after a 32x32 one screws things up
	for(int y=0;y < 10; y++){
		int x = SCREEN_WIDTH/2 - 16;
		x += (y%2 == 0) ? 14 : -14;
		state->buttonSprites[9 - y] = createSprite(&state->images.bigButton, (struct Sprite*)&state->spritesSub, &state->spriteCountSub, x, y * 16 + 8, 0, &oamSub, true, 2);
	}
	for(int y=0;y < 10; y++){
		int x = SCREEN_WIDTH/2 - 16;
		x += (y%2 == 0) ? 14 : -14;
		state->buttonNumberSprites[9 - y] = createSprite(&state->images.numbersFont6px, (struct Sprite*)&state->spritesSub, &state->spriteCountSub, x + 8, y * 16 + 16, 9 - y, &oamSub, true, 1);
		state->buttonNumberSprites[9 - y]->paletteIdx = 1;
		int uiGuyX = x; 
		int arrowX = x; 
		if ((y % 2) == 0){
			uiGuyX += 32;
			arrowX += 44;
		}
		else{
			uiGuyX -= 16;
			arrowX -= 20;
		}
		state->uiGuySprites[9 - y] = createSprite(&state->images.uiGuy, (struct Sprite*)&state->spritesSub, &state->spriteCountSub, uiGuyX, y * 16 + 16, 0, &oamSub, false, 1);
		state->arrowSprites[9 - y] = createSprite(&state->images.arrow, (struct Sprite*)&state->spritesSub, &state->spriteCountSub, arrowX, y * 16 + 24, 0, &oamSub, false, 1);
	}
	state->images.guy = loadImage((uint8_t*)guyTiles, 64, 64, 4);
	for(int i=0; i < ARRAY_SIZE(state->floorGuySprites); i++){
		state->floorGuySprites[i] = createSprite(&state->images.guy, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, 0, 0, 0, &oamMain, false, 1);
	}
	for(int i = ARRAY_SIZE(state->elevatorGuySprites) - 1; i >= 0 ; i--){ // Iterate backgwards to get the last 2 guys to have a higher priority than the rest and appear in front of them
		state->elevatorGuySprites[i] = createSprite(&state->images.guy, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, 0, 0, 0, &oamMain, false, 1);
	
	}
	state->dropOffGuySprite = createSprite(&state->images.guy, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, 0, SCREEN_HEIGHT/2 - 32, 0, &oamMain, false, 1);
	flipSprite(state->dropOffGuySprite);
	for(int i =0; i < ARRAY_SIZE(state->floorIndicatorSprites); i++){
		state->floorIndicatorSprites[i] = createSprite(&state->images.numbersFont6px, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, SCREEN_WIDTH/2 - 8, SCREEN_HEIGHT - 16, 0, &oamMain, false, 1);
	}
	for(int i =0; i < ARRAY_SIZE(state->scoreSprites); i++){
		state->scoreSprites[i] = createSprite(&state->images.numbersFont6px, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, SCREEN_WIDTH/2 - 8, SCREEN_HEIGHT - 16, 0, &oamMain, false, 3);
		state->maxScoreSprites[i] = createSprite(&state->images.numbersFont6px, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, SCREEN_WIDTH/2 - 8, SCREEN_HEIGHT - 16, 0, &oamMain, false, 3);
		state->scoreSprites[i]->paletteIdx = 1;
		state->maxScoreSprites[i]->paletteIdx = 1;
	}
	for(int i=0; i < MAX_GUYS_ON_SCREEN; i++){
		state->guys[i].rectangle = createSprite(&state->images.rectangle, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, 0, 0, 0, &oamMain, false, 2);
		state->guys[i].rectangleNumber = createSprite(&state->images.numbersFont6px, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, 0, SCREEN_HEIGHT/2, 0, &oamMain, false, 1);
		state->guys[i].rectangle->paletteIdx = 1;
	}
	state->levelSprite = createSprite(&state->images.numbersFont6px, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, 0,0 , 0, &oamMain, false, 2);
	state->levelSprite->paletteIdx = 1;
	for(int j =0; j < ARRAY_SIZE(state->floatingNumbers); j++){
		for(int i = 0; i < 4; i++){
			state->floatingNumbers[j].sprites[i] = createSprite(&state->images.numbersFont6px, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, 0, 0, 0, &oamMain, false, 1);
		}
	}	
    }

void resetGame(GameState *state) {
	    mmStart( MOD_MUSIC, MM_PLAY_LOOP );
        state->score = 0;
    memset(state->floorStates, 0, sizeof(state->floorStates));
    state->elevatorPosY = floorsY[10];
    state->currentFloor = 10;
    state->currentDestination = 10;
    state->elevatorSpeed = STARTING_SPEED;
    state->direction = 0;
    state->moving = false;
    state->dropOffFloor = -1;
    state->currentLevel = 0;

    memset(&state->doorTimer, 0, sizeof(state->doorTimer));
    state->spawnTimer.active = true;
    state->spawnTimer.time = 1.5f; // First guy should appear fast

    state->dropOffTimer.active = false;
    state->dropOffTimer.time = 0;
    
    state->flashTextTimer.active = false;
    state->flashTextTimer.time = 0;
    
    state->circleFocusTimer.active = false;
    state->circleFocusTimer.time = 0;

    state->failSoundPlaying = false;

    memset(state->elevatorSpots, 0, sizeof(state->elevatorSpots));
    memset(state->fullFloors, 0, sizeof(state->fullFloors));
    for(int i=0; i < MAX_GUYS_ON_SCREEN; i++){
	state->guys[i].active = false;
    }
    for(int i=0; i < ARRAY_SIZE(state->floatingNumbers); i++){
	    state->floatingNumbers[i].active = false;
	    for(int j= 0; j < ARRAY_SIZE(state->floatingNumbers[i].sprites); j++){
		    state->floatingNumbers[i].sprites[j]->visible = false;
	    }
    }
    loadGameGfx(state);

    
}

void updateAndRender(GameInput* input, GameState* state) {
	if (!state->isInitialized) {
		initGameState(state);
       }
        switch (state->currentScreen) {
        case MENU:{
	    int flashPerSecond = 2; // IMPROVEMNT this doesnt really work for other numbers
	    if (state->flashTextTimer.active){
		state->flashTextTimer.time -= flashPerSecond*DELTA;
	    }
	    if(state->flashTextTimer.time <= 0){
		    state->flashTextTimer.time = FLASH_TIME;
	    }
	    bool drawFlash = ((int)roundf(state->flashTextTimer.time*flashPerSecond)) % 2;
	    state->pressAnyButtonSprite[0]->visible = drawFlash;
	    state->pressAnyButtonSprite[1]->visible = drawFlash;
	    
	    if (state->transitionToBlackTimer.active) {
		if (state->transitionToBlackTimer.time <= 0){
			state->transitionToBlackTimer.active = false;
        		state->transitionFromBlackTimer.time= TRANSITION_TIME;
			state->transitionFromBlackTimer.active = true;
                    	resetGame(state);
			state->currentScreen = GAME;
			
			return;
		}
                state->transitionToBlackTimer.time -= DELTA;
		break; // The actual transition is handled after the switch statement
            }
	    
	    for (int i = 0; i < 10; i++) {
		    if (input->buttons[i]) {
			    state->transitionToBlackTimer.time= TRANSITION_TIME;
			    state->transitionToBlackTimer.active = true;
			    mmEffectEx(&state->audioFiles.click);
			    state->buttonSprites[i]->frame = 1;
			    state->buttonNumberSprites[i]->paletteIdx = 0;
			    return;
		    }
	    }
        }break;        
        case GAME:{ 
		if (state->transitionFromBlackTimer.active) {
			state->transitionFromBlackTimer.time -= DELTA;
			if (state->transitionFromBlackTimer.time <= 0){
				state->transitionFromBlackTimer.active = false;
			}
		}
		/*
		for(int i=0; i < 5; i++)
			iprintf("%d - ", state->elevatorGuySprites[0]->visible);	
		iprintf("\n");	
		*/
            // Timers
	    // Circle Focus
	    	int radius = 20;
		if (state->circleFocusTimer.active){
			uint16_t* bgGfxPtr = state->circleScreen ? bgGetGfxPtr(state->bg3) : bgGetGfxPtr(state->subBg3);
			if (state->circleFocusTimer.time > 2.5) {
				state->circleFocusTimer.time -= DELTA;
				return;
		    } else if (state->circleFocusTimer.time > 2.2) {
			state->circleFocusTimer.time -= DELTA;
			float focusPercentage = (state->circleFocusTimer.time- 2.2f)*1.0f/0.3f; 
			state->levelSprite->visible = false; // We´re out of layers, these numbers are in the front and we need to draw the circle transition above them, so hiding them solves the problem.
			if (state->circleScreen){ // TODO rename for circleTopScreen or sth
				for(int i=0; i < ARRAY_SIZE(state->scoreSprites); i++){
					state->scoreSprites[i]->visible = false;
				}
				for(int i=0; i < ARRAY_SIZE(state->floorIndicatorSprites); i++){
					state->floorIndicatorSprites[i]->visible = false;
				}
			}
			drawFocusCircle(state->circleSpot, (int)(focusPercentage*SCREEN_HEIGHT/2 + (1 - focusPercentage)*radius), bgGfxPtr);
			return;

			}
			else if (state->circleFocusTimer.time > 1.4) {
				state->circleFocusTimer.time -= DELTA;
				if (state->circleFocusTimer.time < 1.8 && !state->failSoundPlaying){
					mmEffectEx(&state->audioFiles.fail);
					state->failSoundPlaying = true;
				}
				return;
			}
			else if (state->circleFocusTimer.time > 1.0) {
				state->circleFocusTimer.time -= DELTA;
				float focusPercentage = (state->circleFocusTimer.time-1.0f)/0.4f; 
				drawFocusCircle(state->circleSpot, (int)(focusPercentage*radius), bgGfxPtr);
				return;
			}
			else if (state->circleFocusTimer.time > 0){
				state->circleFocusTimer.time -= DELTA;
				return;
			}
			else if (state->circleFocusTimer.time < 0) {
				//state->transitionToBlackTimer.time= TRANSITION_TIME;
				//state->transitionToBlackTimer.active = true;
				state->scoreTimer.active = true;
				state->scoreTimer.time = SCORE_TIME;
				state->currentScreen = SCORE;
				state->circleFocusTimer.active = false;
				loadScoreGfx(state);
				return;

			}
		}	    
		
            // Doors
            if (state->doorTimer.active) {
                state->doorTimer.time -= DELTA;
		if (state->doorTimer.time < 0) {
			pickAndPlaceGuys(state);
			state->doorTimer.active = 0;
			state->doorTimer.time = 0;
			    mmEffectEx(&state->audioFiles.doorClose);
		    }
            }
            
            // Mood
            if (!(state->doorTimer.active)) { // Don't update patience when opening doors
                for (int i = 0; i < MAX_GUYS_ON_SCREEN; i++) {
                    if (state->guys[i].active) {
                        state->guys[i].mood -= DELTA;
                        if (state->guys[i].mood <= 0.0) {
				if (state->score > state->maxScore) {
					state->maxScore = state->score;
					FILE* saveFile = fopen("score.bin", "wb");
					fwrite(&state->maxScore, 4, 1, saveFile);
					fclose(saveFile);
				}
				mmEffectEx(&state->audioFiles.brake);
				mmStop();
				 
				state->circleFocusTimer.active = true;
				state->circleFocusTimer.time = CIRCLE_TIME;
				dmaFillHalfWords( 0x0,  bgGetGfxPtr( state->bg3 ), 256 * 192 );
				bgSetScroll(state->bg3, 0, 0);
				if (state->guys[i].onElevator){
					state->circleScreen = 1;
					struct Vector2i elevatorPos = {{0}, {0}}; // TODO rename all of these
					for(int j=0; j < ARRAY_SIZE(state->elevatorSpots); j++){
						if(state->elevatorSpots[j] == &state->guys[i]){
							elevatorPos = elevatorSpotsPos[j];
							break;
						}
					}
					state->circleSpot.x = elevatorGuysOrigin.x - elevatorPos.x + 34;
					state->circleSpot.y =  elevatorGuysOrigin.y -  elevatorPos.y + 24;
				} else{
					state->circleScreen = 0;
					state->circleSpot.x = (state->guys[i].currentFloor % 2) ? (SCREEN_WIDTH/2 + 36) : (SCREEN_WIDTH/2 - 36);
					state->circleSpot.y = (9 - state->guys[i].currentFloor)*16 + 24;

				}
                    return;
                        }
                    }
                }
            }
	    
#ifndef DONTSPAWN
            // Spawn
            state->spawnTimer.time -= DELTA;
            if (state->spawnTimer.time <= 0) {
                state->spawnTimer.time = SPAWN_TIMES[state->currentLevel];
                spawnNewGuy(state->guys, state->fullFloors, state->currentFloor);
            }
#endif
	    
            // Drop Off
            if (state->dropOffTimer.active) {
                state->dropOffTimer.time -= DELTA;
            }
            if (state->dropOffTimer.time < 0) {
                state->dropOffTimer.active = false;
                state->dropOffTimer.time = 0;
            }

            // Update floor states based on input
	    for (int i = 0; i < 10; i++) {
		    if (input->buttons[i]) {
			    if (!state->moving && i == state->currentFloor) {
				    continue;
			    }
			    mmEffectEx(&state->audioFiles.click);
			    state->floorStates[i] = !state->floorStates[i];
		    }
	    }
	
            // Move and calculate getting to floors
            if (!(state->doorTimer.active)) {
                if (state->moving) {
                    state->elevatorSpeed *= 1 + DELTA / 2;
                    if (state->direction == 1) {
                        state->elevatorPosY += (int)((float)state->elevatorSpeed * DELTA);
                    }
                    else if (state->direction == -1) {
                        state->elevatorPosY -= (int)((float)state->elevatorSpeed * DELTA);
                    }
                    if (state->direction == -1) {
                        if (state->elevatorPosY < floorsY[state->currentFloor - 1]) {
                            state->currentFloor += state->direction;
                            setNextDirection(state);
                            if (state->currentFloor == state->currentDestination) {
                                state->elevatorPosY = floorsY[state->currentFloor]; // Correct elevator position
                                state->moving = false;
                                state->direction = 0; // Not strictly needed I think
                                state->floorStates[state->currentDestination] = false;
                                state->doorTimer.active = true;
                                state->doorTimer.time = DOOR_TIME;
			    mmEffectEx(&state->audioFiles.arrival);
			    mmEffectEx(&state->audioFiles.doorOpen);
                            }
			    else{
				    mmEffectEx(&state->audioFiles.passing);
			    }
                        }
                    }
                    else if (state->direction == 1) {
                        if (state->elevatorPosY > floorsY[state->currentFloor + 1]) {
                            state->currentFloor += state->direction;
                            setNextDirection(state);
                            if (state->currentFloor == state->currentDestination) {
                                state->elevatorPosY = floorsY[state->currentFloor]; // Correct elevator position
                                state->moving = false;
                                state->direction = 0; // Not strictly needed I think
                                state->floorStates[state->currentDestination] = false;
                                state->doorTimer.active = true;
                                state->doorTimer.time = DOOR_TIME;
                            	mmEffectEx(&state->audioFiles.arrival);
				mmEffectEx(&state->audioFiles.doorOpen);
				}
			    else{
				    mmEffectEx(&state->audioFiles.passing);
			    }
                        }
                    }
                }
                else {
                    setNextDirection(state);
                }
            }

	    // Update button's gfx
	    for(int i=0; i < 10; i++){	
		state->buttonSprites[i]->frame = state->floorStates[i];
		state->buttonNumberSprites[i]->paletteIdx = !state->floorStates[i];
	    }
	    
            // Draw elevator stuff
            int floorYOffset = state->elevatorPosY % (FLOOR_SEPARATION); 
            if (floorYOffset > FLOOR_SEPARATION/2) {
                floorYOffset = (FLOOR_SEPARATION - floorYOffset) * -1; // Hack to handle negative mod operation.
            }
	    bgSetScroll(2, 0,-floorYOffset); // Scroll BG2 layer
	    bgSetScroll(0, 0,-floorYOffset); // Scroll BG0 layer
	    int doorFrame = (state->doorTimer.active) ? 0 : 1;

            state->doorSpriteTop->frame = doorFrame; 
	    state->doorSpriteBot->frame = doorFrame; 
	                       
            // Display guys
	    struct Vector2i waitingGuyPos = { {10}, {16 + floorYOffset + 55 }};
	    int yLimitBottom = state->elevatorPosY - FLOOR_SEPARATION / 2 + 64;
	    int yLimitTop = state->elevatorPosY + FLOOR_SEPARATION / 2 - 64;
	    for(int i = 0;  i < MAX_GUYS_ON_SCREEN; i++){
		    state->floorGuySprites[i]->visible = false;
		    state->guys[i].rectangleNumber->visible = false;
		    state->guys[i].rectangle->visible = false;
	    }
		state->dropOffGuySprite->visible = false;
	    if (state->dropOffTimer.active) {
                if ((state->dropOffFloor * FLOOR_SEPARATION >= yLimitBottom ) && (state->dropOffFloor * FLOOR_SEPARATION <= yLimitTop)) {
			state->dropOffGuySprite->visible = true;
			state->dropOffGuySprite->x = waitingGuyPos.x;
			state->dropOffGuySprite->y = waitingGuyPos.y;
                }
            }

	    // All of this is because there are no more layers and I need to draw some guys on top of the others by drawing them in the right order
	    for(int i= 0; i < ARRAY_SIZE(state->elevatorSpots); i++){
		if(state->elevatorSpots[i] != NULL){
			int mood = 3 - ceil(state->elevatorSpots[i]->mood / MOOD_TIME);
			state->elevatorGuySprites[i]->visible = true;
			state->elevatorGuySprites[i]->x = elevatorGuysOrigin.x - elevatorSpotsPos[i].x;
			state->elevatorGuySprites[i]->y = elevatorGuysOrigin.y - elevatorSpotsPos[i].y;
			state->elevatorGuySprites[i]->priority = 2;
			state->elevatorGuySprites[i]->frame = mood;
			state->elevatorSpots[i]->rectangleNumber->visible = true; // NOTE: this could be moved so that it's only calcuated in pickUpAndPlaceGuys() if it ever becaomes a perf problem
			state->elevatorSpots[i]->rectangleNumber->frame = state->elevatorSpots[i]->desiredFloor;
			state->elevatorSpots[i]->rectangleNumber->x = state->elevatorGuySprites[i]->x + 32;
			state->elevatorSpots[i]->rectangleNumber->y = state->elevatorGuySprites[i]->y;
			state->elevatorSpots[i]->rectangle->visible = true;
			state->elevatorSpots[i]->rectangle->x = state->elevatorGuySprites[i]->x + 32;
			state->elevatorSpots[i]->rectangle->y = state->elevatorGuySprites[i]->y;

		}
	    }

            for (int j = 0; j < MAX_GUYS_ON_SCREEN; j++) {
		if (state->guys[j].active) {
                    int mood = 3 - ceil(state->guys[j].mood / MOOD_TIME);
                    if (state->guys[j].onElevator) {
			    /*
			state->guySprites[j]->visible = true;
			state->guySprites[j]->x = elevatorGuysOrigin.x - elevatorSpotsPos[state->guys[j].elevatorSpot].x;
			state->guySprites[j]->y = elevatorGuysOrigin.y - elevatorSpotsPos[state->guys[j].elevatorSpot].y;
			//state->guySprites[j]->priority = (state->guys[j].elevatorSpot < 2) ? 2 : 2;
			state->guys[j].rectangleNumber->visible = true; // NOTE: this could be moved so that it's only calcuated in pickUpAndPlaceGuys() if it ever becaomes a perf problem
			state->guys[j].rectangleNumber->frame = state->guys[j].desiredFloor;
			state->guys[j].rectangleNumber->x = state->guySprites[j]->x + 32;
			state->guys[j].rectangleNumber->y = state->guySprites[j]->y;
			state->guys[j].rectangle->visible = true;
			state->guys[j].rectangle->x = state->guySprites[j]->x + 32;
			state->guys[j].rectangle->y = state->guySprites[j]->y;
				*/
                    }
                    else {
		    	state->floorGuySprites[j]->frame = mood;
			if ((state->guys[j].currentFloor * FLOOR_SEPARATION >= yLimitBottom) && // If they're on the floor
				(state->guys[j].currentFloor * FLOOR_SEPARATION <= yLimitTop)) {
				state->floorGuySprites[j]->visible = true;
				state->floorGuySprites[j]->x = waitingGuyPos.x;
				state->floorGuySprites[j]->y = waitingGuyPos.y;
				state->guys[j].rectangleNumber->visible = true; // NOTE: this could be moved so that it's only calcuated in pickUpAndPlaceGuys() if it ever becaomes a perf problem
				state->guys[j].rectangleNumber->frame = state->guys[j].desiredFloor;
				state->guys[j].rectangleNumber->x = waitingGuyPos.x + 32;
				state->guys[j].rectangleNumber->y = waitingGuyPos.y;
			}
			// Draw UI guys
			state->uiGuySprites[state->guys[j].currentFloor]->visible = true;
			state->uiGuySprites[state->guys[j].currentFloor]->frame = mood;
			state->arrowSprites[state->guys[j].currentFloor]->visible = true;
                        int arrowFrame;
                        if (state->guys[j].currentFloor < state->guys[j].desiredFloor) {
                            arrowFrame = 1;
                        }
                        else {
                            arrowFrame = 0;
                        }
			state->arrowSprites[state->guys[j].currentFloor]->frame = arrowFrame;
                }
                }
            }

             // -- Score
	    displayNumber(state->score, state->scoreSprites, ARRAY_SIZE(state->scoreSprites),  &state->images.numbersFont6px, 44, SCREEN_HEIGHT - 20, 0, 1, false, 7.0);
    
	    // -- Current Floor
	    displayNumber(state->currentFloor, state->floorIndicatorSprites, ARRAY_SIZE(state->floorIndicatorSprites),  &state->images.numbersFont6px, SCREEN_WIDTH/2.0f + 1, SCREEN_HEIGHT - 16 -4.0f, 0, 1, true, 7.0);
	    // Draw floating numbers
	    int floatingSpeed = 40;
	    for(int i=0; i < ARRAY_SIZE(state->floatingNumbers); i++){
		    if(state->floatingNumbers[i].active){
			    if ((state->floatingNumbers[i].floor * FLOOR_SEPARATION + state->floatingNumbers[i].offsetY >= yLimitBottom) && (state->floatingNumbers[i].floor * FLOOR_SEPARATION + state->floatingNumbers[i].offsetY <= yLimitTop)) {
				    for(int j=0; j < ARRAY_SIZE(state->floatingNumbers[i].sprites); j++){
					    state->floatingNumbers[i].sprites[j]->y =(float)state->floatingNumbers[i].startingPosOffset.y - state->floatingNumbers[i].offsetY + floorYOffset;
				    }
			    }
			    else{
				    state->floatingNumbers[i].active = false;
				    for(int j=0; j < ARRAY_SIZE(state->floatingNumbers[i].sprites); j++){
					    state->floatingNumbers[i].sprites[j]->visible = false;				    }

			    }
			    state->floatingNumbers[i].offsetY += DELTA*floatingSpeed;
		    }
	    }
            // --Level
	    int flashesPerSec = 2; 
	    if (state->flashTextTimer.active){
		state->flashTextTimer.time -= DELTA;
	      if (state->flashTextTimer.time < 0){
			state->flashTextTimer.active = false;
			state->flashTextTimer.time = 0;
	      }
	    }
	    if ((int)(state->flashTextTimer.time * flashesPerSec) % 2 || !state->flashTextTimer.active){
		    state->levelSprite->visible = true;
		    displayNumber(state->currentLevel, &state->levelSprite, 1,  &state->images.numbersFont6px, SCREEN_WIDTH - 19, SCREEN_HEIGHT - 20, 0, 1, false, 7.0);
	    }
	    else{
		state->levelSprite->visible = false;
	    }
	    
	    //iprintf("[a%d: b:%d c:%d d:%d e:%d]\n ",  state->elevatorSpots[0], state->elevatorSpots[1],state->elevatorSpots[2],state->elevatorSpots[3],state->elevatorSpots[4]);

	    // Debug stuff
#if 0
            for (int i = 0; i < MAX_GUYS_ON_SCREEN; i++) {
                if (state->guys[i].active) { 
			iprintf("[g%d: c:%d d:%d e:%d]\n ", i, state->guys[i].currentFloor, state->guys[i].desiredFloor, state->guys[i].elevatorSpot + 1);
                }
            }
            #endif

#if 1
	//iprintf("mov%d", state->moving);
	    //iprintf("y: %d | curFl: %d | curDes: %d | spd: %f", state->elevatorPosY, state->currentFloor, state->currentDestination, state->elevatorSpeed);
	    //iprintf("y: %d | curFl: %d | curDes: %d | spd: %f | dir:%d | mov:%d", state->elevatorPosY, state->currentFloor, state->currentDestination, state->elevatorSpeed, state->direction, state->moving);
#endif
	    
        }break;        
	case SCORE:{
			   
			   if (state->scoreTimer.active) {
				   state->scoreTimer.time -= DELTA;
				   if(state->scoreTimer.time < 0){
					   state->scoreTimer.active = false;
					   state->scoreTimer.time = 0;
					   state->transitionToBlackTimer.active = true;
					   state->transitionToBlackTimer.time = TRANSITION_TIME;

				   }
			   }
			   else if (state->transitionToBlackTimer.active) {
				   state->transitionToBlackTimer.time -= DELTA;
				   if(state->transitionToBlackTimer.time < 0){
					   state->transitionToBlackTimer.active = false;
					   state->transitionToBlackTimer.time = 0;
				   }
			   }
			   else {
				   state->transitionToBlackTimer.active = false;
				   state->flashTextTimer.active = true;
				   state->flashTextTimer.time = FLASH_TIME;
				   loadMenuGfx(state);
				   state->currentScreen = MENU;
			   } 
			   
		   }break;
	}
	// Transitions         
           if (state->transitionToBlackTimer.active) {
		bgSetScroll(state->bg3, 0, -SCREEN_HEIGHT + (int)(SCREEN_HEIGHT* (1-state->transitionToBlackTimer.time)));
            }
	   
	
	if (state->transitionFromBlackTimer.active) {
		bgSetScroll(state->bg3, 0,  (int)(SCREEN_HEIGHT* (1 - state->transitionFromBlackTimer.time)));
	}
}

