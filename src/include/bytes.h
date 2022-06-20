#ifndef __BYTES_H__
#define __BYTES_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

uint8_t* uint_to_little_endian_bytearray(unsigned int number, int size) {
    uint8_t* nle = (uint8_t*)malloc(size * sizeof(uint8_t));

    for(int i = 0; i < size; i++) {
        nle[i] = number >> i * 8 & 0xFF;
    }

    return nle;
}

void merge_bytes(uint8_t* bytes, int bytesLen, uint8_t* bytes2, int bytesLen2, uint8_t** outBytes, int* outBytesLen) {
    *outBytes = (uint8_t*)malloc((bytesLen + bytesLen2) * sizeof(uint8_t));
    *outBytesLen = 0;

    for(int i = 0; i < bytesLen; i++) {
        (*outBytes)[(*outBytesLen)++] = bytes[i];
    }

    for(int i = 0; i < bytesLen2; i++) {
        (*outBytes)[(*outBytesLen)++] = bytes2[i];
    }
}

void printBytes(uint8_t* bytes, int bytesLen) {
    for(int i = 0; i < bytesLen; i++) {
        printf("\\x%02x", bytes[i]);
    } 

    printf("\n");    
}
#endif