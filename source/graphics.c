#include "graphics.h"

struct Image loadImage(uint8_t* dataPtr, int width, int height, int frames){
	struct Image newImage;
	newImage.width = width;
	newImage.height = height;
	newImage.frames = frames;
	newImage.pixelPointer = dataPtr;
	return newImage;
}

struct Sprite* createSprite(struct Image* image, struct Sprite sprites[], int* spriteCount, int x, int y, int frame, OamState* screen){
	// sassert(spriteCount < MAX_SPRITES, "Sprite count already at maximum value");	
	sprites[*spriteCount].x = x;
	sprites[*spriteCount].y = y;
	sprites[*spriteCount].image = image;
	sprites[*spriteCount].frame = frame;
	sprites[*spriteCount].index = *spriteCount;
	sprites[*spriteCount].screen = screen;
	SpriteSize size;
	if (image->width == 64 && image->height == 64){
		size = SpriteSize_64x64;
	}
	else{
		size = SpriteSize_32x32;
	}
	sprites[*spriteCount].oamPtr = oamAllocateGfx(screen, size, SpriteColorFormat_16Color);
	(*spriteCount)++;
	return &sprites[*spriteCount-1];
}

void drawSprite(struct Sprite* sprite){
	dmaCopy(sprite->image->pixelPointer + sprite->frame*sprite->image->width*sprite->image->height/2, sprite->oamPtr, sprite->image->width*sprite->image->height); // Here i divide by 2 cuz at 16colors (4bit) each byte of dataPtr contains 2 pixels
	SpriteSize size; // TODO ADD TO SPRITE STRUCT
	if (sprite->image->width == 64 && sprite->image->height == 64){
		size = SpriteSize_64x64;
	}
	else{
		size = SpriteSize_32x32;
	}oamSet(sprite->screen, sprite->index, sprite->x, sprite->y, 0 , 0, size, SpriteColorFormat_16Color, sprite->oamPtr, -1, false, false, false, false, false); // TODO replace 32x32
}


