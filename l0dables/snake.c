/*
 * snake
 ********
 *   a snake clone for the r0ket
 *   created by Flori4n (DrivenHoliday) & MascH (CCCHB tent)
 *   snake II implementation by ElSjaako and OriginalSouth
 ***************************************/

#include <string.h>

#include <r0ketlib/config.h>
#include <r0ketlib/display.h>
#include <r0ketlib/fonts.h>
#include <r0ketlib/render.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/select.h>
#include <rad1olib/systick.h>

#include "invfont.c"

#include "usetable.h"

#define MAX_SNAKE_LEN (40)
#define SNAKE_DIM (3)
#define MIN_SPEED (25)
#define MAX_SPEED (3)
#define MIN_X 3
#define MAX_X (RESX-5)
#define MIN_Y 12
#define MAX_Y (RESY-5)
#define SIZE_X ((MAX_X-MIN_X)/SNAKE_DIM)
#define SIZE_Y ((MAX_Y-MIN_Y)/SNAKE_DIM)

#define RIGHT 0
#define LEFT 2
#define UP 3
#define DOWN 1

//define how long you have to hold down the button to quit
#define QUIT_DELAY 1000

typedef enum {
  SNAKE_STANDARD,
  SNAKE_WRAPPING
} gametype_e;

struct pos_s {
    int x,y;
};

struct snake_s {
    struct pos_s *tail;
    int len, dir, speed, t_start;
};

static void reset();
static void next_level();
static void render_level();
static void draw_block();
static int handle_input();
static void death_anim();
static struct pos_s getFood(void);
static int hitWall();
static int hitFood();
static int hitSelf();
static void renderHighscore();
static int showHighscore();
static uint32_t highscore_get();
static char* highscore_filename();

int points = 0;
int highscore = 0;
gametype_e gametype = SNAKE_STANDARD;
struct snake_s snake = { NULL, 3, 0, MIN_SPEED, 2};
struct pos_s food;

void ram(void)
{
    while (1) {

        int c=0, pos=0,del=0;

        struct pos_s tail[MAX_SNAKE_LEN];
        snake.tail = tail;

        lcdClear();

        setTextColor(0xff,0b11100000);
        DoString(0,10, "  SNAKE");

        setTextColor(0xff,0x00);
        DoString(0,RESY/2-33, "  Choose the");
        DoString(0,RESY/2-25, "  game type:");

        setTextColor(0xff,0x00);
        DoString(0, RESY/2+10, "   LEFT: standard");
        DoString(0, RESY/2+18, "  RIGHT: wrapping ");
        DoString(0, RESY/2+26, "   DOWN: quit ");

        lcdDisplay();

        int key = getInputRaw();

        while(1) {
            key = getInputWait();
            getInputWaitRelease();

            if (key&BTN_DOWN) {
                return;
            } else if (key&BTN_LEFT) {
                gametype = SNAKE_STANDARD;
                break;
            } else if (key&BTN_RIGHT) {
                gametype = SNAKE_WRAPPING;
                break;
            }
        }

        // load the highscore
        highscore = highscore_get();

        // initially reset everything
        reset();

        while (1) {
            if(!(++c % snake.speed)) {
                if (handle_input()) { //handle_input returns 1 to quit
                    break;
                }

                pos = (snake.t_start+1) % MAX_SNAKE_LEN;
                snake.tail[pos].x = snake.tail[snake.t_start].x;
                snake.tail[pos].y = snake.tail[snake.t_start].y;

                if(snake.dir == 0) {
                    snake.tail[pos].x++;
                    if(gametype == SNAKE_WRAPPING && snake.tail[pos].x>SIZE_X) snake.tail[pos].x=0;
                } else if(snake.dir == 1) {
                    snake.tail[pos].y++;
                    if(gametype == SNAKE_WRAPPING && snake.tail[pos].y>SIZE_Y) snake.tail[pos].y=0;
                } else if(snake.dir == 2) {
                    snake.tail[pos].x--;
                    if(gametype == SNAKE_WRAPPING && snake.tail[pos].x<0) snake.tail[pos].x=SIZE_X;
                } else if(snake.dir == 3) {
                    snake.tail[pos].y--;
                    if(gametype == SNAKE_WRAPPING && snake.tail[pos].y<0) snake.tail[pos].y=SIZE_Y;
                }

                snake.t_start = pos;

                del = pos - snake.len;
                if (pos < snake.len) del += MAX_SNAKE_LEN;

                // remove last, add first line
                draw_block(snake.tail[del].x, snake.tail[del].y, 0xFF);
                draw_block(snake.tail[pos].x, snake.tail[pos].y, 0b00011000);

                // check for obstacle hit..
                if (hitWall() || hitSelf()) {
                    death_anim();
                    if (showHighscore())
                        break;
                    highscore = highscore_get();
                    reset();
                } else if (hitFood())
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
}

static struct pos_s getFood(void)
{
    int i,pos;
    struct pos_s res;

tryagain:
    res.x = (getRandom() % (SIZE_X+1));
    res.y = (getRandom() % (SIZE_Y+1));

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
    for (i=MIN_X-2; i<MAX_X+2; i++) {
        lcdSetPixel(i,MIN_Y-2,0b000101011);
        lcdSetPixel(i,MAX_Y+2,0b000101011);
        if (gametype != SNAKE_WRAPPING) {
            lcdSetPixel(i,MIN_Y-1,0b000101011);
            lcdSetPixel(i,MAX_Y+1,0b000101011);
        }
    }

    for (i=MIN_Y-2; i<MAX_Y+2; i++) {
        lcdSetPixel(MIN_X-2,i,0b000101011);
        lcdSetPixel(MAX_X+2,i,0b000101011);
        if (gametype != SNAKE_WRAPPING) {
            lcdSetPixel(MIN_X-1,i,0b000101011);
            lcdSetPixel(MAX_X+1,i,0b000101011);
        }
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

    food = getFood();

    // print initail tail
    draw_block(snake.tail[0].x, snake.tail[0].y, 0b00011000);
    draw_block(snake.tail[1].x, snake.tail[1].y, 0b00011000);
    draw_block(snake.tail[2].x, snake.tail[2].y, 0b00011000);

    // switch to level one
    render_level();
}

static void draw_block(int x, int y, int set)
{
    x *= SNAKE_DIM;
    x += MIN_X;
    y *= SNAKE_DIM;
    y += MIN_Y;

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
    points++;
    food = getFood();

    if(snake.len < MAX_SNAKE_LEN-1)
        snake.len++;
    if(snake.speed > MAX_SPEED)
        snake.speed--;

    render_level();
}

static void render_level()
{
    char highscore_string[20];
    char points_string[20];
    draw_block( food.x, food.y, 0b11101000);

    strcpy (points_string,"Points: ");
    strcat (points_string,IntToStr(points,6,0));

    strcpy (highscore_string,"HI: ");
    strcat (highscore_string,IntToStr(highscore,6,0));

    // Display point color based on compare with highscore
    if (points<highscore || (points==0 && highscore==0)) {
        // Black
        setTextColor(0xff,0x00);
    } else if (points==highscore) {
        // Dark Yellow
        setTextColor(0xff,0b11011000);
    } else if (points>highscore) {
        // Dark Green
        setTextColor(0xff,0b00011000);
    }
    DoString(1,1,points_string);
    setTextColor(0xff,0b00000011);
    DoString(MAX_X-44,1,highscore_string);
}

static int handle_input()
{
    int key = getInputRaw(), dir_old = snake.dir;
    static int quitWhen = 0;

    if (key&BTN_UP && dir_old != 1)
        snake.dir = 3;
    else if (key&BTN_DOWN && dir_old != 3)
        snake.dir = 1;
    else if (key&BTN_LEFT && dir_old != 0)
        snake.dir = 2;
    else if (key&BTN_RIGHT && dir_old !=2)
        snake.dir = 0;
    else if (key&BTN_ENTER) {
        if (quitWhen == 0) {
            quitWhen = _timectr + QUIT_DELAY/SYSTICKSPEED;
        } else if (_timectr > quitWhen) {
            return 1; //indicate program should quit
        }
        return 0;
    }
    quitWhen = 0;
    return 0; // program should continue running
}

static int hitWall()
{
    if (gametype == SNAKE_WRAPPING) return false;

    return ( (snake.tail[snake.t_start].x < 0)
             || (snake.tail[snake.t_start].x > SIZE_X)
             || (snake.tail[snake.t_start].y < 0)
             || (snake.tail[snake.t_start].y > SIZE_Y) );

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
        lcdFill(0b11100000);
        lcdDisplay();
        delayms(100);
        lcdFill(0xFF);
        lcdDisplay();
        delayms(100);

#ifdef SIMULATOR
        delayms(5000);
#else
        delayms(250);
#endif
    }
    ;// this is a stub

}

static char* highscore_filename()
{
    switch (gametype) {
        case SNAKE_WRAPPING:
            return "snake2.5cr";
        default:
            return "snake.5cr";
    }
}

static bool highscore_set(uint32_t score)
{
    writeFile(highscore_filename(), &score , sizeof(uint32_t));

    // old r0ket code to get highscore from the world
#if 0
    MPKT * mpkt= meshGetMessage('s');
    if(MO_TIME(mpkt->pkt)>score)
        return false;

    MO_TIME_set(mpkt->pkt,score);
    strcpy((char*)MO_BODY(mpkt->pkt),nick);
    if(GLOBAL(privacy)==0) {
        uint32touint8p(GetUUID32(),mpkt->pkt+26);
        mpkt->pkt[25]=0;
    };
#endif
    return true;
}

static uint32_t highscore_get()
{
    uint32_t score = 0;
    readFile(highscore_filename(), &score, sizeof(score));

    // old r0ket code to send highscore to the world
#if 0
    MPKT * mpkt= meshGetMessage('s');
    char * packet_nick = (char*)MO_BODY(mpkt->pkt);
    // the packet crc end is already zeroed
    if(MAXNICK<MESHPKTSIZE-2-6-1)
        packet_nick[MAXNICK-1] = 0;
    strcpy(nick, packet_nick);
    return MO_TIME(mpkt->pkt);
#endif

    return score;
}

static void renderHighscore()
{
    lcdClear();

    if (points>highscore) {
        setTextColor(0xff,0b11100000);
        DoString(0,10, "  NEW HIGHSCORE");
    }

    setTextColor(0xff,0x00);
    DoString(0,RESY/2-33, "  Your Score");

    // Display own score in green color, if it's higher than high score, else red
    if (points>highscore) {
        setTextColor(0xff,0b00011100);
    } else {
        setTextColor(0xff,0b11100000);
    }

    DoString(RESX/2-4, RESY/2-25, IntToStr(points,6,0));
    setTextColor(0xff,0x00);
    DoString(0,RESY/2-10, "  Last Highscore");
    setTextColor(0xff,0b00000011);
    DoString(RESX/2-4, RESY/2-2, IntToStr(highscore,6,0));
    setTextColor(0xff,0x00);
    DoString(0, RESY/2+18, "   UP to play");
    DoString(0, RESY/2+26, "RIGHT to reset HI ");
    DoString(0, RESY/2+34, " DOWN to quit ");

    lcdDisplay();
}

static int showHighscore()
{
    int key = getInputRaw();

    renderHighscore();

    if (points>highscore) {
        // Save highscore if higher
        highscore_set(points);
    }

    while(1) {
        key = getInputWait();
        getInputWaitRelease();

        if (key&BTN_DOWN) {
            return 1;
        } else if (key&BTN_UP) {
            return 0;
        } else if (key&BTN_RIGHT) {
            highscore = 0;
            highscore_set(highscore);
            setTextColor(0xff,0b00000011);
            DoString(RESX/2-4, RESY/2-2, IntToStr(highscore,6,0));
        }

        renderHighscore();
    }
}

static int hitFood()
{
    return ((snake.tail[snake.t_start].x == food.x) && (snake.tail[snake.t_start].y == food.y)) ? 1 : 0;
}
