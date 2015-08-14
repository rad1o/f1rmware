#ifndef __BASICCONFIG_H_
#define __BASICCONFIG_H_

#include <stdint.h>

int readConfig(void);
int saveConfig(void);
void applyConfig(void);
void enableConfig(char type,char enable);

struct CDESC {
    char *name;
    char value;
    char min;
    char max;
    unsigned disabled :1;
    unsigned type     :3;
};

#define CFG_TYPE_BASIC 0
#define CFG_TYPE_DEVEL 1
#define CFG_TYPE_FLAME 2
#define CFG_TYPE_GONE 3

#define MAXNICK 17
extern struct CDESC the_config[];
extern char nickname[];
extern char nickfont[];
extern char nickl0[];
extern char ledfile[];

#define GLOBALversion      (the_config[ 0].value)
#define GLOBALdaytrig      (the_config[ 1].value)
#define GLOBALdaytrighyst  (the_config[ 2].value)
#define GLOBALdayinvert    (the_config[ 3].value)
#define GLOBALlcdbacklight (the_config[ 4].value)
#define GLOBALlcdmirror    (the_config[ 5].value)
#define GLOBALlcdinvert    (the_config[ 6].value)
#define GLOBALlcdcontrast  (the_config[ 7].value)
#define GLOBALalivechk     (the_config[ 8].value)
#define GLOBALdevelmode    (the_config[ 9].value)
#define GLOBALl0nick       (the_config[10].value)
#define GLOBALchargeled    (the_config[11].value)
#define GLOBALnickname     (nickname)
#define GLOBALnickfont     (nickfont)
#define GLOBALnickl0       (nickl0)
#define GLOBALledfile      (ledfile)
#define GLOBALnickfg       (the_config[12].value)
#define GLOBALnickbg       (the_config[13].value)
#define GLOBALvdd_fix      (the_config[14].value)
#define GLOBALrgbleds      (the_config[15].value)

#define GLOBAL(x) GLOBAL ## x

#endif
