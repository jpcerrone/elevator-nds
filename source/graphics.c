#include "graphics.h"

struct Image loadImage(uint8_t* dataPtr, int width, int height, int frames){
	struct Image newImage;
	newImage.width = width;
	newImage.height = height;
	newImage.frames = frames;
	newImage.pixelPointer = dataPtr;
	return newImage;
}

struct Sprite* createSprite(struct Image* image, struct Sprite sprites[], int* spriteCount, int x, int y, int frame){
	// sassert(spriteCount < MAX_SPRITES, "Sprite count already at maximum value");	
	sprites[*spriteCount].x = x;
	sprites[*spriteCount].y = y;
	sprites[*spriteCount].image = image;
	sprites[*spriteCount].frame = frame;
	sprites[*spriteCount].index = *spriteCount;
	sprites[*spriteCount].oamPtr = oamAllocateGfx(&oamSub, SpriteSize_32x32, SpriteColorFormat_16Color); //TODO oamSub
	(*spriteCount)++;
	return &sprites[*spriteCount-1];
}

void drawSprite(struct Sprite* sprite){
	dmaCopy(sprite->image->pixelPointer + sprite->frame*sprite->image->width*sprite->image->height/2, sprite->oamPtr, sprite->image->width*sprite->image->height); // Here i divide by 2 cuz at 16colors (4bit) each byte of dataPtr contains 2 pixels
	oamSet(&oamSub, sprite->index, sprite->x, sprite->y, 0 , 0, SpriteSize_32x32, SpriteColorFormat_16Color, sprite->oamPtr, -1, false, false, false, false, false); // TODO replace 32x32
}


