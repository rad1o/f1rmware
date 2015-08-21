#ifndef _FS_UTIL_H
#define _FS_UTIL_H 1

#include <fatfs/ff.h>

#define FLEN 13

typedef struct {
    DWORD total;
    DWORD free;
} FS_USAGE;

void fsInit(void);
void fsReInit(void);

int fsInfo(FATFS *fs);
int fsUsage(FATFS *fs, FS_USAGE *fs_usage);
int readFile(char * filename, char * data, int len);
int readTextFile(char * filename, char * data, int len);
int writeFile(char * filename, const void * data, int len);
int getFileSize(char * filename);
const char* f_get_rc_string (FRESULT rc);

#endif /* _FS_UTIL_H */
