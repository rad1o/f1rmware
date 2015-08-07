/*
 * snake
 ********
 *   a snake clone for the r0ket
 *   created by Flori4n (DrivenHoliday) & MascH (CCCHB tent)
 ***************************************/

#include <sysinit.h>
#include <string.h>

#include "basic/basic.h"
#include "basic/config.h"
#include "basic/random.h"
#include "lcd/render.h"
#include "lcd/display.h"
#include "lcd/fonts.h"
#include "lcd/fonts/invaders.h"
//#Include "lcd/lcd.h"
//#include "lcd/print.h"
#include "funk/mesh.h"
#include "usetable.h"

#define MAX_SNAKE_LEN (40)
#define SNAKE_DIM (3)
#define MIN_SPEED (25)
#define MAX_SPEED (3)
#define MIN_X 2
#define MAX_X (RESX-3)
#define MIN_Y 8
#define MAX_Y (RESY-2)
#define SIZE_X ((MAX_X-MIN_X)/SNAKE_DIM)
#define SIZE_Y ((MAX_Y-MIN_Y)/SNAKE_DIM)

#define RIGHT 0
#define LEFT 2
#define UP 3
#define DOWN 1

struct pos_s {
  int x,y;
};

struct snake_s {
  struct pos_s *tail;
  int len, dir, speed, t_start;
};

static void reset();
static void next_level();
static void draw_block();
static void handle_input();
static void death_anim();
static struct pos_s getFood(void);
static int hitWall();
static int hitFood();
static int hitSelf();
static int showHighscore();

int points = 0;
struct snake_s snake = { NULL, 3, 0, MIN_SPEED, 2};
struct pos_s food;

void ram(void)
{
  int c=0, pos=0,del=0;

  struct pos_s tail[MAX_SNAKE_LEN];
  snake.tail = tail;

  // initially reset everything
  reset();

  while (1)
  {
    if(!(++c % snake.speed)) {
      handle_input();

      pos = (snake.t_start+1) % MAX_SNAKE_LEN;
      snake.tail[pos].x = snake.tail[snake.t_start].x;
      snake.tail[pos].y = snake.tail[snake.t_start].y;

      if(snake.dir == 0)
	snake.tail[pos].x++;
      else if(snake.dir == 1)
	snake.tail[pos].y++;
      else if(snake.dir == 2)
	snake.tail[pos].x--;
      else if(snake.dir == 3)
	snake.tail[pos].y--;
	
      snake.t_start = pos;
      
      if (pos < snake.len) {
	del = MAX_SNAKE_LEN - (snake.len - pos);
      }	else
	del = pos - snake.len;

      // remove last, add first line
      draw_block(snake.tail[del].x, snake.tail[del].y, 0);
      draw_block(snake.tail[pos].x, snake.tail[pos].y, 1);

      // check for obstacle hit..
      if (hitWall() || hitSelf()) {
	death_anim();
	if (showHighscore())
	  break;
	reset();
      }
      else if (hitFood())
	next_level();
  
      lcdDisplay();
    }
    
#ifdef SIMULATOR
    delayms(50);
#else
    delayms(3);
#endif
  }
}

static struct pos_s getFood(void)
{
  int i,pos;
  struct pos_s res;

 tryagain:
  res.x = (getRandom() % (SIZE_X-1)) + 1;
  res.y = (getRandom() % (SIZE_Y-3)) + 3;

  for(i=0; i<snake.len; i++) {
    pos = (snake.t_start < i) ? (MAX_SNAKE_LEN - (i-snake.t_start)) : (snake.t_start-i);
    if (snake.tail[pos].x == res.x && snake.tail[pos].y == res.y) // no food  to be spawn in snake plz
      goto tryagain;
  }

  return  res;
}

static void reset()
{
  int i;

  // setup the screen
  lcdClear();
  for (i=MIN_X; i<MAX_X; i++) {
    lcdSetPixel(i,MIN_Y,1);
    lcdSetPixel(i,MAX_Y,1);
  }
  
  for (i=MIN_Y; i<MAX_Y; i++) {
    lcdSetPixel(MIN_X,i,1);
    lcdSetPixel(MAX_X,i,1);
  }

  snake.speed = MIN_SPEED;
  snake.len = 3;
  snake.dir = 0;
  snake.t_start = 2;

  points = 0;

  // create snake in the middle of the field
  snake.tail[0].x = SIZE_X/2;
  snake.tail[0].y = SIZE_Y/2;
  snake.tail[1].x = SIZE_X/2 +1;
  snake.tail[1].y = SIZE_Y/2;
  snake.tail[2].x = SIZE_X/2 +2;
  snake.tail[2].y = SIZE_Y/2;

  // print initail tail
  draw_block(snake.tail[0].x, snake.tail[0].y, 1);
  draw_block(snake.tail[1].x, snake.tail[1].y, 1);
  draw_block(snake.tail[2].x, snake.tail[2].y, 1);

  // switch to level one
  next_level();
}

static void draw_block(int x, int y, int set)
{
  x *= SNAKE_DIM;
  y *= SNAKE_DIM;

  lcdSetPixel(x  , y,   set);
  lcdSetPixel(x+1, y,   set);
  lcdSetPixel(x+2, y,   set);
  lcdSetPixel(x,   y+1, set);
  lcdSetPixel(x+1, y+1, set);
  lcdSetPixel(x+2, y+1, set);
  lcdSetPixel(x,   y+2, set);
  lcdSetPixel(x+1, y+2, set);
  lcdSetPixel(x+2, y+2, set);

}

static void next_level()
{
  food = getFood();
  draw_block( food.x, food.y, 1);

  snake.len++;
  snake.speed--;
  DoInt(0,0,++points);
}

static void handle_input()
{
  int key = getInputRaw(), dir_old = snake.dir;

  if (key&BTN_UP && dir_old != 1)
    snake.dir = 3;
  else if (key&BTN_DOWN && dir_old != 3)
    snake.dir = 1;
  else if (key&BTN_LEFT && dir_old != 0)
    snake.dir = 2;
  else if (key&BTN_RIGHT && dir_old !=2)
    snake.dir = 0;  
}

static int hitWall()
{
  return ( (snake.tail[snake.t_start].x*3 <= MIN_X)
    || (snake.tail[snake.t_start].x*3 >= MAX_X)
    || (snake.tail[snake.t_start].y*3 <= MIN_Y)
    || (snake.tail[snake.t_start].y*3 >= MAX_Y) ) ?
    1 : 0;
    
}

static int hitSelf()
{
  int i, pos;
  for (i=1; i<snake.len; i++) {
    pos = (snake.t_start < i) ? (MAX_SNAKE_LEN - (i-snake.t_start)) : (snake.t_start-i);
    if (snake.tail[pos].x == snake.tail[snake.t_start].x && snake.tail[pos].y == snake.tail[snake.t_start].y)
      return 1;
  }
  return 0;
}

static void death_anim()
{
  int i,j, a=4;
  
  while(a--) {
    //    lcdToggleFlag(LCD_INVERTED);
    for (i=0; i<RESY; i++) {
      for (j=0; j<RESX; j++) {
    	lcdSetPixel(j,i,!lcdGetPixel(j,i));
      }
    }
    lcdDisplay();

#ifdef SIMULATOR
    delayms(5000);
#else
    delayms(250);
#endif
  }
  ;// this is a stub

}

static bool highscore_set(uint32_t score, char nick[]) {
    MPKT * mpkt= meshGetMessage('s');
    if(MO_TIME(mpkt->pkt)>score)
        return false;

    MO_TIME_set(mpkt->pkt,score);
    strcpy((char*)MO_BODY(mpkt->pkt),nick);
    if(GLOBAL(privacy)==0){
        uint32touint8p(GetUUID32(),mpkt->pkt+26);
        mpkt->pkt[25]=0;
    };
	return true;
}

static uint32_t highscore_get(char nick[]){
    MPKT * mpkt= meshGetMessage('s');
    char * packet_nick = (char*)MO_BODY(mpkt->pkt);
    // the packet crc end is already zeroed
    if(MAXNICK<MESHPKTSIZE-2-6-1)
        packet_nick[MAXNICK-1] = 0;
    strcpy(nick, packet_nick);
	return MO_TIME(mpkt->pkt);
}

static int showHighscore()
{
  int key = getInputRaw(); //throw away pending keypress
  char nick[20];
  uint32_t score = 0;

  highscore_set(points,GLOBAL(nickname));
  score = highscore_get(nick);

  lcdClear();
  DoString(0,RESY/2-33, "  Your Score");
  DoInt(RESX/2-4, RESY/2-25, points);
  DoString(0,RESY/2-10, "  Highscore");
  DoInt(RESX/2-4, RESY/2-2, score);
  DoString(0, RESY/2+18, "  UP to play ");
  DoString(0, RESY/2+26, "DOWN to quit ");
  
  lcdDisplay();
  
  while(1) {
    key = getInputRaw();
    if (key&BTN_DOWN) {
      return 1;
    } else if (key&BTN_UP) {
      return 0;
    }
  }
}

static int hitFood()
{
  return ((snake.tail[snake.t_start].x == food.x) && (snake.tail[snake.t_start].y == food.y)) ? 1 : 0;
}
