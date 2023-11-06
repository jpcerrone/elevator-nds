#include <stdio.h>
#include <time.h>
#include <math.h>

#include "vector2i.h"
#include "game.h"
#include "graphics.h"

// Sprites & BG
#include <all_gfx.h>

#include <nds.h>
#include <maxmod9.h>
#include "soundbank_bin.h"
#include "soundbank.h"
#include <fat.h>

// Palettes
static const uint16_t GREY = RGB15(13, 13, 13);
static const uint16_t BLACK = RGB15(0, 0, 0);
static const uint16_t MAGENTA = RGB15(31, 0, 31);
static uint16_t gameColors[3] = {MAGENTA, BLACK, GREY};
static uint16_t gameColorsInv[3] = {MAGENTA, GREY, BLACK};

static const float STARTING_SPEED = 150;
static const int FLOOR_SEPARATION = 320;
static int floorsY[11] = {0, 320, 640, 960, 1280, 1600, 1920, 2240, 2560, 2880, 3200};
static const int REQUIRED_SCORE = 3000;

static const struct Vector2i elevatorSpotsPos[ELEVATOR_SPOTS] = {
	{{-11}, {-30}},
	{{-32}, {-30}},
	{{-41}, {-37}},
	{{1}, {-37}},
	{{-20}, {-37}},
};

static const float SPAWN_TIMES[13] = {8, 6.5f, 5.0f, 5.5f, 4.0f, 3.6f, 3.2f, 3, 2.9f, 2.8f, 2.7f, 2.6f, 2.5f};
static const float MOOD_TIME = 4.0f;
static const float DOOR_TIME = 0.5f;
static const float DROP_OFF_TIME = 1.0f;
static const float TRANSITION_TIME = 1.0f;
static const float SCORE_TIME = 3.0f;
static const float FLASH_TIME = 3.0f;
static const float CIRCLE_TIME = 4.2f;

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

static const float DELTA = 0.01666666666;
static const struct Vector2i SCREEN_CENTER = {{SCREEN_WIDTH / 2}, {SCREEN_HEIGHT / 2}};
static const struct Vector2i elevatorGuysOrigin = {{SCREEN_CENTER.x - 50}, {SCREEN_CENTER.y - 57}};

void setNextDirection(struct GameState *state)
{
	// Get next destination
	int currentNext = state->currentFloor;
	int currentMin = INT32_MAX;
	for (int i = 0; i < 11; i++)
	{
		if (state->floorStates[i] == true)
		{
			int newMin = abs(state->currentFloor - i);
			if (newMin < currentMin)
			{
				currentMin = newMin;
				currentNext = i;
			}
		}
	}
	if (state->currentDestination != currentNext)
	{
		state->currentDestination = currentNext;
		int oldDirection = state->direction;
		if (state->currentDestination < state->currentFloor)
		{
			state->direction = -1;
		}
		else if (state->currentDestination > state->currentFloor)
		{ // Changed logic from gdscript
			state->direction = 1;
		}
		if (oldDirection != state->direction)
		{
			state->elevatorSpeed = STARTING_SPEED;
		}
		state->moving = true;
	}
}

void spawnFloatingNumber(struct FloatingNumber *floatingNumbers, int floatingNumbersSize, struct Image *font, int value, int floor)
{
	for (int i = 0; i < floatingNumbersSize; i++)
	{
		if (!floatingNumbers[i].active)
		{
			floatingNumbers[i].active = true;
			floatingNumbers[i].value = value;
			floatingNumbers[i].floor = floor;
			floatingNumbers[i].offsetY = 0;
			floatingNumbers[i].startingPosOffset.x = SCREEN_WIDTH / 8 + rand() % 15;
			floatingNumbers[i].startingPosOffset.y = SCREEN_HEIGHT / 3 - rand() % 20;
			displayNumber(value, floatingNumbers[i].sprites, ARRAY_SIZE(floatingNumbers[i].sprites), font, floatingNumbers[i].startingPosOffset.x, floatingNumbers[i].startingPosOffset.y, 0, 1, true, 7.0);
			break;
		}
	}
}

int getScore(float mood, bool pickingUp)
{
	int score = 0;
	switch ((int)(ceil(mood / MOOD_TIME)))
	{
	case 3:
	{
		score = pickingUp ? 1000 : 200;
	}
	break;
	case 2:
	{
		score = pickingUp ? 500 : 100;
	}
	break;
	case 1:
	{
		score = pickingUp ? 200 : 50;
	}
	break;
	}
	return score;
}

bool isElevatorFull(struct Guy *elevatorSpots[])
{
	for (int s = 0; s < ELEVATOR_SPOTS; s++)
	{
		if (!elevatorSpots[s])
		{
			return false;
		}
	}
	return true;
}

void pickAndPlaceGuys(struct GameState *state)
{
	for (int i = 0; i < MAX_GUYS_ON_SCREEN; i++)
	{
		if (state->guys[i].active)
		{
			if (state->guys[i].onElevator && (state->guys[i].desiredFloor == state->currentFloor))
			{

				int score = getScore(state->guys[i].mood, false);
				state->score += score;

				spawnFloatingNumber(state->floatingNumbers, ARRAY_SIZE(state->floatingNumbers), &state->images.numbersFont6px, score, state->currentFloor);
				for (int j = 0; j < ARRAY_SIZE(state->elevatorSpots); j++)
				{
					if (state->elevatorSpots[j] == &state->guys[i])
					{
						state->elevatorSpots[j] = NULL;
						state->sprites.elevatorGuy[j]->visible = false;
						break;
					}
				}
				state->guys[i].active = false;
				state->guys[i].onElevator = false;
				state->dropOffFloor = state->currentFloor;
				state->dropOffTimer.active = true;
				state->dropOffTimer.time = DROP_OFF_TIME;
			}
			else
			{
				if (state->guys[i].currentFloor == state->currentFloor && !isElevatorFull(state->elevatorSpots))
				{
					int score = getScore(state->guys[i].mood, true);
					state->score += score;
					spawnFloatingNumber(state->floatingNumbers, ARRAY_SIZE(state->floatingNumbers), &state->images.numbersFont6px, score, state->currentFloor);
					state->guys[i].onElevator = true;
					state->guys[i].mood = MOOD_TIME * 3; // 3 to get all 4 possible mood state's ranges [0..3]
					state->fullFloors[state->currentFloor] = false;
					state->guys[i].currentFloor = -1;

					if (state->score >= REQUIRED_SCORE * (state->currentLevel + 1) + (500 * state->currentLevel))
					{
						state->currentLevel += 1;
						state->flashTextTimer.active = true;
						state->flashTextTimer.time = FLASH_TIME;
					}

					for (int s = 0; s < ELEVATOR_SPOTS; s++)
					{
						if (!state->elevatorSpots[s])
						{
							iprintf("puto");
							state->elevatorSpots[s] = &state->guys[i];
							state->sprites.elevatorGuy[s]->visible = true;
							break;
						}
					}
					state->sprites.uiGuy[state->currentFloor]->visible = false;
					state->sprites.arrow[state->currentFloor]->visible = false;
				}
			}
		}
	}
}

bool areAllFloorsSave1Full(bool *fullFloors)
{
	bool oneFree = false;
	for (int i = 0; i < 10; i++)
	{
		if (!fullFloors[i])
		{
			if (!oneFree)
			{
				oneFree = true;
			}
			else
			{
				return false;
			}
		}
	}
	return true;
}

bool areMaxGuysOnScreen(struct Guy *guys)
{
	for (int i = 0; i < MAX_GUYS_ON_SCREEN; i++)
	{
		if (!guys[i].active)
		{
			return false;
		}
	}
	return true;
}

void spawnNewGuy(struct Guy *guys, bool *fullFloors, int currentFloor)
{
	if (areAllFloorsSave1Full(fullFloors) || areMaxGuysOnScreen(guys))
	{
		return;
	}
	int randomGuyIdx = rand() % MAX_GUYS_ON_SCREEN;
	int randomCurrent = rand() % 10;
	int randomDest = rand() % 10;

	while (guys[randomGuyIdx].active)
	{
		randomGuyIdx = rand() % 10;
	}
	while (fullFloors[randomCurrent] || (randomCurrent == currentFloor))
	{
		randomCurrent = rand() % 10;
	}
	while (randomDest == randomCurrent)
	{
		randomDest = rand() % 10;
	}

	guys[randomGuyIdx].active = true;
	guys[randomGuyIdx].mood = MOOD_TIME * 3; // 3 to get all 3 possible mood state's ranges + dead [0..3 + 4]
	guys[randomGuyIdx].currentFloor = randomCurrent;
	guys[randomGuyIdx].desiredFloor = randomDest;
	fullFloors[randomCurrent] = true;
}

void loadMenuGfx(struct GameState *state)
{
	// Load backgrounds
	state->bg2 = bgInit(2, BgType_Text4bpp, BgSize_T_256x512, 0, 1);
	dmaCopy(menuBGTiles, bgGetGfxPtr(state->bg2), menuBGTilesLen);
	dmaCopy(menuBGMap, bgGetMapPtr(state->bg2), menuBGMapLen);

	state->bg3 = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 4, 0);
	dmaFillHalfWords(0x0101, bgGetGfxPtr(state->bg3), 256 * 192); // VRAM can only be accessed with a 16b pointer, so we have to handle two pixels at a time
	bgSetScroll(state->bg3, 0, -192);

	state->subBg1 = bgInitSub(1, BgType_Text4bpp, BgSize_T_256x256, 0, 1);
	dmaCopy(bottomScreenBGTiles, bgGetGfxPtr(state->subBg1), bottomScreenBGTilesLen);
	dmaCopy(bottomScreenBGMap, bgGetMapPtr(state->subBg1), bottomScreenBGMapLen);

	state->subBg3 = bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 2, 0);
	dmaFillHalfWords(0x0, bgGetGfxPtr(state->subBg3), 256 * 256); // VRAM can only be accessed with a 16b pointer, so we have to handle two pixels at a time

	bgSetPriority(state->bg3, 0); // Transitions
	bgSetPriority(state->bg2, 1);
	bgSetPriority(state->subBg1, 2);
	bgSetPriority(state->subBg3, 0);
	bgHide(1);

	// Enable/Disable required sprites
	for (int i = 0; i < 10; i++)
	{
		state->sprites.button[i]->visible = true;
		state->sprites.button[i]->frame = 0;
		state->sprites.buttonNumber[i]->visible = true;
		state->sprites.buttonNumber[i]->paletteIdx = 1;
	}
	for (int i = 0; i < 6; i++)
	{
		state->sprites.score[i]->visible = false;
		state->sprites.maxScore[i]->visible = false;
	}
	state->sprites.pressAnyButton[0]->visible = true;
	state->sprites.pressAnyButton[1]->visible = true;
}
void loadGameGfx(struct GameState *state)
{
	// Load backgrounds
	state->bg2 = bgInit(2, BgType_Text4bpp, BgSize_T_256x512, 0, 1);
	dmaCopy(floor_bBGTiles, bgGetGfxPtr(state->bg2), floor_bBGTilesLen);
	dmaCopy(floor_bBGMap, bgGetMapPtr(state->bg2), floor_bBGMapLen);

	state->bg1 = bgInit(1, BgType_Text4bpp, BgSize_T_256x256, 2, 2);
	dmaCopy(_topScreenBGTiles, bgGetGfxPtr(state->bg1), _topScreenBGTilesLen);
	dmaCopy(_topScreenBGMap, bgGetMapPtr(state->bg1), _topScreenBGMapLen);

	state->bg0 = bgInit(0, BgType_Text4bpp, BgSize_T_256x512, 4, 3);
	dmaCopy(frontBGTiles, bgGetGfxPtr(state->bg0), frontBGTilesLen);
	dmaCopy(frontBGMap, bgGetMapPtr(state->bg0), frontBGMapLen);

	bgSetPriority(state->bg2, 3);
	bgSetPriority(state->bg1, 2);
	bgSetPriority(state->bg0, 1);
	bgSetPriority(state->bg3, 0); // Transitions

	dmaFillHalfWords(0x0101, bgGetGfxPtr(state->bg3), 256 * 192);
	bgSetScroll(state->bg3, 0, 0);

	// Enable/Disable required sprites
	state->sprites.doorBot->visible = true;
	state->sprites.doorTop->visible = true;
	state->sprites.level->visible = true;
	for (int i = 0; i < 10; i++)
	{
		state->sprites.button[i]->visible = true;
		state->sprites.buttonNumber[i]->visible = true;
		state->sprites.floorGuy[i]->visible = true;
	}
	for (int i = 0; i < ARRAY_SIZE(state->sprites.elevatorGuy); i++)
	{
		state->sprites.elevatorGuy[i]->visible = false;
	}
	for (int i = 0; i < 6; i++)
	{
		state->sprites.score[i]->visible = true;
	}
	for (int i = 0; i < 2; i++)
	{
		state->sprites.floorIndicator[i]->visible = true;
	}
	for (int i = 0; i < ARRAY_SIZE(state->sprites.scoreBoard); i++)
	{
		state->sprites.scoreBoard[i]->visible = true;
	}
	state->sprites.pressAnyButton[0]->visible = false;
	state->sprites.pressAnyButton[1]->visible = false;
}

void loadScoreGfx(struct GameState *state)
{
	// Load backgrounds
	state->bg2 = bgInit(2, BgType_Text4bpp, BgSize_T_256x512, 0, 1);
	dmaCopy(scoreBGTiles, bgGetGfxPtr(state->bg2), scoreBGTilesLen);
	dmaCopy(scoreBGMap, bgGetMapPtr(state->bg2), scoreBGMapLen);

	dmaFillHalfWords(0x0101, bgGetGfxPtr(state->bg3), 256 * 192);
	bgSetScroll(state->bg3, 0, -192);

	bgSetPriority(state->bg2, 2);
	bgHide(1);
	bgHide(0);

	dmaFillHalfWords(0x0101, bgGetGfxPtr(state->subBg3), 256 * 192);

	// Enable/Disable required sprites
	state->sprites.doorBot->visible = false;
	state->sprites.doorTop->visible = false;
	state->sprites.level->visible = false;
	for (int i = 0; i < 10; i++)
	{
		state->sprites.button[i]->visible = false;
		state->sprites.buttonNumber[i]->visible = false;
		state->sprites.uiGuy[i]->visible = false;
		state->sprites.arrow[i]->visible = false;
		state->sprites.floorGuy[i]->visible = false;
		state->guys[i].rectangleNumber->visible = false;
		state->guys[i].rectangle->visible = false;
	}
	for (int i = 0; i < ARRAY_SIZE(state->sprites.elevatorGuy); i++)
	{
		state->sprites.elevatorGuy[i]->visible = false;
	}
	state->sprites.dropOffGuy->visible = false;
	for (int i = 0; i < 2; i++)
	{
		state->sprites.floorIndicator[i]->visible = false;
	}
	for (int i = 0; i < ARRAY_SIZE(state->sprites.scoreBoard); i++)
	{
		state->sprites.scoreBoard[i]->visible = false;
	}
	displayNumber(state->score, state->sprites.score, ARRAY_SIZE(state->sprites.score), &state->images.numbersFont6px, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 16, 1, 1, true, 7.0);
	displayNumber(state->maxScore, state->sprites.maxScore, ARRAY_SIZE(state->sprites.score), &state->images.numbersFont6px, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 32, 1, 1, true, 7.0);
}

void initAudioEffect(mm_sound_effect *audioEffect, int id, int volume)
{
	audioEffect->id = id;
	audioEffect->rate = 1024; // 1024 means "Take input file rate"
	audioEffect->handle = 0;  // 0 means "Allocate new handle"
	audioEffect->volume = volume;
	audioEffect->panning = 100;
}

void initGameState(struct GameState *state)
{
	mmInitDefaultMem((mm_addr)soundbank_bin);
		
	mmLoad(MOD_MUSIC);
	mmSetModuleVolume(256);

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

	fatInitDefault();
	FILE *saveFile = fopen("score.bin", "rb+"); // NOTE: this method of saving only works for flashcarts
	if (saveFile != NULL)
	{
		if (fread(&state->maxScore, 4, 1, saveFile) == 0)
		{
			state->maxScore = 0;
		};
	}
	else
	{ // If can't open, try to create it
		saveFile = fopen("score.bin", "wb+");
		state->maxScore = 0;
		fwrite(&state->maxScore, 4, 1, saveFile);
	}
	fclose(saveFile);

	initAudioEffect(&state->audioFiles.click, SFX_CLICK, 180);
	initAudioEffect(&state->audioFiles.arrival, SFX_ARRIVAL, 80);
	initAudioEffect(&state->audioFiles.brake, SFX_BRAKE, 255);
	initAudioEffect(&state->audioFiles.doorClose, SFX_DOOR_CLOSE, 127);
	initAudioEffect(&state->audioFiles.doorOpen, SFX_DOOR_OPEN, 127);
	initAudioEffect(&state->audioFiles.fail, SFX_FAIL, 255);
	initAudioEffect(&state->audioFiles.passing, SFX_PASSING, 235);

	srand((uint32_t)time(NULL)); // Set random seed
	state->currentScreen = MENU;

	state->flashTextTimer.active = true;
	state->flashTextTimer.time = FLASH_TIME;

	// Load palettes
	dmaCopy(gameColors, BG_PALETTE, sizeof(uint16_t) * ARRAY_SIZE(gameColors));
	dmaCopy(gameColors, BG_PALETTE_SUB, sizeof(uint16_t) * ARRAY_SIZE(gameColors));
	dmaCopy(gameColors, SPRITE_PALETTE_SUB, sizeof(uint16_t) * ARRAY_SIZE(gameColors));
	dmaCopy(gameColorsInv, SPRITE_PALETTE_SUB + 16, sizeof(uint16_t) * ARRAY_SIZE(gameColorsInv));
	dmaCopy(gameColors, SPRITE_PALETTE, sizeof(uint16_t) * ARRAY_SIZE(gameColors));
	dmaCopy(gameColorsInv, SPRITE_PALETTE + 16, sizeof(uint16_t) * ARRAY_SIZE(gameColorsInv));

	loadMenuGfx(state);

	memset(state->floatingNumbers, 0, sizeof(state->floatingNumbers));

	// Sprite loading
	memset(state->spritesSub, 0, sizeof(state->spritesSub));
	memset(state->spritesMain, 0, sizeof(state->spritesMain));

	struct Vector2i doorPos = {{SCREEN_CENTER.x - 68}, {SCREEN_CENTER.y - 51}};
	state->images.door = loadImage((uint8_t *)doorTiles, 64, 64, 2);
	state->sprites.doorTop = createSprite(&state->images.door, (struct Sprite *)&state->spritesMain, &state->spriteCountMain, doorPos.x, doorPos.y, 0, &oamMain, false, 2);
	state->images.doorBot = loadImage((uint8_t *)doorBotTiles, 64, 64, 2);
	state->sprites.doorBot = createSprite(&state->images.doorBot, (struct Sprite *)&state->spritesMain, &state->spriteCountMain, doorPos.x, doorPos.y + 37, 0, &oamMain, false, 2);

	state->images.pressAnyButton = loadImage((uint8_t *)pressAnyButtonTiles, 64, 32, 2);
	state->images.bigButton = loadImage((uint8_t *)button_bigTiles, 32, 32, 2);
	state->images.numbersFont6px = loadImage((uint8_t *)numbersFont6pxTiles, 16, 16, 10);
	state->images.rectangle = loadImage((uint8_t *)rectangleTiles, 16, 16, 1);
	state->images.uiGuy = loadImage((uint8_t *)ui_guyTiles, 16, 16, 4);
	state->images.arrow = loadImage((uint8_t *)arrowTiles, 8, 8, 2);
	state->images.scoreBoard = loadImage((uint8_t *)scoreBoardTiles, 64, 32, 4);

	struct Vector2i pressAnyButtonPos = {{SCREEN_WIDTH / 2 - 64}, {SCREEN_HEIGHT / 2 + 48}};
	state->sprites.pressAnyButton[0] = createSprite(&state->images.pressAnyButton, (struct Sprite *)&state->spritesMain, &state->spriteCountMain, pressAnyButtonPos.x, pressAnyButtonPos.y, 0, &oamMain, true, 1);
	state->sprites.pressAnyButton[1] = createSprite(&state->images.pressAnyButton, (struct Sprite *)&state->spritesMain, &state->spriteCountMain, pressAnyButtonPos.x + 64, pressAnyButtonPos.y, 1, &oamMain, true, 1);
	state->sprites.pressAnyButton[0]->paletteIdx = 1;
	state->sprites.pressAnyButton[1]->paletteIdx = 1;

	for (int i = 0; i < ARRAY_SIZE(state->sprites.scoreBoard); i++)
	{
		state->sprites.scoreBoard[i] = createSprite(&state->images.scoreBoard, (struct Sprite *)&state->spritesMain, &state->spriteCountMain, i * 64, SCREEN_HEIGHT - 28, i, &oamMain, false, 1);
	}

	// Can't merge these two similar loops since adding a 16x16 sprite after a 32x32 one screws things up
	for (int y = 0; y < 10; y++)
	{
		int x = SCREEN_WIDTH / 2 - 16;
		x += (y % 2 == 0) ? 14 : -14;
		state->sprites.button[9 - y] = createSprite(&state->images.bigButton, (struct Sprite *)&state->spritesSub, &state->spriteCountSub, x, y * 16 + 8, 0, &oamSub, true, 2);
	}
	for (int y = 0; y < 10; y++)
	{
		int x = SCREEN_WIDTH / 2 - 16;
		x += (y % 2 == 0) ? 14 : -14;
		state->sprites.buttonNumber[9 - y] = createSprite(&state->images.numbersFont6px, (struct Sprite *)&state->spritesSub, &state->spriteCountSub, x + 8, y * 16 + 16, 9 - y, &oamSub, true, 1);
		state->sprites.buttonNumber[9 - y]->paletteIdx = 1;
		int uiGuyX = x;
		int arrowX = x;
		if ((y % 2) == 0)
		{
			uiGuyX += 32;
			arrowX += 44;
		}
		else
		{
			uiGuyX -= 16;
			arrowX -= 20;
		}
		state->sprites.uiGuy[9 - y] = createSprite(&state->images.uiGuy, (struct Sprite *)&state->spritesSub, &state->spriteCountSub, uiGuyX, y * 16 + 16, 0, &oamSub, false, 1);
		state->sprites.arrow[9 - y] = createSprite(&state->images.arrow, (struct Sprite *)&state->spritesSub, &state->spriteCountSub, arrowX, y * 16 + 24, 0, &oamSub, false, 1);
	}
	state->images.guy = loadImage((uint8_t *)guyTiles, 64, 64, 4);
	for (int i = 0; i < ARRAY_SIZE(state->sprites.floorGuy); i++)
	{
		state->sprites.floorGuy[i] = createSprite(&state->images.guy, (struct Sprite *)&state->spritesMain, &state->spriteCountMain, 0, 0, 0, &oamMain, false, 1);
	}
	for (int i = ARRAY_SIZE(state->sprites.elevatorGuy) - 1; i >= 0; i--)
	{ // Iterate backwards to get the last 2 guys to have a higher priority than the rest and appear in front of them
		state->sprites.elevatorGuy[i] = createSprite(&state->images.guy, (struct Sprite *)&state->spritesMain, &state->spriteCountMain, 0, 0, 0, &oamMain, false, 1);
	}
	state->sprites.dropOffGuy = createSprite(&state->images.guy, (struct Sprite *)&state->spritesMain, &state->spriteCountMain, 0, SCREEN_HEIGHT / 2 - 32, 0, &oamMain, false, 1);
	flipSprite(state->sprites.dropOffGuy);
	for (int i = 0; i < ARRAY_SIZE(state->sprites.floorIndicator); i++)
	{
		state->sprites.floorIndicator[i] = createSprite(&state->images.numbersFont6px, (struct Sprite *)&state->spritesMain, &state->spriteCountMain, SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT - 16, 0, &oamMain, false, 1);
	}
	for (int i = 0; i < ARRAY_SIZE(state->sprites.score); i++)
	{
		state->sprites.score[i] = createSprite(&state->images.numbersFont6px, (struct Sprite *)&state->spritesMain, &state->spriteCountMain, SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT - 16, 0, &oamMain, false, 3);
		state->sprites.maxScore[i] = createSprite(&state->images.numbersFont6px, (struct Sprite *)&state->spritesMain, &state->spriteCountMain, SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT - 16, 0, &oamMain, false, 3);
		state->sprites.score[i]->paletteIdx = 1;
		state->sprites.maxScore[i]->paletteIdx = 1;
	}
	for (int i = 0; i < MAX_GUYS_ON_SCREEN; i++)
	{
		state->guys[i].rectangle = createSprite(&state->images.rectangle, (struct Sprite *)&state->spritesMain, &state->spriteCountMain, 0, 0, 0, &oamMain, false, 2);
		state->guys[i].rectangleNumber = createSprite(&state->images.numbersFont6px, (struct Sprite *)&state->spritesMain, &state->spriteCountMain, 0, SCREEN_HEIGHT / 2, 0, &oamMain, false, 1);
		state->guys[i].rectangle->paletteIdx = 1;
	}
	state->sprites.level = createSprite(&state->images.numbersFont6px, (struct Sprite *)&state->spritesMain, &state->spriteCountMain, 0, 0, 0, &oamMain, false, 2);
	state->sprites.level->paletteIdx = 1;
	for (int j = 0; j < ARRAY_SIZE(state->floatingNumbers); j++)
	{
		for (int i = 0; i < 4; i++)
		{
			state->floatingNumbers[j].sprites[i] = createSprite(&state->images.numbersFont6px, (struct Sprite *)&state->spritesMain, &state->spriteCountMain, 0, 0, 0, &oamMain, false, 1);
		}
	}
}

void resetGame(struct GameState *state)
{
	mmStart(MOD_MUSIC, MM_PLAY_LOOP);
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
	for (int i = 0; i < MAX_GUYS_ON_SCREEN; i++)
	{
		state->guys[i].active = false;
	}
	for (int i = 0; i < ARRAY_SIZE(state->floatingNumbers); i++)
	{
		state->floatingNumbers[i].active = false;
		for (int j = 0; j < ARRAY_SIZE(state->floatingNumbers[i].sprites); j++)
		{
			state->floatingNumbers[i].sprites[j]->visible = false;
		}
	}
	loadGameGfx(state);
}

void updateAndRender(struct GameInput *input, struct GameState *state)
{
	switch (state->currentScreen)
	{
	case MENU:
	{
		int flashPerSecond = 2;
		if (state->flashTextTimer.active)
		{
			state->flashTextTimer.time -= flashPerSecond * DELTA;
		}
		if (state->flashTextTimer.time <= 0)
		{
			state->flashTextTimer.time = FLASH_TIME;
		}
		bool drawFlash = ((int)roundf(state->flashTextTimer.time * flashPerSecond)) % 2;
		state->sprites.pressAnyButton[0]->visible = drawFlash;
		state->sprites.pressAnyButton[1]->visible = drawFlash;

		if (state->transitionToBlackTimer.active)
		{
			if (state->transitionToBlackTimer.time <= 0)
			{
				state->transitionToBlackTimer.active = false;
				state->transitionFromBlackTimer.time = TRANSITION_TIME;
				state->transitionFromBlackTimer.active = true;
				resetGame(state);
				state->currentScreen = GAME;
				mmEffectEx(&state->audioFiles.doorOpen);
				return;
			}
			state->transitionToBlackTimer.time -= DELTA;
			break; // The actual transition is handled after the switch statement
		}

		for (int i = 0; i < 10; i++)
		{
			if (input->buttons[i])
			{
				state->transitionToBlackTimer.time = TRANSITION_TIME;
				state->transitionToBlackTimer.active = true;
				mmEffectEx(&state->audioFiles.click);
				state->sprites.button[i]->frame = 1;
				state->sprites.buttonNumber[i]->paletteIdx = 0;
				return;
			}
		}
	}
	break;
	case GAME:
	{
		if (state->transitionFromBlackTimer.active)
		{
			state->transitionFromBlackTimer.time -= DELTA;
			if (state->transitionFromBlackTimer.time <= 0)
			{
				state->transitionFromBlackTimer.active = false;
			}
		}

		// --- Timers
		// Circle Focus
		int radius = 20;
		if (state->circleFocusTimer.active)
		{
			uint16_t *bgGfxPtr = state->circleOnTopScreen ? bgGetGfxPtr(state->bg3) : bgGetGfxPtr(state->subBg3);
			if (state->circleFocusTimer.time > 3.0)
			{
				state->circleFocusTimer.time -= DELTA;
				return;
			}
			else if (state->circleFocusTimer.time > 2.7)
			{
				state->circleFocusTimer.time -= DELTA;
				float focusPercentage = (state->circleFocusTimer.time - 2.7f) * 1.0f / 0.3f;
				state->sprites.level->visible = false; // We´re out of layers, these numbers are in the front and we need to draw the circle transition above them, so hiding them solves the problem.
				if (state->circleOnTopScreen)
				{
					bgHide(0); // Hide foreground BG so that nothing shows up in front of the guy´s faces.
					for (int i = 0; i < ARRAY_SIZE(state->sprites.score); i++)
					{
						state->sprites.score[i]->visible = false;
					}
					for (int i = 0; i < ARRAY_SIZE(state->sprites.floorIndicator); i++)
					{
						state->sprites.floorIndicator[i]->visible = false;
					}
				}
				drawFocusCircle(state->circleSpot, (int)(focusPercentage * SCREEN_HEIGHT / 2 + (1 - focusPercentage) * radius), bgGfxPtr);
				return;
			}
			else if (state->circleFocusTimer.time > 1.8)
			{
				state->circleFocusTimer.time -= DELTA;
				if (state->circleFocusTimer.time < 2.0 && !state->failSoundPlaying)
				{
					mmEffectEx(&state->audioFiles.fail);
					state->failSoundPlaying = true;
				}
				return;
			}
			else if (state->circleFocusTimer.time > 1.0)
			{
				state->circleFocusTimer.time -= DELTA;
				float focusPercentage = (state->circleFocusTimer.time - 1.0f) / 0.8f;
				drawFocusCircle(state->circleSpot, (int)(focusPercentage * radius), bgGfxPtr);
				return;
			}
			else if (state->circleFocusTimer.time > 0)
			{
				state->circleFocusTimer.time -= DELTA;
				return;
			}
			else if (state->circleFocusTimer.time < 0)
			{
				state->scoreTimer.active = true;
				state->scoreTimer.time = SCORE_TIME;
				state->currentScreen = SCORE;
				state->circleFocusTimer.active = false;
				loadScoreGfx(state);
				return;
			}
		}

		// Doors
		if (state->doorTimer.active)
		{
			state->doorTimer.time -= DELTA;
			if (state->doorTimer.time < 0)
			{
				pickAndPlaceGuys(state);
				state->doorTimer.active = 0;
				state->doorTimer.time = 0;
				mmEffectEx(&state->audioFiles.doorClose);
			}
		}

		// Mood
		if (!(state->doorTimer.active))
		{ // Don't update patience when opening doors
			for (int i = 0; i < MAX_GUYS_ON_SCREEN; i++)
			{
				if (state->guys[i].active)
				{
					state->guys[i].mood -= DELTA;
					if (state->guys[i].mood <= 0.0)
					{
						if (state->score > state->maxScore)
						{
							state->maxScore = state->score;
							FILE *saveFile = fopen("score.bin", "wb");
							fwrite(&state->maxScore, 4, 1, saveFile);
							fclose(saveFile);
						}
						mmEffectEx(&state->audioFiles.brake);
						mmStop();

						state->circleFocusTimer.active = true;
						state->circleFocusTimer.time = CIRCLE_TIME;
						dmaFillHalfWords(0x0, bgGetGfxPtr(state->bg3), 256 * 192);
						bgSetScroll(state->bg3, 0, 0);
						if (state->guys[i].onElevator)
						{
							state->circleOnTopScreen = true;
							struct Vector2i elevatorPos = {{0}, {0}};
							for (int j = 0; j < ARRAY_SIZE(state->elevatorSpots); j++)
							{
								if (state->elevatorSpots[j] == &state->guys[i])
								{
									elevatorPos = elevatorSpotsPos[j];
									break;
								}
							}
							state->circleSpot.x = elevatorGuysOrigin.x - elevatorPos.x + 34;
							state->circleSpot.y = elevatorGuysOrigin.y - elevatorPos.y + 24;
						}
						else
						{
							state->circleOnTopScreen = false;
							state->circleSpot.x = (state->guys[i].currentFloor % 2) ? (SCREEN_WIDTH / 2 + 36) : (SCREEN_WIDTH / 2 - 36);
							state->circleSpot.y = (9 - state->guys[i].currentFloor) * 16 + 24;
						}
						return;
					}
				}
			}
		}

		// Spawn
		state->spawnTimer.time -= DELTA;
		if (state->spawnTimer.time <= 0)
		{
			state->spawnTimer.time = SPAWN_TIMES[state->currentLevel];
			spawnNewGuy(state->guys, state->fullFloors, state->currentFloor);
		}

		// Drop Off
		if (state->dropOffTimer.active)
		{
			state->dropOffTimer.time -= DELTA;
		}
		if (state->dropOffTimer.time < 0)
		{
			state->dropOffTimer.active = false;
			state->dropOffTimer.time = 0;
		}

		// Update floor states based on input
		for (int i = 0; i < 10; i++)
		{
			if (input->buttons[i])
			{
				if (!state->moving && i == state->currentFloor)
				{
					continue;
				}
				mmEffectEx(&state->audioFiles.click);
				state->floorStates[i] = !state->floorStates[i];
			}
		}

		// Move and calculate getting to floors
		if (!(state->doorTimer.active))
		{
			if (state->moving)
			{
				state->elevatorSpeed *= 1 + DELTA / 2;
				if (state->direction == 1)
				{
					state->elevatorPosY += (int)((float)state->elevatorSpeed * DELTA);
				}
				else if (state->direction == -1)
				{
					state->elevatorPosY -= (int)((float)state->elevatorSpeed * DELTA);
				}
				if (state->direction == -1)
				{
					if (state->elevatorPosY < floorsY[state->currentFloor - 1])
					{
						state->currentFloor += state->direction;
						setNextDirection(state);
						if (state->currentFloor == state->currentDestination)
						{
							state->elevatorPosY = floorsY[state->currentFloor]; // Correct elevator position
							state->moving = false;
							state->direction = 0; // Not strictly needed I think
							state->floorStates[state->currentDestination] = false;
							state->doorTimer.active = true;
							state->doorTimer.time = DOOR_TIME;
							mmEffectEx(&state->audioFiles.arrival);
							mmEffectEx(&state->audioFiles.doorOpen);
						}
						else
						{
							mmEffectEx(&state->audioFiles.passing);
						}
					}
				}
				else if (state->direction == 1)
				{
					if (state->elevatorPosY > floorsY[state->currentFloor + 1])
					{
						state->currentFloor += state->direction;
						setNextDirection(state);
						if (state->currentFloor == state->currentDestination)
						{
							state->elevatorPosY = floorsY[state->currentFloor]; // Correct elevator position
							state->moving = false;
							state->direction = 0; // Not strictly needed I think
							state->floorStates[state->currentDestination] = false;
							state->doorTimer.active = true;
							state->doorTimer.time = DOOR_TIME;
							mmEffectEx(&state->audioFiles.arrival);
							mmEffectEx(&state->audioFiles.doorOpen);
						}
						else
						{
							mmEffectEx(&state->audioFiles.passing);
						}
					}
				}
			}
			else
			{
				setNextDirection(state);
			}
		}

		// Update button's gfx
		for (int i = 0; i < 10; i++)
		{
			state->sprites.button[i]->frame = state->floorStates[i];
			state->sprites.buttonNumber[i]->paletteIdx = !state->floorStates[i];
		}

		// Draw elevator stuff
		int floorYOffset = state->elevatorPosY % (FLOOR_SEPARATION);
		if (floorYOffset > FLOOR_SEPARATION / 2)
		{
			floorYOffset = (FLOOR_SEPARATION - floorYOffset) * -1; // Hack to handle negative mod operation.
		}
		bgSetScroll(2, 0, -floorYOffset); // Scroll BG2 layer
		bgSetScroll(0, 0, -floorYOffset); // Scroll BG0 layer
		int doorFrame = (state->doorTimer.active) ? 0 : 1;

		state->sprites.doorTop->frame = doorFrame;
		state->sprites.doorBot->frame = doorFrame;

		// Display guys
		struct Vector2i waitingGuyPos = {{10}, {16 + floorYOffset + 55}};
		int yLimitBottom = state->elevatorPosY - FLOOR_SEPARATION / 2 + 64;
		int yLimitTop = state->elevatorPosY + FLOOR_SEPARATION / 2 - 64;
		for (int i = 0; i < MAX_GUYS_ON_SCREEN; i++)
		{
			state->sprites.floorGuy[i]->visible = false;
			state->guys[i].rectangleNumber->visible = false;
			state->guys[i].rectangle->visible = false;
		}
		state->sprites.dropOffGuy->visible = false;
		if (state->dropOffTimer.active)
		{
			if ((state->dropOffFloor * FLOOR_SEPARATION >= yLimitBottom) && (state->dropOffFloor * FLOOR_SEPARATION <= yLimitTop))
			{
				state->sprites.dropOffGuy->visible = true;
				state->sprites.dropOffGuy->x = waitingGuyPos.x;
				state->sprites.dropOffGuy->y = waitingGuyPos.y;
			}
		}

		// All of this is because there are no more layers and I need to draw some guys on top of the others by drawing them in the right order
		for (int i = 0; i < ARRAY_SIZE(state->elevatorSpots); i++)
		{
			if (state->elevatorSpots[i] != NULL)
			{
				int mood = 3 - ceil(state->elevatorSpots[i]->mood / MOOD_TIME);
				state->sprites.elevatorGuy[i]->visible = true;
				state->sprites.elevatorGuy[i]->x = elevatorGuysOrigin.x - elevatorSpotsPos[i].x;
				state->sprites.elevatorGuy[i]->y = elevatorGuysOrigin.y - elevatorSpotsPos[i].y;
				state->sprites.elevatorGuy[i]->priority = 2;
				state->sprites.elevatorGuy[i]->frame = mood;
				state->elevatorSpots[i]->rectangleNumber->visible = true; // NOTE: this could be moved so that it's only calcuated in pickUpAndPlaceGuys() if it ever becaomes a perf problem
				state->elevatorSpots[i]->rectangleNumber->frame = state->elevatorSpots[i]->desiredFloor;
				state->elevatorSpots[i]->rectangleNumber->x = state->sprites.elevatorGuy[i]->x + 32;
				state->elevatorSpots[i]->rectangleNumber->y = state->sprites.elevatorGuy[i]->y;
				state->elevatorSpots[i]->rectangle->visible = true;
				state->elevatorSpots[i]->rectangle->x = state->sprites.elevatorGuy[i]->x + 32;
				state->elevatorSpots[i]->rectangle->y = state->sprites.elevatorGuy[i]->y;
			}
		}

		for (int j = 0; j < MAX_GUYS_ON_SCREEN; j++)
		{
			if (state->guys[j].active)
			{
				int mood = 3 - ceil(state->guys[j].mood / MOOD_TIME);
				if (!state->guys[j].onElevator)
				{
					state->sprites.floorGuy[j]->frame = mood;
					if ((state->guys[j].currentFloor * FLOOR_SEPARATION >= yLimitBottom) && // If they're on the floor
						(state->guys[j].currentFloor * FLOOR_SEPARATION <= yLimitTop))
					{
						state->sprites.floorGuy[j]->visible = true;
						state->sprites.floorGuy[j]->x = waitingGuyPos.x;
						state->sprites.floorGuy[j]->y = waitingGuyPos.y;
						state->guys[j].rectangleNumber->visible = true; // NOTE: this could be moved so that it's only calcuated in pickUpAndPlaceGuys() if it ever becaomes a perf problem
						state->guys[j].rectangleNumber->frame = state->guys[j].desiredFloor;
						state->guys[j].rectangleNumber->x = waitingGuyPos.x + 32;
						state->guys[j].rectangleNumber->y = waitingGuyPos.y;
					}
					// Draw UI guys
					state->sprites.uiGuy[state->guys[j].currentFloor]->visible = true;
					state->sprites.uiGuy[state->guys[j].currentFloor]->frame = mood;
					state->sprites.arrow[state->guys[j].currentFloor]->visible = true;
					int arrowFrame;
					if (state->guys[j].currentFloor < state->guys[j].desiredFloor)
					{
						arrowFrame = 1;
					}
					else
					{
						arrowFrame = 0;
					}
					state->sprites.arrow[state->guys[j].currentFloor]->frame = arrowFrame;
				}
			}
		}

		// Score
		displayNumber(state->score, state->sprites.score, ARRAY_SIZE(state->sprites.score), &state->images.numbersFont6px, 44, SCREEN_HEIGHT - 20, 0, 1, false, 7.0);

		// Current Floor
		displayNumber(state->currentFloor, state->sprites.floorIndicator, ARRAY_SIZE(state->sprites.floorIndicator), &state->images.numbersFont6px, SCREEN_WIDTH / 2.0f + 1, SCREEN_HEIGHT - 16 - 4.0f, 0, 1, true, 7.0);
		// Draw floating numbers
		int floatingSpeed = 40;
		for (int i = 0; i < ARRAY_SIZE(state->floatingNumbers); i++)
		{
			if (state->floatingNumbers[i].active)
			{
				if ((state->floatingNumbers[i].floor * FLOOR_SEPARATION + state->floatingNumbers[i].offsetY >= yLimitBottom) && (state->floatingNumbers[i].floor * FLOOR_SEPARATION + state->floatingNumbers[i].offsetY <= yLimitTop))
				{
					for (int j = 0; j < ARRAY_SIZE(state->floatingNumbers[i].sprites); j++)
					{
						state->floatingNumbers[i].sprites[j]->y = (float)state->floatingNumbers[i].startingPosOffset.y - state->floatingNumbers[i].offsetY + floorYOffset;
					}
				}
				else
				{
					state->floatingNumbers[i].active = false;
					for (int j = 0; j < ARRAY_SIZE(state->floatingNumbers[i].sprites); j++)
					{
						state->floatingNumbers[i].sprites[j]->visible = false;
					}
				}
				state->floatingNumbers[i].offsetY += DELTA * floatingSpeed;
			}
		}
		// Level
		int flashesPerSec = 2;
		if (state->flashTextTimer.active)
		{
			state->flashTextTimer.time -= DELTA;
			if (state->flashTextTimer.time < 0)
			{
				state->flashTextTimer.active = false;
				state->flashTextTimer.time = 0;
			}
		}
		if ((int)(state->flashTextTimer.time * flashesPerSec) % 2 || !state->flashTextTimer.active)
		{
			state->sprites.level->visible = true;
			displayNumber(state->currentLevel, &state->sprites.level, 1, &state->images.numbersFont6px, SCREEN_WIDTH - 19, SCREEN_HEIGHT - 20, 0, 1, false, 7.0);
		}
		else
		{
			state->sprites.level->visible = false;
		}
	}
	break;
	case SCORE:
	{
		if (state->scoreTimer.active)
		{
			state->scoreTimer.time -= DELTA;
			if (state->scoreTimer.time < 0)
			{
				state->scoreTimer.active = false;
				state->scoreTimer.time = 0;
				state->transitionToBlackTimer.active = true;
				state->transitionToBlackTimer.time = TRANSITION_TIME;
			}
		}
		else if (state->transitionToBlackTimer.active)
		{
			state->transitionToBlackTimer.time -= DELTA;
			if (state->transitionToBlackTimer.time < 0)
			{
				state->transitionToBlackTimer.active = false;
				state->transitionToBlackTimer.time = 0;
			}
		}
		else
		{
			state->transitionToBlackTimer.active = false;
			state->flashTextTimer.active = true;
			state->flashTextTimer.time = FLASH_TIME;
			loadMenuGfx(state);
			state->currentScreen = MENU;
		}
	}
	break;
	}
	// Transitions
	if (state->transitionToBlackTimer.active)
	{
		bgSetScroll(state->bg3, 0, -SCREEN_HEIGHT + (int)(SCREEN_HEIGHT * (1 - state->transitionToBlackTimer.time)));
	}

	if (state->transitionFromBlackTimer.active)
	{
		bgSetScroll(state->bg3, 0, (int)(SCREEN_HEIGHT * (1 - state->transitionFromBlackTimer.time)));
	}
}
