//
// Created by robert on 4/16/22.
//

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H

int main(int argc, char **argv) {
    FT_Library ft_lib;
    assert(FT_Init_FreeType(&ft_lib) == 0);

    FT_Face ft_face;
    assert(FT_New_Face(ft_lib, argv[2], 0, &ft_face) == 0);
    assert(FT_Set_Pixel_Sizes((FT_Face)ft_face, 0, (unsigned) atoi(argv[1])) == 0);

    int minX = 0, minY = 0, maxX = 0, maxY = 0;
    for(uint8_t c = 0; c < 255; c++) {
        assert(FT_Load_Char((FT_Face) ft_face, c, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO) == 0);
        FT_GlyphSlot glyph = ft_face->glyph;

        int y = -glyph->bitmap_top;
        if(y > maxY) maxY = y;
        if(y < minY) minY = y;

        y += glyph->bitmap.rows - 1;
        if(y > maxY) maxY = y;
        if(y < minY) minY = y;

//        int x = glyph->advance.x / 64;
//        if(x > maxX) maxX = x;
//        if(x < minX) minX = x;

        int x = glyph->bitmap.width - 1;
        if(x > maxX) maxX = x;
        if(x < minX) minX = x;
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
    printf("font bytes: %lu\n", sizeof(output));

    for(uint8_t c = 0; c < 255; c++) {
        uint8_t *oglyph = output + 2 + (c * charSize);
        assert(FT_Load_Char((FT_Face) ft_face, c, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO) == 0);
        FT_GlyphSlot glyph = ft_face->glyph;
        struct FT_Bitmap_ bitmap = glyph->bitmap;
        int y = -glyph->bitmap_top - minY;
        int x = (charWidth - bitmap.width) / 2;
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
                        int b = y + i;
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
