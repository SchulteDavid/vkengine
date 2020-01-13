#ifndef _TGA_H
#define _TGA_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <stdio.h>

typedef enum tga_color_type_e {

    TGA_COLOR_BW,         /// Binary black and white
    TGA_COLOR_GS,         /// 8-bit Grayscale
    TGA_COLOR_RGB,        /// 8-bit RBG
    TGA_COLOR_RBGA        /// 8-bit RGB with Alpha

} TGA_COLOR_TYPE;

typedef struct tga_file_t TGA_FILE;

/// Open a TGA Image
TGA_FILE * tgaOpen(const char * fname);
TGA_FILE * tgaOpenFile(FILE * file);

/// Close a TGA Image
void tgaClose(TGA_FILE * file);

/// Get raw body information
uint8_t * tgaGetBytes(TGA_FILE * file);

void tgaGetSize(TGA_FILE * file, int * width, int * height);

void tgaSaveImage(const char * fname, uint8_t * data, int width, int height, int hasAlph);

/// Get the color data from a TGA Image
uint8_t * tgaGetColorDataRGB(TGA_FILE * file);
uint8_t * tgaGetColorDataRGBA(TGA_FILE * file);

uint8_t * tgaUtilReorderComponents(uint8_t * data, int width, int height, int chanels);
uint8_t * tgaUtilReorderPixels(uint8_t * data, int width, int height, int chanels);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
