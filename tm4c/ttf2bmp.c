//
// Created by robert on 4/16/22.
//

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H

int main(int argc, char **argv) {
    FT_Library ft_lib;
    if(FT_Init_FreeType(&ft_lib)) abort();

    FT_Face ft_face;
    if(FT_New_Face(ft_lib, argv[2], 0, &ft_face)) abort();

    if(FT_Set_Pixel_Sizes((FT_Face)ft_face, 0, (unsigned) atoi(argv[1]))) abort();

    int minX = 0, minY = 0, maxX = 0, maxY = 0;
    FT_Vector kerning = {};
    for(uint8_t c = 0; c < 255; c++) {
        if(FT_Load_Char((FT_Face) ft_face, (FT_UInt) c, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO))
            abort();

        FT_GlyphSlot glyph = ft_face->glyph;

        if(FT_Get_Kerning(ft_face, (FT_UInt)c, (FT_UInt)0, FT_KERNING_DEFAULT, &kerning))
            abort();

        int x = kerning.x / 64;
        int y = kerning.y / 64;

        int yb = y - glyph->bitmap_top;

        struct FT_Bitmap_ bitmap = glyph->bitmap;
        for(unsigned int i = 0; i < bitmap.rows; i++) {
            for (int byte_index = 0; byte_index < bitmap.pitch; byte_index++) {

                uint8_t byte_value = bitmap.buffer[i * bitmap.pitch + byte_index];

                int num_bits_done = byte_index * 8;

                int bits = 8;
                if ((bitmap.width - num_bits_done) < 8) {
                    bits = bitmap.width - num_bits_done;
                }

                for (int bit_index = 0; bit_index < bits; bit_index++) {
                    if(byte_value & (1 << (7 - bit_index))) {
                        int a = x + num_bits_done + bit_index;
                        if(a > maxX) maxX = a;
                        if(a < minX) maxX = a;
                        int b = yb + i;
                        if(b > maxY) maxY = b;
                        if(b < minY) minY = b;
                    }
                }
            }
        }

        x += glyph->advance.x / 64;
        y += glyph->advance.y / 64;
        if(x > maxX) maxX = x;
        if(x < minX) maxX = x;
        if(y > maxY) maxY = y;
        if(y < minY) minY = y;
    }

    uint16_t charWidth = maxX - minX + 1;
    uint16_t charHeight = maxY - minY + 1;
    uint16_t charSize = ((charWidth * charHeight) + 7) / 8;
    uint8_t output[2 + charSize * 256];
    bzero(output, sizeof(output));
    output[0] = charWidth;
    output[1] = charHeight;

    printf("char width: %d\n", charWidth);
    printf("char height: %d\n", charHeight);
    printf("char bytes: %d\n", charSize);
    printf("font bytes: %d\n", sizeof(output));

    for(uint8_t c = 0; c < 255; c++) {
        uint8_t *oglyph = output + 2 + c * charSize;
        if(FT_Load_Char((FT_Face) ft_face, (FT_UInt) c, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO))
            abort();

        FT_GlyphSlot glyph = ft_face->glyph;

        if(FT_Get_Kerning(ft_face, (FT_UInt)c, (FT_UInt)0, FT_KERNING_DEFAULT, &kerning))
            abort();

        int x = (kerning.x / 64) - minX;
        int y = (kerning.y / 64) - minY;

        int yb = y - glyph->bitmap_top;

        struct FT_Bitmap_ bitmap = glyph->bitmap;
        for(unsigned int i = 0; i < bitmap.rows; i++) {
            for (int byte_index = 0; byte_index < bitmap.pitch; byte_index++) {

                uint8_t byte_value = bitmap.buffer[i * bitmap.pitch + byte_index];

                int num_bits_done = byte_index * 8;

                int bits = 8;
                if ((bitmap.width - num_bits_done) < 8) {
                    bits = bitmap.width - num_bits_done;
                }

                for (int bit_index = 0; bit_index < bits; bit_index++) {
                    if(byte_value & (1 << (7 - bit_index))) {
                        int a = x + num_bits_done + bit_index;
                        int b = yb + i;
                        int offset = a + (b * charWidth);
                        oglyph[offset >> 3u] |= 1u << (offset & 7u);
                    }
                }
            }
        }
    }

    printf("\n");
    for(int i = 0; i < sizeof(output); i++) {
        printf("0x%02X", output[i]);
        if(i+1 < sizeof(output))
            printf(", ");
        if((i+1) % 16 == 0)
            printf("\n");
    }
    printf("\n");

    return 0;
}
