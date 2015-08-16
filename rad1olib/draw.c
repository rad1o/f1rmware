#include <rad1olib/setup.h>
#include <r0ketlib/display.h>

#define SWAP(p1, p2) do { int SWAP = p1; p1 = p2; p2 = SWAP; } while (0)
#define ABS(p) (((p)<0) ? -(p) : (p))

void drawHLine(int y, int x1, int x2, uint8_t color) {
	if(x1>x2) {
		SWAP(x1, x2);
	}
    for (int i=x1; i<=x2; ++i) {
        lcdSetPixel(i, y, color);
    }
}

void drawVLine(int x, int y1, int y2, uint8_t color) {
	if(y1>y2) {
		SWAP(y1,y2);
	}
    for (int i=y1; i<=y2; ++i) {
        lcdSetPixel(x, i, color);
    }
}

void drawRectFill(int x, int y, int width, int heigth, uint8_t color) {
    for (int i=y; i<y+heigth; ++i) {
        drawHLine(i, x, x+width-1, color);
    }
}

void drawLine(int x1, int y1, int x2, int y2, uint8_t color) {
	if(x1==x2) {
		drawVLine(x1, y1, y2, color);
		return;
	}
	if(y1==y2) {
		drawHLine(y1, x1, x2, color);
		return;
	}
	bool xSwap = x1 > x2;
	bool ySwap = y1 > y2;
	if(xSwap){
		x1 = -x1;
		x2 = -x2;
	}
	if(ySwap){
		y1 = -y1;
		y2 = -y2;
	}
	bool mSwap = ABS(x2-x1) < ABS(y2-y1);
	if(mSwap) {
		SWAP(x1,y1);
		SWAP(x2,y2);
	}
	int dx = x2-x1;
	int dy = y2-y1;
	int D = 2*dy - dx;
	
	lcdSetPixel(x1, y1, color);
	int y = y1;
	for(int x = x1+1; x < x2; x++) {
		if(D > 0) {
			y++;
			D += 2 * dy - 2 * dx;
		} else {
			D += 2 * dy;
		}
		int px = mSwap ? y : x;
		if(xSwap) {
			px = -px;
		}
		int py = mSwap ? x : y;
		if(ySwap) {
			py = -py;
		}
		lcdSetPixel(px, py, color);
	}
}
