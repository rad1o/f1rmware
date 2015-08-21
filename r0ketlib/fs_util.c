#include <fatfs/ff.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/display.h>
#include <r0ketlib/fs_util.h>
#include <stdint.h>
#include <string.h>

FATFS FatFs;          /* File system object for logical drive */
FS_USAGE FsUsage;

const TCHAR *rcstrings =
    _T("OK\0DISK_ERR\0INT_ERR\0NOT_READY\0NO_FILE\0NO_PATH\0INVALID_NAME\0")
    _T("DENIED\0EXIST\0INVALID_OBJECT\0WRITE_PROTECTED\0INVALID_DRIVE\0")
    _T("NOT_ENABLED\0NO_FILESYSTEM\0MKFS_ABORTED\0TIMEOUT\0LOCKED\0")
    _T("NOT_ENOUGH_CORE\0TOO_MANY_OPEN_FILES\0INVALID_PARAMETER\0");

const char* f_get_rc_string (FRESULT rc) {
	FRESULT i;
    const char *p=rcstrings;

	for (i = 0; i != rc && *p; i++) {
		while(*p++) ;
	}
    return p;
}

void fsInit(){
	FRESULT res;
	res=f_mount(&FatFs,"/",0);
	if(res == FR_OK){
		DIR dir;                /* Directory object */
		res = f_opendir(&dir, "0:");
	};
	if(res != FR_OK){
		lcdPrintln("FS init error");
		lcdPrintln(f_get_rc_string(res));
		lcdDisplay();
		getInputWait();
		return;
	};
}

void fsReInit(){
	f_mount(&FatFs,"/",0);
}

int fsInfo(FATFS *fs)
{
    memcpy(fs, &FatFs, sizeof(FATFS));
    return 0;
}
int fsUsage(FATFS *fs, FS_USAGE *fs_usage)
{
    FRESULT res;
    DWORD tot_clust, fre_clust, sec_size;

    res = f_getfree("/", &fre_clust, &fs);
    if(res != FR_OK)
        return -res;

    // sectore size = sectors per cluster *  sector size
#if _MAX_SS == _MIN_SS
    sec_size = fs->csize * _MAX_SS;
#else
    sec_size = fs->csize * fs.ssize;
#endif

    // total/free sectors * sectore size
    tot_clust = fs->n_fatent - 2;
    fs_usage->total = tot_clust * sec_size; //FatFs.ssize;
    fs_usage->free = fre_clust * sec_size; //FatFs.ssize;

    return 0;
}

int readFile(char * filename, char * data, int len){
    FIL file;
    UINT readbytes;
    int res;

    res=f_open(&file, filename, FA_OPEN_EXISTING|FA_READ);
    if(res){
        return -1;
    };

    res = f_read(&file, data, len, &readbytes);
    if(res){
        return -1;
    };

    f_close(&file);

	return readbytes;
}

int readTextFile(char * filename, char * data, int len){
    int readbytes;

    if(len<1) return -1;
    readbytes=readFile(filename,data,len-1);
    if(readbytes<0){
        data[0]=0;
        return readbytes;
    };
    data[readbytes]=0;
    while(readbytes>0 && data[readbytes-1]<0x20){
        data[--readbytes]=0;
    };
    return readbytes;
}


int writeFile(char * filename, const void * data, int len){
    FIL file;
    UINT writebytes;
    int res;

	res=f_open(&file, filename, FA_CREATE_ALWAYS|FA_WRITE);
    if(res){
        return -res;
    };

    res = f_write(&file, data, len, &writebytes);
    if(res){
        return -res;
    };
    f_close(&file);

	return writebytes;
}

int getFileSize(char * filename){
    FIL file;
    int res;

    res=f_open(&file, filename, FA_READ);
    if(res){
        return -1;
    }

    return file.fsize;
}
