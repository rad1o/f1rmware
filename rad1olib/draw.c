#include <rad1olib/setup.h>
#include <r0ketlib/display.h>

void drawHLine(int y, int x1, int x2, uint8_t color) {
    for (int i=x1; i<=x2; ++i) {
        lcdSetPixel(i, y, color);
    }
}

void drawVLine(int x, int y1, int y2, uint8_t color) {
    for (int i=y1; i<=y2; ++i) {
        lcdSetPixel(x, i, color);
    }
}

void drawRectFill(int x, int y, int width, int heigth, uint8_t color) {
    for (int i=y; i<=y+heigth; ++i) {
        drawHLine(i, x, x+width, color);
    }
}