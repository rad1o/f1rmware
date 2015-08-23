#ifndef _SELECT_H_
#define _SELECT_H_
#include <stdint.h>
#include <r0ketlib/fs_util.h>

int getFiles(char files[][FLEN], uint8_t count, uint16_t skip, const char *ext);
uint16_t init_selectFile(const char *extension);
int selectFileRepeat(char *filename, const char *extension);
int selectFile(char *filename, const char *extension);

#endif
