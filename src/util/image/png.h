#ifndef PNG_H_INCLUDED
#define PNG_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdio.h>
#include <stdint.h>

/**
 Load PNG image data from file, still very experimental.
*/
uint8_t * pngLoadImageData(FILE * file, uint32_t * width, uint32_t * height);

/**
 Load PNG image from memory buffer.
*/
uint8_t * pngLoadImageDataMemory(uint8_t * file, uint32_t * width, uint32_t * height);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // PNG_H_INCLUDED
