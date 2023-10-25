#pragma once
//#include "platform.h"
#include "graphics.h"
//#include "math.h"
#include "vector2i.h"
//#include "audio.c"

#include <fat.h>

#include <maxmod9.h>

static const uint16_t GREY = RGB15(13,13,13);
static const uint16_t BLACK = RGB15(0,0,0);
static const uint16_t MAGENTA = RGB15(31,0,31);

static uint16_t gameColors[3] = {MAGENTA, BLACK, GREY};
static uint16_t gameColorsInv[3] = {MAGENTA, GREY, BLACK};
static uint16_t gameColorsNoTransp[2] = {BLACK, GREY};

#define MAX_GUYS_ON_SCREEN 10
#define ELEVATOR_SPOTS 5
static const float STARTING_SPEED = 150;
static const int FLOOR_SEPARATION = 320;
static int floorsY[11] = { 0, 320, 640, 960, 1280, 1600, 1920, 2240, 2560, 2880, 3200 };
const static int REQUIRED_SCORE = 3000;

static const struct Vector2i elevatorSpotsPos[ELEVATOR_SPOTS] = {
    {{-1}, {-30}},
    {{-22}, {-30}},
    {{-31}, {-37}},
    {{11}, {-37}},
    {{ -10}, {-37}},
};

static const float SPAWN_TIMES[13] = { 8, 6.5f, 5.0f, 5.5f, 4.0f, 3.6f, 3.2f, 3, 2.9f, 2.8f, 2.7f, 2.6f, 2.5f };
static const float MOOD_TIME = 4.0f;
static const float DOOR_TIME = 0.5f;
static const float DROP_OFF_TIME = 1.0f;
static const float TRANSITION_TIME = 1.0f;
static const float SCORE_TIME = 3.0f;
static const float FLASH_TIME = 3.0f;
static const float CIRCLE_TIME = 3.2f;

typedef struct Guy {
	bool active;

	bool onElevator;
	int elevatorSpot;

	int desiredFloor;
	int currentFloor;

	float mood; //From MOOD_TIME*3 to MOOD_TIME, 0 is game over
	struct Sprite* rectangleNumber;
	struct Sprite* rectangle;
} Guy;

typedef enum Screen {
    MENU,
    GAME,
    SCORE,
} Screen;

typedef struct Timer{
	bool active;
	float time;
} Timer;

struct FloatingNumber{
	    bool active;
	    int value;
	    int floor;
	    float offsetY;
	    struct Vector2i startingPosOffset;
	    struct Sprite* sprites[4];
};

typedef struct GameState {
	bool isInitialized;
	Screen currentScreen;
	int score;
	int maxScore;
	bool floorStates[11]; // 0 is the index for floor 0, 10 is the index for floor 9, there's a starting floor 10.
	int elevatorPosY;
	int currentFloor;
	int currentDestination;
	float elevatorSpeed;
	int direction;
	bool moving;
	int currentLevel;

	Timer spawnTimer;
	Timer doorTimer;
	Timer dropOffTimer;
	Timer transitionToBlackTimer;
	Timer transitionFromBlackTimer;
	Timer scoreTimer;
	Timer flashTextTimer;
	Timer circleFocusTimer;

	struct Vector2i circleSpot;
	bool circleScreen;
	bool failSoundPlaying;

	int dropOffFloor;

	Guy guys[MAX_GUYS_ON_SCREEN];
	bool elevatorSpots[ELEVATOR_SPOTS];
	bool fullFloors[10];

	int bg3;
	int bg2;
	int bg1;
	int bg0;
	
	int subBg3;
	int subBg2;
	int subBg1;
	int subBg0;

	struct {
		struct Image pressAnyButton;
		struct Image bigButton;
		struct Image door;
		struct Image doorBot;
		struct Image guy;
		struct Image numbersFont6px;
		struct Image rectangle;
		struct Image uiGuy;
		struct Image arrow;
		/*
		   Image ui;
		   Image button;
		   Image uiBottom;
		   Image elevator;
		   Image elevatorF;
		   Image floorB;
		   Image floor;
		   Image vigasB;
		   Image vigasF;
		   Image numbersFont3px;
		   Image numbersFont4px;
		   Image uiLabels;
		   Image titleLabels;
		   */
	} images;
	struct Sprite spritesMain[128]; 
	struct Sprite spritesSub[64]; 
	int spriteCountMain;
	int spriteCountSub;

	struct Sprite* pressAnyButtonSprite[2];
	struct Sprite* doorSpriteBot; // TODO merge into sprites struct?
	struct Sprite* doorSpriteTop;
	struct Sprite* buttonSprites[10];
	struct Sprite* buttonNumberSprites[10];
	struct Sprite* guySprites[MAX_GUYS_ON_SCREEN];
	struct Sprite* dropOffGuySprite;
	struct Sprite* floorIndicatorSprites[2];
	struct Sprite* scoreSprites[6]; // Used during game and for score screen.
	struct Sprite* maxScoreSprites[6]; 
	struct Sprite* levelSprite;
	struct Sprite* uiGuySprites[10];
	struct Sprite* arrowSprites[10];

	struct FloatingNumber floatingNumbers[3];
	bool musicPlaying;
	struct {
		mm_sound_effect click;
		mm_sound_effect arrival;
		mm_sound_effect brake;
		mm_sound_effect doorClose;
		mm_sound_effect doorOpen;
		mm_sound_effect fail;
		mm_sound_effect passing;
	} audioFiles;
	/*
	 *
	 struct audioFiles_t {
	 AudioFile music;
	 	 };
	 audioFiles_t audioFiles;

	 AudioClip clips[11];
	 */   
} GameState;


typedef struct GameInput {
    union {
        bool buttons[10];
        struct {
            bool button0;
            bool button1;
            bool button2;
            bool button3;
            bool button4;
            bool button5;
            bool button6;
            bool button7;
            bool button8;
            bool button9;
        };
    };

} GameInput;


void updateAndRender(GameInput* input, GameState* state);

