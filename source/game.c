#include <nds.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <fat.h>

#include "vector2i.h"
#include "game.h"
#include "graphics.h"

// BG
#include <floor_b.c>
#include <floor_b.h>
#include <topScreen.h>
#include <bottomScreen.h>
#include <menu.h>
#include <score.h>

// Sprites
#include <button_big.h>
#include <door.h>
#include <doorBot.h>
#include <guy.h>
#include <numbersFont6px.h>
#include <rectangle.h>
#include <ui-guy.h>
#include <arrow.h>
#include <pressAnyButton.h>

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

bool isElevatorFull(bool elevatorSpots[]){
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
				state->elevatorSpots[state->guys[i].elevatorSpot] = false;
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
							state->guys[i].elevatorSpot = s;
							state->elevatorSpots[s] = true;
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

    	dmaCopy(gameColorsNoTransp, BG_PALETTE, sizeof(uint16_t) * ARRAY_SIZE(gameColorsNoTransp));
	dmaCopy(gameColorsNoTransp, BG_PALETTE_SUB, sizeof(uint16_t) * ARRAY_SIZE(gameColorsNoTransp));
    	
	dmaCopy(gameColors, SPRITE_PALETTE_SUB, sizeof(uint16_t) * ARRAY_SIZE(gameColors));
	dmaCopy(gameColorsInv, SPRITE_PALETTE_SUB + 16, sizeof(uint16_t) * ARRAY_SIZE(gameColorsInv));
	dmaCopy(gameColors, SPRITE_PALETTE, sizeof(uint16_t) * ARRAY_SIZE(gameColors));
	dmaCopy(gameColorsInv, SPRITE_PALETTE + 16, sizeof(uint16_t) * ARRAY_SIZE(gameColorsInv));
	dmaCopy(gameColorsNoTransp, SPRITE_PALETTE + 32, sizeof(uint16_t) * ARRAY_SIZE(gameColorsNoTransp));

    	// Background loading
	state->bg2 = bgInit(2, BgType_Text4bpp, BgSize_T_256x512, 0 /*the 2k offset into vram the tile map will be placed*/,1 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaCopy(menuTiles, bgGetGfxPtr(state->bg2), menuTilesLen);
	dmaCopy(menuMap, bgGetMapPtr(state->bg2),  menuMapLen);

	state->bg3 = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 4 /*the 2k offset into vram the tile map will be placed*/,0 /* the 16k offset into vram the tile graphics data will be placed*/);
	// VRAM can only be accessed with a 16b pointer, so we have to handle two pixels at a time
	dmaFillHalfWords( 0x101, bgGetGfxPtr( state->bg3 ), 256 * 192 );
	bgSetScroll(state->bg3,0, -192);

	int bg1Sub = bgInitSub(1, BgType_Text4bpp, BgSize_T_256x256, 0 /*the 2k offset into vram the tile map will be placed*/,1 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaCopy(bottomScreenTiles, bgGetGfxPtr(bg1Sub), bottomScreenTilesLen);
	dmaCopy(bottomScreenMap, bgGetMapPtr(bg1Sub),  bottomScreenMapLen);

	bgSetPriority(state->bg3, 0); // Transitions
	bgSetPriority(state->bg2, 1); 
	bgSetPriority(bg1Sub, 2);
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
    	dmaCopy(gameColors, BG_PALETTE, sizeof(uint16_t) * ARRAY_SIZE(gameColors));
	dmaCopy(gameColorsNoTransp, BG_PALETTE_SUB, sizeof(uint16_t) * ARRAY_SIZE(gameColorsNoTransp));

	int bg2 = bgInit(2, BgType_Text4bpp, BgSize_T_256x512, 0 /*the 2k offset into vram the tile map will be placed*/,1 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaCopy(floor_bTiles, bgGetGfxPtr(bg2), floor_bTilesLen);
	dmaCopy(floor_bMap, bgGetMapPtr(bg2),  floor_bMapLen);

	int bg1 = bgInit(1, BgType_Text4bpp, BgSize_T_256x256, 2 /*the 2k offset into vram the tile map will be placed*/,2 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaCopy(topScreenTiles, bgGetGfxPtr(bg1), topScreenTilesLen);
	dmaCopy(topScreenMap, bgGetMapPtr(bg1),  topScreenMapLen);

	bgSetPriority(bg2, 3);
	bgSetPriority(bg1, 2);
	bgSetPriority(state->bg3, 0); // Transitions

	dmaFillHalfWords( 0x202, bgGetGfxPtr( state->bg3 ), 256 * 192 );
	bgSetScroll(state->bg3,0, 0);

	state->doorSpriteBot->visible = true;
	state->doorSpriteTop->visible = true;
	state->levelSprite->visible = true;
	for(int i = 0; i < 10; i++){
		state->buttonSprites[i]->visible = true;
		state->buttonNumberSprites[i]->visible = true;
		state->guySprites[i]->visible = true;
	}
	for(int i= 0; i < 6; i++){

		state->scoreSprites[i]->visible = true;
	}
	for(int i= 0; i < 2; i++){

		state->floorIndicatorSprites[i]->visible = true;
	}

	state->pressAnyButtonSprite[0]->visible = false;
	state->pressAnyButtonSprite[1]->visible = false;


}
void loadScoreGfx(GameState* state){

    	dmaCopy(gameColorsNoTransp, BG_PALETTE, sizeof(uint16_t) * ARRAY_SIZE(gameColorsNoTransp));
	dmaCopy(gameColorsNoTransp, BG_PALETTE_SUB, sizeof(uint16_t) * ARRAY_SIZE(gameColorsNoTransp));
    	
    	// Background loading
	state->bg2 = bgInit(2, BgType_Text4bpp, BgSize_T_256x512, 0 /*the 2k offset into vram the tile map will be placed*/,1 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaCopy(scoreTiles, bgGetGfxPtr(state->bg2), scoreTilesLen);
	dmaCopy(scoreMap, bgGetMapPtr(state->bg2),  scoreMapLen);
	
	dmaFillHalfWords( 0x101, bgGetGfxPtr( state->bg3 ), 256 * 192 );
	bgSetScroll(state->bg3,0, 0);

	bgSetPriority(state->bg2, 2);
	bgHide(1);
	bgHide(5);

	state->doorSpriteBot->visible = false;
	state->doorSpriteTop->visible = false;
	state->levelSprite->visible = false;
	for(int i = 0; i < 10; i++){
		state->buttonSprites[i]->visible = false;
		state->buttonNumberSprites[i]->visible = false;
		state->uiGuySprites[i]->visible = false;
		state->arrowSprites[i]->visible = false;
		state->guySprites[i]->visible = false;
		state->guys[i].rectangleNumber->visible = false;
		state->guys[i].rectangle->visible = false;

	}
	state->dropOffGuySprite->visible = false;
	for(int i= 0; i < 2; i++){

		state->floorIndicatorSprites[i]->visible = false;
	}

	displayNumber(state->score, state->scoreSprites, ARRAY_SIZE(state->scoreSprites),  &state->images.numbersFont6px, SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 16, 0, 1, true, 7.0);
	displayNumber(state->maxScore, state->maxScoreSprites, ARRAY_SIZE(state->scoreSprites),  &state->images.numbersFont6px, SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 32, 0, 1, true, 7.0);
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
	FILE* saveFile = fopen("score.bin", "rb+");
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

	struct Vector2i doorPos = {{SCREEN_CENTER.x - 67}, {SCREEN_CENTER.y - 52}};
    	state->images.door = loadImage((uint8_t*)doorTiles, 64, 64, 2);
	state->doorSpriteTop = createSprite(&state->images.door, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, doorPos.x, doorPos.y, 0, &oamMain, false, 1);
	state->images.doorBot = loadImage((uint8_t*)doorBotTiles, 64, 64, 2);
	state->doorSpriteBot = createSprite(&state->images.doorBot, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, doorPos.x, doorPos.y +37, 0, &oamMain, false,10);

	state->images.pressAnyButton = loadImage((uint8_t*)pressAnyButtonTiles, 64, 32, 2);
    	state->images.bigButton = loadImage((uint8_t*)buttonBigTiles, 32, 32, 2);
	state->images.numbersFont6px = loadImage((uint8_t*)numbersFont6pxTiles, 16, 16, 10);
	state->images.rectangle = loadImage((uint8_t*)rectangleTiles, 16, 16, 1);
	state->images.uiGuy = loadImage((uint8_t*)uiGuyTiles, 16, 16, 4);
	state->images.arrow = loadImage((uint8_t*)arrowTiles, 8, 8, 2);

	struct Vector2i pressAnyButtonPos = {{SCREEN_WIDTH/2 - 64}, {SCREEN_HEIGHT/2 + 48}};
	state->pressAnyButtonSprite[0] = createSprite(&state->images.pressAnyButton, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, pressAnyButtonPos.x, pressAnyButtonPos.y , 0, &oamMain, true, 1);
	state->pressAnyButtonSprite[1] = createSprite(&state->images.pressAnyButton, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, pressAnyButtonPos.x + 64, pressAnyButtonPos.y, 1, &oamMain, true, 1);
	state->pressAnyButtonSprite[0]->paletteIdx = 1;
	state->pressAnyButtonSprite[1]->paletteIdx = 1;
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
	for(int i=0; i < MAX_GUYS_ON_SCREEN; i++){
		state->guySprites[i] = createSprite(&state->images.guy, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, 0, 0, 0, &oamMain, false, 1);
	}
	
	state->dropOffGuySprite = createSprite(&state->images.guy, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, 0, SCREEN_HEIGHT/2 - 32, 0, &oamMain, false, 1);
	flipSprite(state->dropOffGuySprite);
	for(int i =0; i < ARRAY_SIZE(state->floorIndicatorSprites); i++){
		state->floorIndicatorSprites[i] = createSprite(&state->images.numbersFont6px, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, SCREEN_WIDTH/2 - 8, SCREEN_HEIGHT - 16, 0, &oamMain, false, 1);
	}
	for(int i =0; i < ARRAY_SIZE(state->scoreSprites); i++){
		state->scoreSprites[i] = createSprite(&state->images.numbersFont6px, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, SCREEN_WIDTH/2 - 8, SCREEN_HEIGHT - 16, 0, &oamMain, false, 1);
		state->maxScoreSprites[i] = createSprite(&state->images.numbersFont6px, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, SCREEN_WIDTH/2 - 8, SCREEN_HEIGHT - 16, 0, &oamMain, false, 1);
		state->scoreSprites[i]->paletteIdx = 1;
		state->maxScoreSprites[i]->paletteIdx = 1;
	}
	for(int i=0; i < MAX_GUYS_ON_SCREEN; i++){
		state->guys[i].rectangle = createSprite(&state->images.rectangle, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, 0, 0, 0, &oamMain, false, 2);
		state->guys[i].rectangleNumber = createSprite(&state->images.numbersFont6px, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, 0, SCREEN_HEIGHT/2, 0, &oamMain, false, 1);
		state->guys[i].rectangle->paletteIdx = 1;
	}
	state->levelSprite = createSprite(&state->images.numbersFont6px, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, 0,0 , 0, &oamMain, false, 1);
	state->levelSprite->paletteIdx = 1;
	for(int j =0; j < ARRAY_SIZE(state->floatingNumbers); j++){
		for(int i = 0; i < 4; i++){
			state->floatingNumbers[j].sprites[i] = createSprite(&state->images.numbersFont6px, (struct Sprite*)&state->spritesMain, &state->spriteCountMain, 0, 0, 0, &oamMain, false, 1);
		}
	}	
	
    /*
    state->images.ui = loadBMP("../spr/ui.bmp", state->readFileFunction);
    state->images.button = loadBMP("../spr/button.bmp", state->readFileFunction, 2);
    state->images.uiBottom = loadBMP("../spr/ui-bottom.bmp", state->readFileFunction);
    state->images.uiGuy = loadBMP("../spr/ui-guy.bmp", state->readFileFunction, 4);
    state->images.elevator = loadBMP("../spr/elevator.bmp", state->readFileFunction);
    state->images.floorB = loadBMP("../spr/floor_b.bmp", state->readFileFunction);
    state->images.floor = loadBMP("../spr/floor.bmp", state->readFileFunction);
    state->images.vigasB = loadBMP("../spr/vigasB.bmp", state->readFileFunction);
    state->images.vigasF = loadBMP("../spr/vigasF.bmp", state->readFileFunction);
    state->images.elevatorF = loadBMP("../spr/elevator_f.bmp", state->readFileFunction);
    state->images.numbersFont4px = loadBMP("../spr/4x6Numbers.bmp", state->readFileFunction, 10);
    state->images.uiLabels = loadBMP("../spr/uiLabels.bmp", state->readFileFunction, 4);
    state->images.titleLabels = loadBMP("../spr/titleLabels.bmp", state->readFileFunction, 2);
    state->images.rectangle = loadBMP("../spr/rectangle.bmp", state->readFileFunction);
*/
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
    /*
    state->circleFocusTimer = {};

    state->failSoundPlaying = false;
*/
    memset(state->elevatorSpots, 0, sizeof(state->elevatorSpots));
    memset(state->fullFloors, 0, sizeof(state->fullFloors));
    for(int i=0; i < 10; i++){
	state->guys[i].active = false;
    }
    for(int i=0; i < ARRAY_SIZE(state->floatingNumbers); i++){
	    state->floatingNumbers[i].active = false;
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
			mmEffectEx(&state->audioFiles.click);
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
            // Timers
	    // Circle Focus
	    	int radius = 13;
		if (state->circleFocusTimer.active){
			if (state->circleFocusTimer.time > 2.5) {
				state->circleFocusTimer.time -= DELTA;
				return;
		    } else if (state->circleFocusTimer.time > 2.2) {
			state->circleFocusTimer.time -= DELTA;
			float focusPercentage = (state->circleFocusTimer.time- 2.2f)*1.0f/0.3f; 
			drawFocusCircle(bitMapMemory, state->circleSpot.x, state->circleSpot.y, (int)(focusPercentage*screenHeight/2 + (1 - focusPercentage) * radius), screenWidth, screenHeight);
			return;

			}
			else if (state->circleFocusTimer.time > 1.4) {
				state->circleFocusTimer.time -= DELTA;
				if (state->circleFocusTimer.time < 1.8 && !state->failSoundPlaying){
					playSound(state->clips, &state->audioFiles.fail, 0.5);
					mmEffectEx(&state->audioFiles.fail);
					state->failSoundPlaying = true;
				}
				return;
			}
			else if (state->circleFocusTimer.time > 1.0) {
				state->circleFocusTimer.time -= DELTA;
				float focusPercentage = (state->circleFocusTimer.time-1.0f)/0.4f; 
				drawFocusCircle(bitMapMemory, state->circleSpot.x, state->circleSpot.y, (int)(focusPercentage*radius), screenWidth, screenHeight);
				return;
			}
			else if (state->circleFocusTimer.time > 0){
				state->circleFocusTimer.time -= DELTA;
				return;
			}
			else if (state->circleFocusTimer.time < 0) {
				state->transitionToBlackTimer.time= TRANSITION_TIME;
				state->transitionToBlackTimer.active = true;
				state->scoreTimer = {true, SCORE_TIME};
				state->currentScreen = SCORE;
				state->circleFocusTimer = {};
				return;

			}
		}	    
		*/
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
				loadScoreGfx(state);
				mmEffectEx(&state->audioFiles.brake);
				mmStop();
				//mmUnload(MOD_MUSIC); // Unload and reload later since both mmStop() and mmPause() aren't working, possible maxmod bug.
				//mmEffectCancelAll(); 
				// TODO remove all this once going to circle focus
				state->transitionToBlackTimer.time= TRANSITION_TIME;
				state->transitionToBlackTimer.active = true;
				state->currentScreen = SCORE;
				state->scoreTimer.active = true;
				state->scoreTimer.time = SCORE_TIME;
				return;
				 /* 
				 */
				//state->circleFocusTimer= {true, CIRCLE_TIME};
				if (state->guys[i].onElevator){
					//state->circleSpot = sum(sum(Vector2i{screenWidth/2, screenHeight/2}, elevatorSpotsPos[state->guys[i].elevatorSpot]), Vector2i{11,32});
				} else{
				//state->circleSpot ={ screenWidth-27, (state->guys[i].currentFloor+1)*16 +7};
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
	    bgSetScroll(2, 0,-floorYOffset); // Scroll BG3 layer
	    int doorFrame = (state->doorTimer.active) ? 0 : 1;

            state->doorSpriteTop->frame = doorFrame; 
	    state->doorSpriteBot->frame = doorFrame; 
	/*	
            drawImage(renders, ARRAY_SIZE(renders), &state->images.vigasB, 0, 16, 1);
            drawImage(renders, ARRAY_SIZE(renders), &state->images.elevator, (float)(screenWidth - state->images.elevator.width) / 2,
                (float)(screenHeight - 16 - state->images.elevator.height) / 2 + 16, 2);
            drawImage(renders, ARRAY_SIZE(renders), &state->images.elevatorF, (float)(screenWidth - state->images.elevatorF.width) / 2,
                (float)(screenHeight - 16 - state->images.elevatorF.height) / 2 + 16, 5); // Layers 3 and 4 reserved for guys
                        drawImage(renders, ARRAY_SIZE(renders), &state->images.floor, 0, (float)16 - floorYOffset, 7);
            drawImage(renders, ARRAY_SIZE(renders), &state->images.vigasF, 0, 16, 7);
*/
            
/*
            // -- Elevator numbers
            if (state->currentFloor == 10) {
                drawNumber(1, renders, ARRAY_SIZE(renders), &state->images.numbersFont3px, (float)screenCenter.x - 37, (float)screenCenter.y + 38 ,6);
                drawNumber(0, renders, ARRAY_SIZE(renders), &state->images.numbersFont3px, (float)screenCenter.x - 34, (float)screenCenter.y + 35 ,6);
            }
            else {
                drawNumber(state->currentFloor, renders, ARRAY_SIZE(renders), &state->images.numbersFont3px, (float)screenCenter.x - 36, (float)screenCenter.y + 37, 6);
            }
*/
            
            // Display guys
	    struct Vector2i waitingGuyPos = { {10}, {16 + floorYOffset + 40 }};
	    int yLimitBottom = state->elevatorPosY - FLOOR_SEPARATION / 2 + 64;
	    int yLimitTop = state->elevatorPosY + FLOOR_SEPARATION / 2 - 64;
	    for(int i = 0;  i < MAX_GUYS_ON_SCREEN; i++){
		    state->guySprites[i]->visible = false;
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
            for (int j = 0; j < MAX_GUYS_ON_SCREEN; j++) {
		if (state->guys[j].active) {
                    int mood = 3 - ceil(state->guys[j].mood / MOOD_TIME);
		    state->guySprites[j]->frame = mood;
                    if (state->guys[j].onElevator) {

			state->guySprites[j]->visible = true;
			state->guySprites[j]->x = elevatorGuysOrigin.x - elevatorSpotsPos[state->guys[j].elevatorSpot].x;
			state->guySprites[j]->y = elevatorGuysOrigin.y - elevatorSpotsPos[state->guys[j].elevatorSpot].y;
			state->guySprites[j]->priority = (state->guys[j].elevatorSpot < 2) ? 2 : 1;
			state->guys[j].rectangleNumber->visible = true; // NOTE: this could be moved so that it's only calcuated in pickUpAndPlaceGuys() if it ever becaomes a perf problem
			state->guys[j].rectangleNumber->frame = state->guys[j].desiredFloor;
			state->guys[j].rectangleNumber->x = state->guySprites[j]->x + 32;
			state->guys[j].rectangleNumber->y = state->guySprites[j]->y;
			state->guys[j].rectangle->visible = true;
			state->guys[j].rectangle->x = state->guySprites[j]->x + 32;
			state->guys[j].rectangle->y = state->guySprites[j]->y;

                    }
                    else {
			if ((state->guys[j].currentFloor * FLOOR_SEPARATION >= yLimitBottom) && // If they're on the floor
				(state->guys[j].currentFloor * FLOOR_SEPARATION <= yLimitTop)) {
				state->guySprites[j]->visible = true;
				state->guySprites[j]->x = waitingGuyPos.x;
				state->guySprites[j]->y = waitingGuyPos.y;
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

