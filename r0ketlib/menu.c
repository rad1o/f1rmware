#include <unistd.h>
#include <stdint.h>

#include <r0ketlib/menu.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/print.h>
#include <r0ketlib/display.h>

/**************************************************************************/

uint8_t menuflags=0;

void handleMenu(const struct MENU *the_menu) {
    uint8_t back = 0;
    int8_t menuselection = 0;
    uint8_t numentries;
    uint8_t visible_lines = 0;
    uint8_t current_offset = 0;

    if (the_menu == NULL) return;

    setSystemFont();

    for (numentries = 0; the_menu->entries[numentries].text != NULL ; numentries++);

/*    if(the_menu->entries[numentries-1].text[0]=='|' && !GLOBAL(develmode))
        numentries--; */

    visible_lines = lcdGetVisibleLines()-1; // subtract title line

    if(menuflags&MENU_BIG)
        visible_lines/=2;

#ifdef SAFETY
    if (visible_lines < 2) return;
#endif

    while (!back) {
        // Display current menu page
        lcdClear();
        lcdPrintln(the_menu->title);

        for (uint8_t i = current_offset; i < (visible_lines + current_offset) && i < numentries; i++) {
            if(menuflags&MENU_BIG)
                lcdNl();
            if (i == menuselection) {
                lcdPrint("*");
            }
            lcdSetCrsrX(14);
            lcdPrintln(the_menu->entries[i].text);
        }
        lcdDisplay();

        switch (getInputWaitTimeout((menuflags&MENU_TIMEOUT)?15000:0)) {
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
                return;
            case BTN_RIGHT:
            case BTN_ENTER:
                getInputWaitRelease();
                if (the_menu->entries[menuselection].callback!=NULL)
                    the_menu->entries[menuselection].callback();
                lcdDisplay();
                setSystemFont();
                if (menuflags&MENU_JUSTONCE)
                    return;
                break;
            case BTN_NONE: /* timeout */
                return;
            default:
                /* NOTREACHED */
                break;
        }
        getInputWaitRelease();
    }
    return;
}
