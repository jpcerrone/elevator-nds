#include <nds.h>
#include <stdio.h>

// BG
#include <floor_b.h>

// Sprites
//#include <button.h>
#include <button_big.h>

typedef struct Vector2i{
union {
        int x;
        int width;
    };
    union {
        int y;
        int height;
    };
} Vector2i;

static Vector2i SCREEN_SIZE = {{256}, {192}};

typedef struct Sprite{
	int x;
	int y;
	int width;
	int height;
	int frame;
	int index;
	uint16_t* oamPtr; // TODO check what's the standard (u16 vs uint16_t)
	uint8_t* dataPtr;
} Sprite;

const int MAX_SPRITES = 10; // TODO increase
static int spriteCount;
Sprite sprites[10];

Sprite* createSprite(int x, int y, int width, int height, int frame, uint8_t* dataPtr){
	sassert(spriteCount < MAX_SPRITES, "Sprite count already at maximum value");	
	sprites[spriteCount].x = x;
	sprites[spriteCount].y = y;
	sprites[spriteCount].width = width;
	sprites[spriteCount].height = height;
	sprites[spriteCount].frame = frame;
	sprites[spriteCount].index = spriteCount;
	sprites[spriteCount].oamPtr = oamAllocateGfx(&oamSub, SpriteSize_32x32, SpriteColorFormat_16Color); //TODO oamSub
	sprites[spriteCount].dataPtr = dataPtr;
	spriteCount++;
	return &sprites[spriteCount-1];
}
void drawSprite(Sprite* sprite){
	dmaCopy(sprite->dataPtr + sprite->frame*sprite->width*sprite->height/2, sprite->oamPtr, sprite->width*sprite->height); // Here i divide by 2 cuz at 16colors (4bit) each byte of dataPtr contains 2 pixels
	oamSet(&oamSub, sprite->index, sprite->x, sprite->y, 0 /* priority*/, 0, SpriteSize_32x32, SpriteColorFormat_16Color, sprite->oamPtr, -1, false, false, false, false, false); // TODO replace 32x32
}

int main(void) {
	powerOn(POWER_ALL_2D);
	videoSetMode(MODE_5_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankD(VRAM_D_SUB_SPRITE);

	//consoleDemoInit();
	int bg3 = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0 /*the 2k offset into vram the tile map will be placed*/,0 /* the 16k offset into vram the tile graphics data will be placed*/);
	dmaCopy(floor_bBitmap, bgGetGfxPtr(bg3), floor_bBitmapLen);
	dmaCopy(floor_bPal, BG_PALETTE, floor_bPalLen); 

	// Sprites
	oamInit(&oamSub, SpriteMapping_1D_64, false); // Note: I think this means the first row is 32bits wide 
	dmaCopy(buttonBigPal, SPRITE_PALETTE_SUB, buttonBigPalLen);
	memset(sprites, 0, sizeof(Sprite)*MAX_SPRITES);

	// Display buttons
	for(int y=0;y < 10; y++){
		int x = SCREEN_SIZE.width/2 - 16;
		x += (y%2 == 0) ? 14 : -14;
		createSprite(x, y * 16 + 8, 32, 32, 0, (uint8_t*)buttonBigTiles);
	}
	//float delta = 0.01666666666;
	while(1) {
		for(int i=0; i < spriteCount; i++){
			drawSprite(&sprites[i]);
		}
		swiWaitForVBlank();
		oamUpdate(&oamSub);
		scanKeys();
		int pressed = keysDown();
		if(pressed & KEY_UP){
			bgScroll(bg3, 0,1);
			sprites[0].frame = 1;
		}
		bgUpdate();
	}

}

