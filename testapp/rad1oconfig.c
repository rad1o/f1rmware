#include <r0ketlib/config.h>
#include <r0ketlib/print.h>
#include <r0ketlib/render.h>
#include <r0ketlib/display.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include <fatfs/ff.h>

#include <string.h>

//# MENU config
void menu_config(void){
    uint8_t numentries = 0;
    signed char menuselection = 0;
    uint8_t visible_lines = 0;
    uint8_t current_offset = 0;

    for (int i=0;the_config[i].name!=NULL;i++){
        if(!the_config[i].disabled)
            numentries++;
    };

    visible_lines = ((RESY/getFontHeight())-1)/2;

    while (1) {
        // Display current menu page
        lcdClear();
        lcdPrint("Config");
        
        lcdSetCrsrX(60);
        lcdPrint("[");
        lcdPrint(IntToStr((current_offset/visible_lines)+1,1,0));
        lcdPrint("/");
        lcdPrint(IntToStr(((numentries-1)/visible_lines)+1,1,0));
        lcdPrint("]");
        lcdNl();

        lcdNl();

        uint8_t j=0;
        for (uint8_t i=0;i<current_offset;i++)
            while (the_config[++j].disabled);

        uint8_t t=0;
        for (uint8_t i=0;i<menuselection;i++)
            while (the_config[++t].disabled);

        for (uint8_t i = current_offset; i < (visible_lines + current_offset) && i < numentries; i++,j++) {
            while(the_config[j].disabled)j++;
            if(i==0){
                lcdPrintln("Save changes:");
                if (i == t)
                    lcdPrint("*");
                lcdSetCrsrX(14);
                if (i == t)
                    lcdPrintln("YES");
                else
                    lcdPrintln("no");
            }else{
                lcdPrintln(the_config[j].name);
                if (j == t)
                    lcdPrint("*");
                lcdSetCrsrX(14);
                lcdPrint("<");
                lcdPrint(IntToStr(the_config[j].value,3,F_LONG));
                lcdPrintln(">");
            };
        lcdDisplay();
        }

        switch (getInputWaitRepeat()) {
            case BTN_UP:
                menuselection--;
                if (menuselection < current_offset) {
                    if (menuselection < 0) {
                        menuselection = numentries-1;
                        current_offset = ((numentries-1)/visible_lines) * visible_lines;
                    } else {
                        current_offset -= visible_lines;
                    }
                }
                break;
            case BTN_DOWN:
                menuselection++;
                if (menuselection > (current_offset + visible_lines-1) || menuselection >= numentries) {
                    if (menuselection >= numentries) {
                        menuselection = 0;
                        current_offset = 0;
                    } else {
                        current_offset += visible_lines;
                    }
                }
                break;
            case BTN_LEFT:
                if(the_config[t].value >
                        the_config[t].min)
                    the_config[t].value--;
                if(the_config[t].value > the_config[t].max)
                    the_config[t].value=
                        the_config[t].max;
                applyConfig();
                break;
            case BTN_RIGHT:
                if(the_config[t].value <
                        the_config[t].max)
                    the_config[t].value++;
                if(the_config[t].value < the_config[t].min)
                    the_config[t].value=
                        the_config[t].min;
                applyConfig();
                break;
            case BTN_ENTER:
                if(menuselection==0)
                    saveConfig();
                return;
        }
    }
    /* NOTREACHED */
}
