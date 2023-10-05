#include "graphics.h"
#include <math.h>
struct Image loadImage(uint8_t* dataPtr, int width, int height, int frames){
	struct Image newImage;
	newImage.width = width;
	newImage.height = height;
	newImage.frames = frames;
	newImage.pixelPointer = dataPtr;
	return newImage;
}

struct Sprite* createSprite(struct Image* image, struct Sprite sprites[], int* spriteCount, int x, int y, int frame, OamState* screen, bool visible, int priority){
	// sassert(spriteCount < MAX_SPRITES, "Sprite count already at maximum value");	
	sassert(image->width == image->height, "Image is not sqaure");
	sassert(image->width == 64 ||  image->width == 32 || image->width == 16 || image->width == 8, "Image size not suported");

	sprites[*spriteCount].x = x;
	sprites[*spriteCount].y = y;
	sprites[*spriteCount].image = image;
	sprites[*spriteCount].frame = frame;
	sprites[*spriteCount].index = *spriteCount;
	sprites[*spriteCount].screen = screen;
	sprites[*spriteCount].visible = visible;
	sprites[*spriteCount].priority = priority;
	sprites[*spriteCount].flipH = false;
	sprites[*spriteCount].paletteIdx = 0;
	SpriteSize size = SpriteSize_64x64;
	if (image->width == 64 && image->height == 64){
		size = SpriteSize_64x64;
	}
	else if(image->width == 32 && image->height == 32){
		size = SpriteSize_32x32;
	}
	else if(image->width == 16 && image->height == 16){
		size = SpriteSize_16x16;
	}
	else if(image->width == 8 && image->height == 8){
		size = SpriteSize_8x8;
	}
	sprites[*spriteCount].oamPtr = oamAllocateGfx(screen, size, SpriteColorFormat_16Color);
	(*spriteCount)++;
	return &sprites[*spriteCount-1];
}

void drawSprite(struct Sprite* sprite){
	dmaCopy(sprite->image->pixelPointer + sprite->frame*sprite->image->width*sprite->image->height/2, sprite->oamPtr, sprite->image->width*sprite->image->height); // Here i divide by 2 cuz at 16colors (4bit) each byte of dataPtr contains 2 pixels
	SpriteSize size; // TODO ADD TO SPRITE STRUCT
	// TODO collapse in switch, only check 1 dimmension and assert that theyre equal beforehand
	if (sprite->image->width == 64 && sprite->image->height == 64){
		size = SpriteSize_64x64;
	}
	else if (sprite->image->width == 32 && sprite->image->height == 32){
		size = SpriteSize_32x32;
	}
	else if(sprite->image->width == 16 && sprite->image->height == 16){
		size = SpriteSize_16x16;
	}
	else{ 
		size = SpriteSize_8x8;
	}
	oamSet(sprite->screen, sprite->index, sprite->x, sprite->y, sprite->priority , sprite->paletteIdx, size, SpriteColorFormat_16Color, sprite->oamPtr, -1, false, !sprite->visible, sprite->flipH, false, false); 
}

void flipSprite(struct Sprite* sprite){
	sprite->flipH =	!sprite->flipH;  
}

void getDigitsFromNumber(uint32_t number, int* digits, int maxDigits) {
    int currentDigit = 0;
    int currentNumber = number;
    for (int i = maxDigits - 1; i >= 0; i--) {
        int significantVal = pow(10, i);
        int numBySignificantVal = currentNumber / significantVal;
        if ((numBySignificantVal) >= 1) {
            digits[currentDigit] = (int)(numBySignificantVal);
            currentNumber = currentNumber - significantVal * numBySignificantVal;
        }
        currentDigit++;
    }
}

void displayNumber(uint32_t number, struct Sprite* sprites[], uint16_t displaySize, struct Image* font, float x, float y, int priority, int scale, bool centered, float spacing){
    float digitSeparation =(float)(spacing);
    sassert(number < pow(10, displaySize), "Number digits exceed maximum display slots");
    int digits[displaySize] = { };
    int digitsToDraw = displaySize;
    getDigitsFromNumber(number, digits, displaySize);
    for (int i = 0; i < displaySize; i++) {
        sprites[i]->visible = false;
    }
    for (int i = 0; i < displaySize; i++) {
        if (digits[i] == 0) {
            digitsToDraw--;
        }
	else{
		break;
	}
    }
    digitsToDraw = fmax(digitsToDraw, 1); // So that '0' can be drawn as a single digit.
    
    if (centered) {
        x = x - ((digitsToDraw-1) * digitSeparation + font->width) / 2.0f;
    }
    
    for (int i = 0; i < digitsToDraw; i++) {
	    sprites[i]->visible = true;
	    sprites[i]->image = font;
	    sprites[i]->x = x + i * digitSeparation;
	    sprites[i]->y = y;
	    sprites[i]->frame = digits[displaySize - digitsToDraw + i];
	    sprites[i]->flipH = false; //IMPROVEMENT add parameter to drawNumberCall
	    //sprites[i]->scale = scale;
	    //sprites[i]->centered = centered;
	    sprites[i]->priority = priority; 
	    //sprites[i]->recolor = color;	
    }	

}

