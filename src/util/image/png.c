#include "png.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <zlib.h>

/**
 Structure for PNG IHDR section (3 bytes longer than in file due to padding)
*/
typedef struct png_hdr_t {

    uint32_t width;
    uint32_t height;
    uint8_t bitDepth;
    uint8_t colorType;
    uint8_t compression;
    uint8_t filterMethod;
    uint8_t interlace;

} png_hdr_t;

/**
 Enum for usefull chunk types, all other will be ignored (except IHDR)
*/
enum png_chunk_types {

    CHUNK_IEND = 1145980233,
    CHUNK_IDAT = 1413563465,

};

/**
 Function for decompressing image data
*/

uint8_t * inflateData(uint8_t * compData, uint32_t dataSize, uint32_t outDataSize) {

    uint8_t * outData = malloc(outDataSize * sizeof(uint8_t));

    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    infstream.avail_in = dataSize;
    infstream.next_in = compData;
    infstream.avail_out = outDataSize;
    infstream.next_out = outData;

    inflateInit(&infstream);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);

    return outData;

}

uint32_t getInflateIndex(unsigned int i, unsigned int j, unsigned int c, unsigned int chanelCount, unsigned int width) {

    return (i * width + j) * chanelCount + c + i + 1;

}

uint32_t getImageIndex(unsigned int i, unsigned int j, unsigned int c, unsigned int chanelCount, unsigned int width) {

    return (i * width + j) * chanelCount + c;

}

#define abs(x) ((x) >= 0 ? (x) : -(x))

int32_t paethPredict(int a, int b, int c) {

    int32_t p = a + b - c;
    int32_t pa = abs(p - a);
    int32_t pb = abs(p - b);
    int32_t pc = abs(p - c);

    if (pa <= pb && pa <= pc) return a;
    else if (pb <= pc) return b;
    return c;

}

uint8_t applyFilters(uint8_t filter, uint8_t * imageData, uint8_t * infData, unsigned int i, unsigned int j, unsigned int c, unsigned int chanelCount, unsigned int width) {

    uint8_t val = infData[getInflateIndex(i, j, c, chanelCount, width)];

    switch (filter) {

        case 0:
            return val;

        case 1:
            if (j >= 1)
                return imageData[getImageIndex(i, j-1, c, chanelCount, width)] + val;
            else
                return val;

        case 2:
            return imageData[getImageIndex(i - 1, j, c, chanelCount, width)] + val;

        case 3:
            return val + (imageData[getImageIndex(i - 1, j, c, chanelCount, width)] + imageData[getImageIndex(i, j - 1, c, chanelCount, width)]) / 2;

        case 4:
            if (j >= 1)
                return (val + paethPredict(imageData[getImageIndex(i, j-1, c, chanelCount, width)],
                                           imageData[getImageIndex(i-1, j, c, chanelCount, width)],
                                           imageData[getImageIndex(i-1, j-1, c, chanelCount, width)]));
            else
                return (val + paethPredict(0, imageData[getImageIndex(i-1, j, c, chanelCount, width)], 0));

    }

}

uint8_t * pngLoadImageData(FILE * file, uint32_t * width, uint32_t * height, uint32_t * cCount) {

    uint64_t hdr;
    fread(&hdr, sizeof(uint64_t), 1, file);

    if (hdr != 0x0a1a0a0d474e5089) {
        fprintf(stderr, "No PNG Header found!\n");
        return NULL;
    }

    uint32_t hdrLength;
    fread(&hdrLength, sizeof(uint32_t), 1, file);
    hdrLength = __builtin_bswap32(hdrLength);

    uint32_t hdrType;
    fread(&hdrType, sizeof(uint32_t), 1, file);

    png_hdr_t header;
    fread(&header, sizeof(png_hdr_t), 1, file);
    fseek(file, -3, SEEK_CUR);

    header.width = __builtin_bswap32(header.width);
    header.height = __builtin_bswap32(header.height);

    if (header.interlace != 0) {
        return NULL;
    }

    uint32_t crc;
    fread(&crc, sizeof(uint32_t), 1, file);

    uint32_t dataSize = 0;
    uint32_t dataIndex = 0;

    uint32_t chunkSize;
    uint32_t chunkType;

    uint8_t * iData = NULL;

    int i = 0;

    while (1) {

        fread(&chunkSize, sizeof(uint32_t), 1, file);
        chunkSize = __builtin_bswap32(chunkSize);
        fread(&chunkType, sizeof(uint32_t), 1, file);

        if (chunkType == CHUNK_IEND) {
            break;
        } else if (chunkType == CHUNK_IDAT) {

            /// Load idat data

            if (!iData) {
                iData = (uint8_t *) malloc(sizeof(uint8_t) * chunkSize);
            } else {
                iData = realloc(iData, (dataSize + chunkSize) * sizeof(uint8_t));
            }

            fread(iData + dataIndex, sizeof(uint8_t), chunkSize, file);

            dataIndex += chunkSize;
            dataSize += chunkSize;


        } else {

            fseek(file, chunkSize, SEEK_CUR);

        }

        fread(&crc, sizeof(uint32_t), 1, file);

    }

    uint32_t chanelCount = 3;
    if (header.colorType == 6) chanelCount = 4;
    uint32_t outDataSize = (header.bitDepth / 8) * chanelCount * (header.width+1) * header.height;
    uint8_t * inflatedData = inflateData(iData, dataSize, outDataSize);

    *height = header.height;
    *width  = header.width;
    *cCount = chanelCount;

    free(iData);

    uint8_t * imageData = (uint8_t *) malloc(chanelCount * header.width * header.height * sizeof(uint8_t));
    for (unsigned int i = 0; i < header.height; ++i) {
        uint8_t filterType = inflatedData[(i * header.width) * chanelCount + i];
        for (unsigned int j = 0; j < header.width; ++j) {
            for (unsigned int c = 0; c < chanelCount; ++c) {

                imageData[(i * header.width + j) * chanelCount + c] = applyFilters(filterType, imageData, inflatedData, i, j, c, chanelCount, header.width);

            }
        }
    }

    free(inflatedData);

    return imageData;

}

uint8_t * pngLoadImageDataMemory(uint8_t * file, uint32_t * width, uint32_t * height, uint32_t * cCount) {

    uint32_t fOffset = 0;

    uint64_t hdr;
    //fread(&hdr, sizeof(uint64_t), 1, file);
    hdr = *((uint64_t *) (file + fOffset));
    fOffset += sizeof(uint64_t);

    if (hdr != 0x0a1a0a0d474e5089) {
        fprintf(stderr, "No PNG Header found!\n");
        return NULL;
    }

    uint32_t hdrLength;
    //fread(&hdrLength, sizeof(uint32_t), 1, file);
    hdrLength = *((uint32_t *) (file + fOffset));
    fOffset += sizeof(uint32_t);
    hdrLength = __builtin_bswap32(hdrLength);

    uint32_t hdrType;
    //fread(&hdrType, sizeof(uint32_t), 1, file);
    hdrType = *((uint32_t *) (file + fOffset));
    fOffset += sizeof(uint32_t);

    png_hdr_t header;
    //fread(&header, sizeof(png_hdr_t), 1, file);
    //fseek(file, -3, SEEK_CUR);
    header = *((png_hdr_t *) (file + fOffset));
    fOffset += hdrLength;

    header.width = __builtin_bswap32(header.width);
    header.height = __builtin_bswap32(header.height);

    if (header.interlace != 0) {
        printf("Header Interlace = %d\n", header.interlace);
        return NULL;
    }

    uint32_t crc;
    //fread(&crc, sizeof(uint32_t), 1, file);
    crc = *((uint32_t *) file + fOffset);
    fOffset += sizeof(uint32_t);

    uint32_t dataSize = 0;
    uint32_t dataIndex = 0;

    uint32_t chunkSize;
    uint32_t chunkType;

    uint8_t * iData = NULL;

    int i = 0;

    while (1) {

        //fread(&chunkSize, sizeof(uint32_t), 1, file);
        chunkSize = *((uint32_t *) (file + fOffset));
        fOffset += sizeof(uint32_t);
        chunkSize = __builtin_bswap32(chunkSize);
        //fread(&chunkType, sizeof(uint32_t), 1, file);
        chunkType = *((uint32_t *) (file + fOffset));
        fOffset += sizeof(uint32_t);

        if (chunkType == CHUNK_IEND) {
            break;
        } else if (chunkType == CHUNK_IDAT) {

            /// Load idat data

            if (!iData) {
                iData = (uint8_t *) malloc(sizeof(uint8_t) * chunkSize);
            } else {
                iData = realloc(iData, (dataSize + chunkSize) * sizeof(uint8_t));
            }

            //fread(iData + dataIndex, sizeof(uint8_t), chunkSize, file);
            memcpy(iData + dataIndex, file + fOffset, sizeof(uint8_t) * chunkSize);
            fOffset += sizeof(uint8_t) * chunkSize;

            dataIndex += chunkSize;
            dataSize += chunkSize;


        } else {

            //fseek(file, chunkSize, SEEK_CUR);
            fOffset += chunkSize;

        }

        //fread(&crc, sizeof(uint32_t), 1, file);
        crc = *((uint32_t *) (file + fOffset));
        fOffset += sizeof(uint32_t);

    }

    uint32_t chanelCount = 3;
    if (header.colorType == 6) chanelCount = 4;
    uint32_t outDataSize = (header.bitDepth / 8) * chanelCount * (header.width+1) * header.height;
    uint8_t * inflatedData = inflateData(iData, dataSize, outDataSize);

    *height = header.height;
    *width  = header.width;
    *cCount = chanelCount;

    free(iData);

    uint8_t * imageData = (uint8_t *) malloc(chanelCount * header.width * header.height * sizeof(uint8_t));
    for (unsigned int i = 0; i < header.height; ++i) {
        uint8_t filterType = inflatedData[(i * header.width) * chanelCount + i];
        for (unsigned int j = 0; j < header.width; ++j) {
            for (unsigned int c = 0; c < chanelCount; ++c) {

                imageData[(i * header.width + j) * chanelCount + c] = applyFilters(filterType, imageData, inflatedData, i, j, c, chanelCount, header.width);

            }
        }
    }

    free(inflatedData);

    return imageData;

}

