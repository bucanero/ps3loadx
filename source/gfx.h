#ifndef GFX_H
#define GFX_H

#include <tiny3d.h>
#include <libfont.h>

// font 0: 224 chr from 32 to 255, 16 x 32 pix 2 bit depth
#include "font.h"


#include <jpgdec/loadjpg.h>
#include <pngdec/loadpng.h>
#include "psl1ght_jpg.bin.h" // jpg in memory

extern JpgDatas jpg1;
extern u32 jpg1_offset;

extern u32 * texture_pointer; // use to asign texture space without changes texture_mem
extern u32 * texture_pointer2; // use to asign texture for PNG

extern PngDatas Png_datas[5];
extern u32 Png_offset[5];


void LoadTexture();
int LoadTexturePNG(char * filename, int index);

void DrawBox(float x, float y, float z, float w, float h, u32 rgba);
void DrawTextBox(float x, float y, float z, float w, float h, u32 rgba);

void init_twat();
void update_twat();
void draw_twat(float x, float y, float angle);


#endif