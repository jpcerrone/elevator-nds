#include "graphics.h"

#include <math.h>
#include <stdio.h>

struct Image loadImage(uint8_t* dataPtr, int width, int height, int frames){
	struct Image newImage;
	newImage.width = width;
	newImage.height = height;
	newImage.frames = frames;
	newImage.pixelPointer = dataPtr;
	return newImage;
}

struct Sprite* createSprite(struct Image* image, struct Sprite sprites[], int* spriteCount, int x, int y, int frame, OamState* screen, bool visible, int priority){
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
	else if (image->width == 64 && image->height == 32){
		size = SpriteSize_64x32;
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
	SpriteSize size;
	if (sprite->image->width == 64 && sprite->image->height == 64){
		size = SpriteSize_64x64;
	}
	else if (sprite->image->width == 64 && sprite->image->height == 32){
		size = SpriteSize_64x32;
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
	    sprites[i]->flipH = false;
	    sprites[i]->priority = priority; 
    }	
}

void drawFocusCircle(struct Vector2i center, int radius, uint16_t* backgroundPtr){
	sassert(radius >= 0, "negative radius");
	// VRAM can only be accessed with a 16b pointer, so we have to fill two pixels at a time
	int twoBlackPixels = 0x0101;
	int oneTransparentOneBlack = 0x0001;
	int oneBlackOneTransparent = 0x0100;

	// Fill outside of circle with black first
	dmaFillHalfWords( twoBlackPixels, backgroundPtr, fmax(SCREEN_WIDTH * (center.y-radius), 2));
	if (center.y + radius < SCREEN_HEIGHT){
		dmaFillHalfWords( twoBlackPixels, backgroundPtr + SCREEN_WIDTH*(center.y+radius)/2, SCREEN_WIDTH * (SCREEN_HEIGHT - (center.y + radius)));
	}
	for(int y=fmax(center.y - radius, 0); y < fmin(center.y +radius, SCREEN_HEIGHT); y++){
		if (center.x - radius >= 2){ // Having a > 0 could cause the dma copy to try to fill only one pixel and that screws up the filling
			dmaFillHalfWords( twoBlackPixels, backgroundPtr + y*SCREEN_WIDTH/2 , center.x - radius);
		}
		if (center.x + radius <= SCREEN_WIDTH - 2){
			dmaFillHalfWords( twoBlackPixels, backgroundPtr + y*SCREEN_WIDTH/2 + (center.x + radius)/2, SCREEN_WIDTH - (center.x + radius));
		}
	}
	
	// Fill inside circle with transparent palette index
	int radiusSquared = radius*radius;
	for(int y=center.y - radius; y < center.y + radius; y++){
		for(int x=center.x - radius; x < center.x + radius; x+=2){
			struct Vector2i firstPixel = {{x}, {y}};
			struct Vector2i secondPixel = {{x + 1}, {y}};
			if (distanceSquared(firstPixel, center) < radiusSquared){
				// Filling outside is enough, I don´t neeed to fill the inside since it´s already initialized as transparent
				if(2*(center.x - x) <= SCREEN_WIDTH-2){
					dmaFillHalfWords( twoBlackPixels, backgroundPtr + x/2 + y*SCREEN_WIDTH/2 + 2*(center.x - firstPixel.x)/2, SCREEN_WIDTH - 2*(center.x - x)); // Fills row outside of circle
				}
				break; 
			}
			
			else if (distanceSquared(secondPixel, center) < radiusSquared){
				dmaFillHalfWords( oneTransparentOneBlack, backgroundPtr + x/2 + y*SCREEN_WIDTH/2, 2 );
				if(2*(center.x - secondPixel.x) <= SCREEN_WIDTH-2){
					dmaFillHalfWords( oneBlackOneTransparent, backgroundPtr + x/2 + y*SCREEN_WIDTH/2 + 2*(center.x - secondPixel.x)/2, 2 );
					dmaFillHalfWords( twoBlackPixels, backgroundPtr + (x+2)/2 + y*SCREEN_WIDTH/2 + 2*(center.x - secondPixel.x)/2, SCREEN_WIDTH - 2*(center.x - secondPixel.x)); // Fills row outside of circle
				}
				break; 
			} else{
				if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT){
					dmaFillHalfWords( twoBlackPixels, backgroundPtr + x/2 + y*SCREEN_WIDTH/2, 2 );
				}
			}
		}

	}
	
}
