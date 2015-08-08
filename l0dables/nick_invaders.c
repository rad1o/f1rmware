/* nick_invaders.c
 *  
 *  By NiciDieNase
 *  r0ket@nicidienase.de
 *
 *  Known Bugs:
 *  - Ignores Font-Setting
 *  - might look odd with really short or really long nicknames
 *
 */

#include <sysinit.h>
#include <string.h>
#include "basic/basic.h"
#include "lcd/render.h"
#include "lcd/display.h"
#include "usetable.h"
#include "lcd/fonts.h"
#include "lcd/fonts/invaders.h"
#include "lcd/fonts/invaders.c"

void ram(void)
{
  bool step = true;
  const int minX = 10;
  const int minY = 10;
  const int maxX = 50;
  const int maxY = 30;
  const int space = 15;
  const int delaytime = 150;
  int x = minX;
  int y = minY;
  int u = 0;
  int dir = 0;
  int dx,dy;


  dx=DoString(0,0,nickname);
  dx=(RESX-dx)/2;
  if(dx<0)
      dx=0;
  dy=40;
  
  while(1){
    lcdFill(0);
    font = &Font_Invaders;
    // Draw UFO
    DoChar(-15+u,1,'U');
    u+=2;
    if(u > 1337) u=0;
    //Draw Invaders
    DoChar(x,y,step?'a':'A');
    DoChar(x+space,y,step?'b':'B');
    DoChar(x+2*space,y,step?'c':'C');
    switch (dir){
      // move left
    case 0:
      x++;
      if(x == maxX) dir++;
      break;
      // move down on right end
    case 1:
      y++;
      dir++;      
      if(y == maxY) y=minY;
      break;
      // move left
    case 2:      
      x--;
      if(x==minX) dir++;
      break; 
      // move down on left end
    case 3:
      y++;
      dir++;      
      if(y == maxY) y=minY;
      break;
    }
    dir %= 4;
    font = NULL;
    DoString(dx,dy,nickname);
    lcdDisplay();
    if(getInputRaw()!=BTN_NONE) return;
    step = !step;
    delayms_queue_plus(delaytime,0);
  }
}
