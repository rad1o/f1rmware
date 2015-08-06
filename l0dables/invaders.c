#include <stdint.h>
#include <string.h>

#include <r0ketlib/display.h>
#include <r0ketlib/fonts.h>
#include <r0ketlib/render.h>
#include <r0ketlib/fonts/smallfonts.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>

#include "invfont.c"

//FIXME: temporary hacks
#define GLOBAL(nickname) "Sec"
#define delayms(x) delay(x*100)
#define delayms_queue(x) delay(x*100)
#define getRandom(x) (0)

#include "usetable.h"
/**************************************************************************/
#define POS_PLAYER_Y RESY-8
#define POS_PLAYER_X RESX/2-3
#define POS_UFO_Y 0
#define ENEMY_ROWS 3
#define ENEMY_COLUMNS 6
#define DISABLED 255

#define UFO_PROB 1024

#define TYPE_PLAYER  1
#define TYPE_ENEMY_A 3
#define TYPE_ENEMY_B 2
#define TYPE_ENEMY_C 4
#define TYPE_UFO     5

#define BUNKERS 4
#define BUNKER_WIDTH  10
#define BUNKER_X(b) ( RESX / (BUNKERS+1) * (b+1) - BUNKER_WIDTH/2 )
static const uint8_t ENEMY_WIDTHS[] = {8,10,12};

struct gamestate {
    char player;
	char ufo;
    char shot_x, shot_y;
	char shots_x[ENEMY_COLUMNS];
	char shots_y[ENEMY_COLUMNS];
    char alive;
    int16_t move;
	char direction, lastcol;
    bool killed;
	bool step;
	uint32_t score;
	uint16_t level;
	int8_t rokets;
    char enemy_x[ENEMY_ROWS][ENEMY_COLUMNS];
    char enemy_row_y[ENEMY_ROWS];
    uint8_t bunker[BUNKERS][BUNKER_WIDTH];
} game;
char key;

static bool highscore_set(uint32_t score, char nick[]);
static uint32_t highscore_get(char nick[]);
static void init_game();
static void init_enemy();
static void check_end();
static void move_ufo();
static void move_shot();
static void move_shots();
static void move_player();
static void move_enemy();
static void draw_score();
static void draw_bunker();
static void draw_player();
static void draw_enemy();
static void draw_shots();
static void draw_sprite(char type, char x, char y);
static void draw_ufo();
static bool screen_intro();
static bool screen_gameover();
static void screen_level();
static bool check_bunker(char xpos, char ypos, int8_t shift);

void ram(void) {
	while(1) {
		if (!screen_intro()){
      setTextColor(0xff,0x00);
      return;
    }
		game.rokets = 3;
		game.level = 1;
		game.score = 0;
		init_game();
		screen_level();
		while (game.rokets>=0) {
			////checkISP();
			lcdFill(0x00);
			check_end();
			move_ufo();
			move_shot();
			move_shots();
			move_player();
			move_enemy();
			draw_score();
			draw_ufo();
			draw_bunker();
			draw_player();
			draw_enemy();
			draw_shots();
			//        draw_status();
			lcdDisplay();
			delayms(12);
		}
		if (!screen_gameover())
      setTextColor(0xff,0x00);
      return;
	}
}

static bool screen_intro() {
	uint32_t highscore;
	char highnick[20];
	char key=0;
	bool step = false;
	//getInputWaitRelease();
	while(key==0) {
		getInputWaitRelease();
		lcdFill(0x00);
		setIntFont(&Font_Invaders);
    setTextColor(0x00,0b11100000);
    lcdSetCrsr(28+18,25+15);lcdPrint(step?"ABC":"abc");
		setIntFont(&Font_7x8);
    setTextColor(0x00,0b00011000);
		lcdSetCrsr(28+18,40+15);lcdPrint("SPACE");
    setTextColor(0x00,0b11111000);
		lcdSetCrsr(18+18,50+15);lcdPrint("INVADERS");

		highscore = highscore_get(highnick);
    setTextColor(0x00,0xff);
		lcdSetCrsr(0,0);lcdPrint(IntToStr(highscore,6,F_LONG));
//		lcdSetCrsr(0,9);lcdPrint(highnick);
		lcdDisplay();
		step = !step;
		key=getInputWaitTimeout(1000);
	}
	//getInputWaitRelease();
	return !(key==BTN_LEFT);
}

static bool screen_gameover() {
	char key =0;
	while(key==0) {
		lcdFill(0x00);
		setIntFont(&Font_7x8);
    setTextColor(0x00,0b11100000);
		lcdSetCrsr(14+15,32+15);lcdPrint("GAME OVER");
    setTextColor(0x00,0xff);
		lcdSetCrsr(0,0);lcdPrint(IntToStr(game.score,6,F_LONG));
		if (highscore_set(game.score, GLOBAL(nickname))){
      setTextColor(0x00,0b00011000);
      lcdSetCrsr(0,9);lcdPrint("HIGHSCORE!");
		};
		lcdDisplay();
		key=getInputWaitTimeout(5000);
	}
	//getInputWaitRelease();
	return !(key==BTN_LEFT);
}

static void screen_level() {
	lcdFill(0x00);
	draw_score();
	setIntFont(&Font_7x8);
	lcdSetCrsr(20,32);
  setTextColor(0x00,0xff);
	lcdPrint("Level ");
	lcdPrint(IntToStr(game.level,3,0));
	lcdDisplay();
	delayms_queue(500);
}

static bool highscore_set(uint32_t score, char nick[]) {
#if 0
    MPKT * mpkt= meshGetMessage('i');
    if(MO_TIME(mpkt->pkt)>score)
        return false;

    MO_TIME_set(mpkt->pkt,score);
    strcpy((char*)MO_BODY(mpkt->pkt),nick);
    if(GLOBAL(privacy)==0){
        uint32touint8p(GetUUID32(),mpkt->pkt+26);
        mpkt->pkt[25]=0;
    };
#endif
	return true;
}

static uint32_t highscore_get(char nick[]){
#if 0
    MPKT * mpkt= meshGetMessage('i');
    char * packet_nick = (char*)MO_BODY(mpkt->pkt);
    // the packet crc end is already zeroed
    if(MAXNICK<MESHPKTSIZE-2-6-1)
        packet_nick[MAXNICK-1] = 0;
    strcpy(nick, packet_nick);
	return MO_TIME(mpkt->pkt);
#else
	return 0;
#endif
}

static void init_game(void) {
	game.player = POS_PLAYER_X;
	game.shot_x = DISABLED;
	game.shot_y = 0;
	game.alive = ENEMY_ROWS*ENEMY_COLUMNS;
	game.move = 0;
	if (getRandom()%2 == 0) {
		game.direction = -1;
		game.lastcol = ENEMY_COLUMNS-1;
	} else {
		game.direction = 1;
		game.lastcol = 0;
	}
	game.killed = 0;
	game.step = false;
	game.ufo = DISABLED;
	init_enemy();

	for (int col=0; col<ENEMY_COLUMNS; col++){
	   game.shots_x[col] = DISABLED;
   	}

	for (int b=0; b<BUNKERS; b++){
		//for (int slice=0; slice<BUNKER_WIDTH; slice++){
		//	game.bunker[b][slice] = 255<<2;
		//}
		game.bunker[b][0] = 0b00111100;
		game.bunker[b][1] = 0b01111100;
		game.bunker[b][2] = 0b11111100;
		game.bunker[b][3] = 0b11100000;
		game.bunker[b][4] = 0b11100000;
		game.bunker[b][5] = 0b11100000;
		game.bunker[b][6] = 0b11100000;
		game.bunker[b][7] = 0b11111100;
		game.bunker[b][8] = 0b01111100;
		game.bunker[b][9] = 0b00111100;
	}
}

static void init_enemy() {
    for (int row = 0; row<ENEMY_ROWS; row++) {
        game.enemy_row_y[row] = 10 + (40/ENEMY_ROWS)*row;
        for (int col = 0; col<ENEMY_COLUMNS; col++) {
            game.enemy_x[row][col] = 5+(86/ENEMY_COLUMNS)*col+(2-row);
        }
    }
}

static bool check_bunker(char xpos, char ypos, int8_t shift){
	for (int b=0; b<BUNKERS; b++) {
		if (xpos>BUNKER_X(b) &&
				xpos<BUNKER_X(b)+BUNKER_WIDTH &&
				ypos<RESY-8 &&
				ypos>RESY-16) {
			int offset = xpos-BUNKER_X(b);
			if (game.bunker[b][offset]!=0) {
				if (shift>0)
					game.bunker[b][offset]&=game.bunker[b][offset]<<shift;
				else
					game.bunker[b][offset]&=game.bunker[b][offset]>>-shift;
				return true;
			}
		}
	}
	return false;
}

static void move_shot() {
    //No shot, do nothing
    if(game.shot_x == DISABLED) {
        return;
    }

    //moving out of top, end shot
    if (game.shot_y <= 0) {
            game.shot_x = DISABLED;
            return;
    }

   if (check_bunker(game.shot_x,game.shot_y-5,1 ))
		game.shot_x=DISABLED;

    //check for collision with enemy, kill enemy if
    for (int row=0; row<ENEMY_ROWS; row++) {
        if (game.enemy_row_y[row]+6 >= game.shot_y && game.enemy_row_y[row]+6 < game.shot_y+7) {
            for(int col = 0; col<ENEMY_COLUMNS; col++) {
                if(game.shot_x >= game.enemy_x[row][col] && game.shot_x < game.enemy_x[row][col]+ENEMY_WIDTHS[row]) {
                    game.enemy_x[row][col]=DISABLED;
                    game.shot_x = DISABLED;
                    game.alive--;
					game.score+=(3-row)*10;
                    return;
                }
            }
        }
    }

    //check for collision with ufo
   	if (game.ufo != DISABLED &&
		game.shot_x>game.ufo &&
		game.shot_x<game.ufo + 16 &&
		game.shot_y<8) {

		game.ufo = DISABLED;
		game.score += 50;
	}

    game.shot_y -= 2;
}




static void move_shots() {
    for (int col = 0; col<ENEMY_COLUMNS; col++){
		//No shot, maybe generate
		if (game.shots_x[col] == DISABLED) {
			for (int row = 0; row<ENEMY_ROWS; row++) {
				if (game.enemy_x[row][col] != DISABLED) {
					if(getRandom()%(game.alive*20/((game.level/3)+1))==0) {
						game.shots_x[col] = game.enemy_x[row][col]+5;
						game.shots_y[col] = game.enemy_row_y[row]+0;
					}
				}
			}
			continue;
		}

		//moving out of bottm, end shot
		if (game.shots_y[col] >= RESY) {
			game.shots_x[col] = DISABLED;
			return;
		}
		//check for collision with bunker
        if (check_bunker(game.shots_x[col],game.shots_y[col],-1))
			game.shots_x[col]=DISABLED;

		//check for collision with player
		if (game.shots_y[col] >= RESY-13 &&
			game.shots_x[col] > game.player+1 &&
			game.shots_x[col] < game.player+6) {

			game.killed = true;
		}

		//move shots down
		game.shots_y[col] += 1;
	}
}

static void move_ufo() {
	if (game.ufo == DISABLED) {
		if ((getRandom()%UFO_PROB)==0) {
			game.ufo = 0;
		}
		return;
	}
	if (game.ufo >= RESX){
		game.ufo = DISABLED;
		return;
	}
	game.ufo++;
}

static void move_player() {
	switch(getInputRaw()){
		case BTN_LEFT:
			if(game.player>0)
				game.player-=1;
			break;
		case BTN_RIGHT:
			if(game.player <RESX-8)
				game.player+=1;
			break;
		case BTN_ENTER:
			if(game.shot_x == 255){
				game.shot_x = game.player+4;
				game.shot_y = POS_PLAYER_Y;
			}
			break;
	};
}

static void move_enemy() {
    if(game.move > 0){
        game.move-=game.level/5+1;
        return;
    }

	game.step = !game.step;
    for (int col = 0; col < ENEMY_COLUMNS; col++) {
        for (int row = 0; row < ENEMY_ROWS; row++) {
            char pos = game.enemy_x[row][(game.direction==1)?(ENEMY_COLUMNS-(col+1)):col];
            if (pos != DISABLED) {
                //Check collision with player
                if((game.enemy_row_y[row]+8 >= POS_PLAYER_Y && pos+8 >= game.player && pos < game.player+8) ||
						game.enemy_row_y[row]+8 >= POS_PLAYER_Y+8) {
					for(int row=0; row<ENEMY_ROWS; row++) {
						game.enemy_row_y[row] = 10 + (40/ENEMY_ROWS)*row;
					}
					game.killed = true;
                }
                check_bunker(pos,game.enemy_row_y[row]+8,-2);

			    //Are we at the beginning or end? Direction change
                if((pos <=0 && game.direction != 1) ||
                   (pos >=RESX-10 && game.direction == 1)){
                    game.direction = (game.direction==1)?-1:1;
                    for (int r = 0; r<ENEMY_ROWS; r++) {
                        game.enemy_row_y[r]+=game.level>=23?4:2;
                    }
                    return;
                }
                game.enemy_x[row][(game.direction==1)?(ENEMY_COLUMNS-(col+1)):col] += game.direction;
            }
        }
    }

    game.move = game.alive*2-1;
}

static void draw_player() {
    draw_sprite(TYPE_PLAYER, game.player, POS_PLAYER_Y);
}

static void draw_ufo() {
	if (game.ufo!=DISABLED)
		draw_sprite(TYPE_UFO, game.ufo, POS_UFO_Y);
}

static void draw_enemy() {
    for (int row = 0; row<ENEMY_ROWS; row++) {
        for (int col = 0; col<ENEMY_COLUMNS; col++) {
            if (game.enemy_x[row][col] != DISABLED) {
                draw_sprite(TYPE_ENEMY_C-row,game.enemy_x[row][col],game.enemy_row_y[row]);
            }
        }
    }
}

static void draw_bunker() {
	for (int b=0; b<BUNKERS; b++) {
		for (int x=0;x<8;x++){
			for (int y=0;y<BUNKER_WIDTH;y++){
				if(game.bunker[b][y] & (1<<x)){
					lcdSetPixel(BUNKER_X(b)+y,RESY-x-8,0b00011000);
				}else{
					lcdSetPixel(BUNKER_X(b)+y,RESY-x-8,0x00);
				};
			}
		}
	};
};

static void draw_shots() {
    if (game.shot_x != 255) {
        for (int length=0; length<=5; length++) {
            lcdSetPixel(game.shot_x, game.shot_y+length, 0xff);
        }
    }

	for (int col = 0; col < ENEMY_COLUMNS; col++) {
		if (game.shots_x[col] != DISABLED) {
			for (int length=0; length<=5; length++) {
				lcdSetPixel(game.shots_x[col], game.shots_y[col]+length,0xff);
			}
		}
	}

}

static void draw_status() {
    for (int p = 0; p<game.alive; p++){
        lcdSetPixel(p+1,1,0xff);
    }
}

static void draw_sprite(char type, char x, char y) {
	setIntFont(&Font_Invaders);
	switch(type) {
		case TYPE_PLAYER:
      setTextColor(0x00,0b11111000);
			DoChar(x,y-1,'P');
			break;
		case TYPE_ENEMY_A:
      setTextColor(0x00,0b11100000);
			DoChar(x,y-1,game.step?'a':'A');
			break;
		case TYPE_ENEMY_B:
      setTextColor(0x00,0b11100000);
			DoChar(x,y-1,game.step?'b':'B');
			break;
		case TYPE_ENEMY_C:
      setTextColor(0x00,0b11100000);
			DoChar(x,y-1,game.step?'c':'C');
			break;
		case TYPE_UFO:
      setTextColor(0x00,0b11100111);
			DoChar(x,y-1,'U');
			break;
	}
}

static void draw_score() {
	setIntFont(&Font_7x8);
  setTextColor(0x00,0xff);
	lcdSetCrsr(0,0);lcdPrint(IntToStr(game.score,6,F_LONG));
	lcdSetCrsr(RESX-8,0);lcdPrint(IntToStr(game.rokets,1,0));
	setIntFont(&Font_Invaders);
	DoChar(RESX-16, 0, 'P');
}


static void check_end() {
    if (game.killed) {
		game.rokets--;
		delayms_queue(500);
        game.player = POS_PLAYER_X;

		for(int col=0; col<ENEMY_COLUMNS; col++) {
			game.shots_x[col] = DISABLED;
		}
        game.killed = false;
    }
    if (game.alive == 0) {
		delayms(500);
		game.level++;
		init_game();
		screen_level();
    }
}
