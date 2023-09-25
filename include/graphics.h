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
	/*
	bool flip;
	int scale;
	bool centered;
	int zLayer;
	uint32_t recolor;
	*/
};

struct Image loadImage(uint8_t* dataPtr, int width, int height, int frames);

struct Sprite* createSprite(struct Image* image, struct Sprite sprites[], int* spriteCount, int x, int y, int frame);

void drawSprite(struct Sprite* sprite);
