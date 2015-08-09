#ifndef __DISPLAYIMG_H_
#define __DISPLAYIMG_H_

#include <fatfs/ff.h>

typedef enum {IMG_NONE=0, IMG_RAW_8, IMG_RAW_12, IMG_RAW_16} image_t;

uint8_t lcdShowAnim(char *file);
int lcdShowImage(FIL *file);
uint8_t lcdShowImageFile(char *fname);

#endif
