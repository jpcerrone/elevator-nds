#pragma once
#include <nds.h>

#define MAX_SPRITES 10 // TODO increase

struct Image {
    uint8_t* pixelPointer;
    uint32_t width;
    uint32_t height;
    int frames;
};

struct Sprite{
	struct Image* image;
	uint16_t* oamPtr;
	float x;
	float y;
	int frame;
	int index;
	OamState* screen;
	bool visible;
	int priority; // 0(highest) - 3(lowest)
	bool flipH;
	int paletteIdx;
	/*
	int scale;
	bool centered;
	uint32_t recolor;
	*/
};

struct Image loadImage(uint8_t* dataPtr, int width, int height, int frames);

struct Sprite* createSprite(struct Image* image, struct Sprite sprites[], int* spriteCount, int x, int y, int frame, OamState* screen, bool visible, int priority);

void drawSprite(struct Sprite* sprite);

void flipSprite(struct Sprite* sprite);

void displayNumber(uint32_t number, struct Sprite* sprites[], uint16_t displaySize, struct Image* font, float x, float y, int priority, int scale, bool centered, float spacing);

void setPalette(int index);
