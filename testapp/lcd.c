#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>

#undef RGB
#define RGB1(r,g,b) (((r)&0b11111000)|((g)>>5))
#define RGB2(r,g,b) (((g)&0b00011100)<<3|((b)>>3))
#define RGB(r,g,b) ((RGB1(r,g,b)<<8) | RGB2(r,g,b))

#define PIX(r,g,b) do{lcdWrite(TYPE_DATA,RGB1(r,g,b));lcdWrite(TYPE_DATA,RGB2(r,g,b));}while(0)

//# MENU lcd
void lcd_menu(){
	getInputWaitRelease();
	lcdClear();
	lcdPrintln("LCD:");
	lcdDisplay();
	ON(RAD1O_LED1);

	uint16_t x,y;
	uint8_t rgb=0;
	uint8_t ct=0x3a;
	uint16_t rgba[]= { RGB(0xff,0,0),   RGB(0,0xff,0),   RGB(0,0,0xff),
		               RGB(0,0xff,0xff),RGB(0xff,0,0xff),RGB(0xff,0xff,0),
					   RGB(0xff,0xff,0xff),0};
	while(1){
		switch(getInputWaitRepeat()){
			case BTN_DOWN:
				lcd_select();
				lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,5);
				lcdWrite(TYPE_CMD,0x2C);
				for (y=0;y<128;y++){
					for (x=0;x<RESX;x++){
						if (x<20){
							if(y<64){
								PIX(0xff,(0xff-(0xff&(y<<2))),(0xff-(0xff&(y<<2))));
							}else{
								PIX((0xff-(0xff&(y<<2))),0,0);
							};
						}else if (x <40){
							if(y<64){
								PIX((0xff-(0xff&(y<<2))),0xff,(0xff-(0xff&(y<<2))));
							}else{
								PIX(0,(0xff-(0xff&(y<<2))),0);
							};
						}else if (x <60){
							if(y<64){
								PIX((0xff-(0xff&(y<<2))),(0xff-(0xff&(y<<2))),0xff);
							}else{
								PIX(0,0,(0xff-(0xff&(y<<2))));
							};
						}else if (x <80){
							if(y<64){
								PIX(((0xff&(y<<2))),0,0);
							}else{
								PIX(0xff,((0xff&(y<<2))),((0xff&(y<<2))));
							};
						}else if (x <100){
							if(y<64){
								PIX(0,((0xff&(y<<2))),0);
							}else{
								PIX(((0xff&(y<<2))),0xff,((0xff&(y<<2))));
							};
						}else if (x <120){
							if(y<64){
								PIX(0,0,((0xff&(y<<2))));
							}else{
								PIX(((0xff&(y<<2))),((0xff&(y<<2))),0xff);
							};
						}else{
							PIX(0,0,0);
						}
					};
				};
				for (y=128;y<RESY;y++){
					for (x=0;x<RESX;x++){
						PIX(0,0,0);
					};
				};
				lcd_deselect();
				break;
			case BTN_LEFT:
				ct--;
				lcd_select();
				lcdWrite(TYPE_CMD,0x25); lcdWrite(TYPE_DATA, ct);
				lcd_deselect();
				break;
			case BTN_RIGHT:
				ct++;
				lcd_select();
				lcdWrite(TYPE_CMD,0x25); lcdWrite(TYPE_DATA, ct);
				lcd_deselect();
				break;
			case BTN_UP:
				lcd_select();
				lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,5);
				lcdWrite(TYPE_CMD,0x2C);
				if(rgb==0){
					for (y=0;y<RESY;y++){
						for (x=0;x<RESX;x++){
							if(x==0 || x==RESX-1 || y==0 || y==RESY-1){
								PIX(0xff,0,0);
							}else if(x==1 || x==RESX-2 || y==1 || y==RESY-2){
								PIX(0xff,0xff,0);
							}else if(x==2 || x==RESX-3 || y==2 || y==RESY-3){
								PIX(0,0,0xff);
							}else{
								PIX(0xff,0xff,0xff);
							};
						};
					};
				}else{
					for (y=0;y<RESY;y++){
						for (x=0;x<RESX;x++){
							lcdWrite(TYPE_DATA,rgba[rgb-1]>>8);
							lcdWrite(TYPE_DATA,rgba[rgb-1]&0xff);
						};
					};
				};
				lcd_deselect();
				rgb=(rgb+1)%(1+(sizeof(rgba)/sizeof(*rgba)));
				break;
			case BTN_ENTER:
				lcd_select();
				lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,2);
				lcd_deselect();
				return;
		};
		getInputWaitRelease();
	};
};
