#pragma once

#include "graphics.h"
#include "vector2i.h"

#include <fat.h>

#include <maxmod9.h>

#define MAX_GUYS_ON_SCREEN 10
#define ELEVATOR_SPOTS 5

struct Guy {
	bool active;
	bool onElevator;
	int desiredFloor;
	int currentFloor;
	float mood; // From MOOD_TIME*3 to MOOD_TIME, 0 is game over
	struct Sprite* rectangleNumber;
	struct Sprite* rectangle;
};

enum Screen {
    MENU,
    GAME,
    SCORE,
};

struct Timer{
	bool active;
	float time;
};

struct FloatingNumber{
	    bool active;
	    int value;
	    int floor;
	    float offsetY;
	    struct Vector2i startingPosOffset;
	    struct Sprite* sprites[4];
};

struct GameState {
	enum Screen currentScreen;
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

	struct Timer spawnTimer;
	struct Timer doorTimer;
	struct Timer dropOffTimer;
	struct Timer transitionToBlackTimer;
	struct Timer transitionFromBlackTimer;
	struct Timer scoreTimer;
	struct Timer flashTextTimer;
	struct Timer circleFocusTimer;

	struct Vector2i circleSpot;
	bool circleOnTopScreen;
	bool failSoundPlaying;

	int dropOffFloor;

	struct Guy guys[MAX_GUYS_ON_SCREEN];
	struct Guy* elevatorSpots[ELEVATOR_SPOTS];
	bool fullFloors[10];

	// Id´s for backgrounds
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
		struct Image scoreBoard;
	} images;
	struct Sprite spritesMain[128]; 
	struct Sprite spritesSub[64]; 
	int spriteCountMain;
	int spriteCountSub;

	struct {
		struct Sprite* pressAnyButton[2];
		struct Sprite* doorBot;
		struct Sprite* doorTop;
		struct Sprite* button[10];
		struct Sprite* buttonNumber[10];
		struct Sprite* dropOffGuy;
		struct Sprite* floorIndicator[2];
		struct Sprite* score[6]; // Used during game and for score screen.
		struct Sprite* maxScore[6]; 
		struct Sprite* level; // Delete?
		struct Sprite* uiGuy[10];
		struct Sprite* arrow[10];
		struct Sprite* scoreBoard[4];
		struct Sprite* floorGuy[10];
		struct Sprite* elevatorGuy[5];
	} sprites;
	
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

};


struct GameInput {
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

};

void initGameState(struct GameState *state);
void updateAndRender(struct GameInput* input, struct GameState* state);
