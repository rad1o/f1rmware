/*
 * A Tetris clone for Rad1o
 *
 * by lavisrap   
 ***************************************/

#include <string.h>


#include <r0ketlib/config.h>
#include <r0ketlib/display.h>
#include <r0ketlib/fonts.h>
#include <r0ketlib/render.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>

#include "invfont.c"

#include "usetable.h"

#define MAX_X (130)
#define MAX_Y (130)
#define BLOCK_SIZE (6)
#define ROW_OFFSET (10)
#define COL_OFFSET (10)
#define BLOCKS_PER_COL (20)
#define BLOCKS_PER_ROW (10)
#define BLOCKS_CNT (4)

#define RIGHT 0
#define LEFT 2
#define UP 3
#define DOWN 1

struct pos {
  int x,y;
};

struct brc {
    int row, col;
};

static int BLOCKS[7][4][8];
static int colors[7];

int bl[BLOCKS_PER_COL][BLOCKS_PER_ROW];
int lastBlocks[8];
struct Tile {
    int type;
    int rotation;
    struct pos pos;
};

struct Tile currentTile;

static void init();
static void newGame();
static bool handleInput();
static bool checkPos();
static void getNewTile();
static void drawTile();
static bool placeTile();
static void drawBlock();
static void theEnd();
static void setHighscore();
static int getHighscore();
int atoi();

int score;
int highscore;
int c;
int speed;
int lastKey;
int stop;


void ram(void)
{
    highscore = getHighscore();

    init();
    // initially reset everything
    newGame();

    while (1)
    {
        //DoString(80,60,stop?"yes":"no");
        //DoString(80,70,IntToStr(speed,6,0));
        //DoString(80,80,IntToStr(c,6,0));
        //lcdDisplay();
        if(!(++c % speed) && !stop) {
            if( checkPos( currentTile.pos.x, currentTile.pos.y+1, currentTile.rotation ) ) {
                currentTile.pos.y++;
            } else {
                if( placeTile() ) {
                    stop = 1;
                    drawTile();
                    theEnd();
                } else {
                    speed--;
                    getNewTile();
                }
            }

            drawTile();
        }
        
        if( handleInput() ) {
            drawTile();
        }
        delayms(3);
    }
}

static bool handleInput()
{
    struct Tile ct = currentTile;
    int key = getInputRaw();
    bool ret = false;

    if (lastKey != key) {
        if(key&BTN_ENTER || key&BTN_UP) {
            int rotation = (ct.rotation+1)%4; 
            if( checkPos( ct.pos.x, ct.pos.y, rotation ) ) {
                currentTile.rotation = rotation;
                ret = true;
            } else {
                if( checkPos( ct.pos.x-1, ct.pos.y, rotation ) ) {
                    currentTile.rotation = rotation;
                    currentTile.pos.x--;
                    ret = true;
                } else if(checkPos( ct.pos.x+1, ct.pos.y, rotation ) ) {
                    currentTile.rotation = rotation;
                    currentTile.pos.x++;
                    ret = true;
                } 
            }
        } else if( key&BTN_LEFT ) {
            if( checkPos(ct.pos.x-1,ct.pos.y,ct.rotation) ) {
                currentTile.pos.x--;
                ret = true;
            }
        } else if( key&BTN_RIGHT ) {
            if( checkPos(ct.pos.x+1,ct.pos.y,ct.rotation) ) {
                currentTile.pos.x++;
                ret = true;
            } 
        } else if( key&BTN_DOWN ) {
            int i;
            for( i=ct.pos.y+1 ; i<BLOCKS_PER_COL && checkPos(ct.pos.x, i,ct.rotation) ; i++ );

            currentTile.pos.y = i-1;
            ret = true;
        }
    }
    lastKey = key;

    return ret;
};

static bool checkPos(int x, int y, int rotation) {
    struct Tile ct = currentTile;

    if( ct.type < 0 || ct.type >= 7 || rotation < 0 || rotation >= 4 ) return true;

    int *tile = BLOCKS[ct.type][rotation],
        i;

    for( i=0 ; i<BLOCKS_CNT ; i++ ) {
        int x1 = x+tile[i*2],
            y1 = y+tile[i*2+1];

        if(x1 < 0 || x1 >= BLOCKS_PER_ROW) {
            return false;
        }
        if(y1 >= BLOCKS_PER_COL ) {
            return false;
        }
        if( y1 >= 0 && bl[y1][x1] != 0 ) {
            return false;
        }
    }

    return true;
};

static void getNewTile() {
    currentTile.type = getRandom()%7;
    currentTile.rotation = getRandom()%4;
    currentTile.pos.x = 5;
    currentTile.pos.y = -2;
}; 

static bool placeTile() {
    int i,j,i1,j1,
        minY = BLOCKS_PER_COL-1,
        maxY = 0;
    struct Tile ct = currentTile;
    
    if( ct.type < 0 || ct.type >= 7 || ct.rotation < 0 || ct.rotation >= 4 ) return false;
    
    int *tile = BLOCKS[ct.type][ct.rotation];
    
    lastBlocks[0] = -1;
    
    for( i=0 ; i<BLOCKS_CNT ; i++ ) {
        int x = ct.pos.x + tile[i*2],
            y = ct.pos.y + tile[i*2+1];
       
        if( x >= 0 && x < BLOCKS_PER_ROW && y >= 0 && y < BLOCKS_PER_COL ) bl[y][x] = colors[ct.type]; 
        if( minY > y ) minY = y;
        if( maxY < y ) maxY = y;
    }

    if( maxY <= 1 ) return true;

    int redraw = false;
    for( i=minY ; i<=maxY ; i++ ) {
        int complete = true,
            cnt = 0;
        for( j=0 ; j<BLOCKS_PER_ROW ; j++ ) {
            if( bl[i][j] == 0 ) {
                complete = false;
                cnt++;
            }
        }
        if( complete ) {
            for( i1=i ; i1>=1 ; i1-- ) for( j1=0 ; j1<BLOCKS_PER_ROW ; j1++ ) bl[i1][j1] = bl[i1-1][j1];
            score += BLOCKS_PER_COL-i;
            redraw = true;
        }
    }

    if( redraw == true ) {
        for( i=maxY ; i>=1 ; i-- ) {
            for( j=0 ; j<BLOCKS_PER_ROW ; j++ ) {
                int color = bl[i][j];
                if( color == 0 ) color = 0xff;
                drawBlock(j,i,color); 
            }
        }

        DoString(85,30,IntToStr(score,6,0));
        if( score > highscore ) {
            highscore = score;
            DoString(85,60,IntToStr(highscore,6,0));
        }   
    }

    lcdDisplay();
    return false;
};

static void init() {

    int i,j,k;
    static int blocks[7][4][8] = {{
            { -2, 0,  -1, 0,   0, 0,   1, 0 },
            {  0,-1,   0, 0,   0, 1,   0, 2 },
            { -2, 0,  -1, 0,   0, 0,   1, 0 },
            {  0,-1,   0, 0,   0, 1,   0, 2 }
        },{
            { -1,-1,  -1, 0,   0, 0,   0,-1 }, 
            { -1,-1,  -1, 0,   0, 0,   0,-1 }, 
            { -1,-1,  -1, 0,   0, 0,   0,-1 }, 
            { -1,-1,  -1, 0,   0, 0,   0,-1 } 
        },{
            { -1, 0,   0, 0,   1, 0,   1, 1 },
            {  0,-1,   0, 0,   0, 1,  -1, 1 },
            { -1,-1,  -1, 0,   0, 0,   1, 0 },
            {  0,-1,   1,-1,   0, 0,   0, 1 }
        },{
            { -1, 0,   0, 0,   1, 0,   1,-1 },
            {  0,-1,   0, 0,   0, 1,   1, 1 },
            { -1, 1,  -1, 0,   0, 0,   1, 0 },
            {  0,-1,  -1,-1,   0, 0,   0, 1 }
        },{
            { -1, 0,   0, 0,   1, 0,   0,-1 },
            {  0,-1,   0, 0,   0, 1,   1, 0 },
            { -1, 0,   0, 0,   1, 0,   0, 1 },
            {  0,-1,   0, 0,   0, 1,  -1, 0 }
        },{
            { -1, 0,   0, 0,   0, 1,   1, 1 },
            {  0,-1,   0, 0,  -1, 0,  -1, 1 },
            { -1, 0,   0, 0,   0, 1,   1, 1 },
            {  0,-1,   0, 0,  -1, 0,  -1, 1 },
        },{
            { -1, 0,   0, 0,   0,-1,   1,-1 },
            {  0,-1,   0, 0,   1, 0,   1, 1 },
            { -1, 0,   0, 0,   0,-1,   1,-1 },
            {  0,-1,   0, 0,   1, 0,   1, 1 }
        }};

    for( i=0 ; i<7 ; i++ ) for( j=0 ; j<4 ; j++ ) for( k=0 ; k<8 ; k++ ) BLOCKS[i][j][k] = blocks[i][j][k];

    static int c[7] = { 0b0000000111, 0b111000000, 0b000111000, 0b111111000, 0b000111111, 0b111000111, 0b010101010 }; 
    for( i=0 ; i<7 ; i++ ) colors[i] = c[i];

    lastBlocks[0] = -1;
};

static void newGame() {
    int i,j;

    currentTile.type = -1;
    score = 0;
    stop = 0;
    speed = 333;
    lastKey = 0;
    stop = 0;

    // setup the screen
    lcdClear();
    for (i=0 ; i<MAX_Y; i++) {
        lcdSetPixel(COL_OFFSET + BLOCKS_PER_ROW * BLOCK_SIZE,i,0b000101011);
        lcdSetPixel(COL_OFFSET + BLOCKS_PER_ROW * BLOCK_SIZE + 2,i,0b000101011);
    }
    for (i=0 ; i<MAX_Y; i++) {
        lcdSetPixel(COL_OFFSET-1,i,0b000101011);
        lcdSetPixel(COL_OFFSET-3,i,0b000101011);
    } 

    for(i=0 ; i<BLOCKS_PER_COL ; i++ ) {
        for( j=0 ; j<BLOCKS_PER_ROW ; j++ ) {
            bl[i][j] = 0;
        }
    }

    DoString(85,20,"Score");
    DoString(85,30,IntToStr(score,6,0));
    DoString(85,50,"High");
    DoString(85,60,IntToStr(highscore,6,0));
    getNewTile();

    lcdDisplay();
}

static void drawTile() {
    int i;
    struct Tile ct = currentTile;
    
    if( ct.type < 0 || ct.type >= 7 || ct.rotation < 0 || ct.rotation >= 4 ) return;
    
    int *tile = BLOCKS[ct.type][ct.rotation];
    struct pos pos = currentTile.pos;

    if( lastBlocks[0] != -1 ) {
        for( i=0 ; i<BLOCKS_CNT ; i++) {
            drawBlock( lastBlocks[i*2], lastBlocks[i*2+1], 0xff );
        }
    }

    for( i=0 ; i<BLOCKS_CNT ; i++ ) {
        int x = pos.x + tile[i*2],
            y = pos.y + tile[i*2+1];
        
        drawBlock( x, y, colors[ct.type] );
        lastBlocks[i*2] = x;
        lastBlocks[i*2+1] = y;
    }
    
    lcdDisplay();
}

static void drawBlock(int x, int y, int f) {
  x = x * BLOCK_SIZE + COL_OFFSET;
  y = y * BLOCK_SIZE + ROW_OFFSET;

  lcdSetPixel(x  , y,   f);
  lcdSetPixel(x+1, y,   f);
  lcdSetPixel(x+2, y,   f);
  lcdSetPixel(x+3, y,   f);
  lcdSetPixel(x+4, y,   f);
  lcdSetPixel(x+5, y,   f);
  lcdSetPixel(x,   y+1, f);
  lcdSetPixel(x+1, y+1, f);
  lcdSetPixel(x+2, y+1, f);
  lcdSetPixel(x+3, y+1, f);
  lcdSetPixel(x+4, y+1, f);
  lcdSetPixel(x+5, y+1, f);
  lcdSetPixel(x,   y+2, f);
  lcdSetPixel(x+1, y+2, f);
  lcdSetPixel(x+2, y+2, f);
  lcdSetPixel(x+3, y+2, f);
  lcdSetPixel(x+4, y+2, f);
  lcdSetPixel(x+5, y+2, f);
  lcdSetPixel(x,   y+3, f);
  lcdSetPixel(x+1, y+3, f);
  lcdSetPixel(x+2, y+3, f);
  lcdSetPixel(x+3, y+3, f);
  lcdSetPixel(x+4, y+3, f);
  lcdSetPixel(x+5, y+3, f);
  lcdSetPixel(x,   y+4, f);
  lcdSetPixel(x+1, y+4, f);
  lcdSetPixel(x+2, y+4, f);
  lcdSetPixel(x+3, y+4, f);
  lcdSetPixel(x+4, y+4, f);
  lcdSetPixel(x+5, y+4, f);
  lcdSetPixel(x,   y+5, f);
  lcdSetPixel(x+1, y+5, f);
  lcdSetPixel(x+2, y+5, f);
  lcdSetPixel(x+3, y+5, f);
  lcdSetPixel(x+4, y+5, f);
  lcdSetPixel(x+5, y+5, f);
}

static void theEnd() {
    DoString(32,75,"GAME OVER");
    DoString(15,100,"Press button...");
    lcdDisplay();
    
    setHighscore(highscore);
    
    while(1) {
        int key = getInputRaw();
        if( key != 0 ) {
            newGame();
            stop = 0;
            return;
        }
    }
}

static void setHighscore(int score) {
    char filecontent[7];

    strcpy(filecontent, IntToStr(score,6,0));
    filecontent[6] = 0;

    writeFile("tetris.6cr",filecontent,7);
}

static int getHighscore() {
  char filecontent[7];
  readTextFile("tetris.6cr",filecontent,7);
  
  // I don't get it, but we have to somehow read the variable, otherwise atoi will return 0... o_O
  strlen(filecontent);
  return atoi(filecontent);
}

