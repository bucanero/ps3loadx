#include "gfx.h"

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>

JpgDatas jpg1;
u32 jpg1_offset;

u32 * texture_pointer; // use to asign texture space without changes texture_mem
u32 * texture_pointer2; // use to asign texture for PNG

PngDatas Png_datas[5];
u32 Png_offset[5];

void LoadTexture()
{

    u32 * texture_mem = tiny3d_AllocTexture(64*1024*1024); // alloc 64MB of space for textures (this pointer can be global)    


    if(!texture_mem) return; // fail!

    texture_pointer = texture_mem;

    ResetFont();
    texture_pointer = (u32 *) AddFontFromBitmapArray((u8 *) font  , (u8 *) texture_pointer, 32, 255, 16, 32, 2, BIT0_FIRST_PIXEL);

    // here you can add more textures using 'texture_pointer'. It is returned aligned to 16 bytes

    jpg1.jpg_in= (void *) psl1ght_jpg_bin;
	jpg1.jpg_size= sizeof(psl1ght_jpg_bin);

    LoadJPG(&jpg1, NULL);

    jpg1_offset = 0;
       
    if(jpg1.bmp_out) {

        memcpy(texture_pointer, jpg1.bmp_out, jpg1.wpitch * jpg1.height);
        
        free(jpg1.bmp_out);

        jpg1.bmp_out= texture_pointer;

        texture_pointer += (jpg1.wpitch/4 * jpg1.height + 3) & ~3; // aligned to 16 bytes (it is u32) and update the pointer

        jpg1_offset = tiny3d_TextureOffset(jpg1.bmp_out);      // get the offset (RSX use offset instead address)
     }
}

int LoadTexturePNG(char * filename, int index)
{
    
    texture_pointer2 = texture_pointer + index * 1024 * 1024; // 4 MB reserved for PNG index

    // here you can add more textures using 'texture_pointer'. It is returned aligned to 16 bytes
   
	LoadPNG(&Png_datas[index], filename);
    free(Png_datas[index].png_in);

    Png_offset[index] = 0;
       
    if(Png_datas[index].bmp_out) {

        memcpy(texture_pointer2, Png_datas[index].bmp_out, Png_datas[index].wpitch * Png_datas[index].height);
        
        free(Png_datas[index].bmp_out);

        Png_datas[index].bmp_out= texture_pointer2;

        Png_offset[index] = tiny3d_TextureOffset(Png_datas[index].bmp_out);      // get the offset (RSX use offset instead address)

     return 0;
     }

    return -1;
}

void DrawBox(float x, float y, float z, float w, float h, u32 rgba)
{
    tiny3d_SetPolygon(TINY3D_QUADS);
       
    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(x + w, y    , z);

    tiny3d_VertexPos(x + w, y + h, z);

    tiny3d_VertexPos(x    , y + h, z);

    tiny3d_End();
}

void DrawTextBox(float x, float y, float z, float w, float h, u32 rgba)
{
    tiny3d_SetPolygon(TINY3D_QUADS);
    
   
    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);
    tiny3d_VertexTexture(0.0f , 0.0f);

    tiny3d_VertexPos(x + w, y    , z);
    tiny3d_VertexTexture(0.99f, 0.0f);

    tiny3d_VertexPos(x + w, y + h, z);
    tiny3d_VertexTexture(0.99f, 0.99f);

    tiny3d_VertexPos(x    , y + h, z);
    tiny3d_VertexTexture(0.0f , 0.99f);

    tiny3d_End();
}

struct {
    float x, y, dx, dy, r, rs;

} m_twat[32];

void init_twat()
{
    int i;

    for(i = 0; i < 32; i++) {
        m_twat[i].x = (rand() % 640) + 104;
        m_twat[i].y = (rand() % 300) + 106;

        m_twat[i].dx = ((float) ((int) (rand() & 7) - 3)) / 12.0f;
        m_twat[i].dy = ((float) ((int) (rand() & 7) - 3)) / 12.0f;
        m_twat[i].r = 0;
        m_twat[i].rs = ((float) ((int) (rand() & 7) - 3)) / 80.0f;
    }
}

void update_twat()
{
    int i;

    for(i = 0; i < 32; i++) {

        if((rand() & 0x1ff) == 5) {
            m_twat[i].dx = ((float) ((int) (rand() & 7) - 3)) / 12.0f;
            m_twat[i].dy = ((float) ((int) (rand() & 7) - 3)) / 12.0f;
            m_twat[i].rs = ((float) ((int) (rand() & 7) - 3)) / 80.0f;
            
        }

        if(m_twat[i].dx == 0.0f && m_twat[i].dy == 0.0f) {m_twat[i].dy = 0.25f; m_twat[i].dx = (rand() & 1) ? 0.25f : -0.25f;}
        if(m_twat[i].rs == 0.0f) m_twat[i].rs = (rand() & 1) ? .001f : -0.001f;
        
        m_twat[i].x += m_twat[i].dx;
        m_twat[i].y += m_twat[i].dy;
        m_twat[i].r += m_twat[i].rs;
    
        if(i & 1) {
            if(m_twat[i].x < 0)  {
                if(m_twat[i].dx < 0) m_twat[i].dx = -m_twat[i].dx;
            }

            if(m_twat[i].y < 0)  {
                if(m_twat[i].dy < 0) m_twat[i].dy = -m_twat[i].dy;
            }

            if(m_twat[i].x >= 600)  {
                if(m_twat[i].dx > 0) m_twat[i].dx = -m_twat[i].dx;
            }

            if(m_twat[i].y >= 480)  {
                if(m_twat[i].dy > 0) m_twat[i].dy = -m_twat[i].dy;
            }
        } else {
            if(m_twat[i].x < 248)  {
                if(m_twat[i].dx < 0) m_twat[i].dx = -m_twat[i].dx;
            }

            if(m_twat[i].y < 32)  {
                if(m_twat[i].dy < 0) m_twat[i].dy = -m_twat[i].dy;
            }

            if(m_twat[i].x >= 848)  {
                if(m_twat[i].dx > 0) m_twat[i].dx = -m_twat[i].dx;
            }

            if(m_twat[i].y >= 512)  {
                if(m_twat[i].dy > 0) m_twat[i].dy = -m_twat[i].dy;
            }
        
        }
        
        draw_twat(m_twat[i].x, m_twat[i].y, m_twat[i].r);
    }
    
}

void draw_twat(float x, float y, float angle)
{
    int n;

    float ang, angs = 6.2831853071796 / 8, angs2 = 6.2831853071796 / 32;

    MATRIX matrix;
    
    // rotate and translate the sprite
    matrix = MatrixRotationZ(angle);
    matrix = MatrixMultiply(matrix, MatrixTranslation(x , y , 65535.0f));

    // fix ModelView Matrix
    tiny3d_SetMatrixModelView(&matrix);

    tiny3d_SetPolygon(TINY3D_TRIANGLES);

    ang = 0.0f;

    for(n = 0; n <8; n++) {

        tiny3d_VertexPos(4.0f *sinf(ang), 4.0f *cosf(ang), 0);
        tiny3d_VertexColor(0xffffff30);
        tiny3d_VertexPos(7.0f *sinf(ang+angs/2), 7.0f *cosf(ang+angs/2), 0);
        tiny3d_VertexColor(0xff00ff40);
        tiny3d_VertexPos(4.0f *sinf(ang+angs), 4.0f *cosf(ang+angs), 0);
        tiny3d_VertexColor(0xffffff30);

        ang += angs;
    }

    tiny3d_End();

    tiny3d_SetPolygon(TINY3D_POLYGON);

    ang = 0.0f;

    for(n = 0; n <32; n++) {
        tiny3d_VertexPos(3.0f * sinf(ang), 3.0f * cosf(ang), 0);
        if(n & 1) tiny3d_VertexColor(0x80ffff40); else tiny3d_VertexColor(0xffffff40);
        ang += angs2;
    }

    tiny3d_End();

    tiny3d_SetMatrixModelView(NULL); // set matrix identity

}

