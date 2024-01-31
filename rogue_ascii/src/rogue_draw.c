#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL.h>
#include "font.h"
#include "rogue.h"
#include "curses.h"

#define YOFFSET		0
#define BROWN       0x808000
#define GREY        0xc0c0c0
#define RED         0xff0000
#define CYAN        0x00ffff
#define GREEN       0x00ff00
#define YELLOW      0xffff00
#define BLUE        0x0000ff
#define ORANGE      0xffa5ff
#define WHITE       0xffffff
#define BLACK       0x000000
#define DARK_RED    0x540000
#define MAGENTA     0xff00ff
#define NORMAL  GREY
#define HIGH_INTENSITY  WHITE

#define GET_FG_COLOR  ((ch_attr) & 15)
#define GET_BG_COLOR  (((ch_attr) >> 4) & 15)

Uint32 rogue_colors[] = {
    BLACK, //7,  /*  0 normal         */
    BLUE, //2,  /*  1 green          */
    GREEN, //3,  /*  2 cyan           */
    CYAN, //4,  /*  3 red            */
    RED, //5,  /*  4 magenta        */
    MAGENTA, //6,  /*  5 brown          */
    BROWN, //8,  /*  6 dark grey      */
    NORMAL, //9,  /*  7 light blue     */
    GREY, //10, /*  8 light green    */
    BLUE, //12, /*  9 light red      */
    GREEN, //13, /* 10 light magenta  */
    ORANGE, //14, /* 11 yellow         */
    RED, //15, /* 12 uline          */
    MAGENTA, //1,  /* 13 blue           */
    YELLOW, //112,/* 14 reverse        */
    WHITE //15, /* 15 high intensity */
   //112, /* bold				 */
    //0   /* no more           */
} ;

extern int c_row, c_col, ch_attr;
extern SDL_Surface *screen;

void print_rogue_char(SDL_Surface *surface, int x, int y, unsigned char c)
{
    SDL_Rect pos = {c_col*CHAR_WIDTH, c_row*CHAR_HEIGHT, CHAR_WIDTH, CHAR_HEIGHT};
    pos.x = c_col*CHAR_WIDTH; pos.y = c_row*CHAR_HEIGHT;
    SDL_FillRect(screen, &pos, rogue_colors[GET_BG_COLOR]);
    print_char_colored(screen, x, y, c, rogue_colors[GET_FG_COLOR]);
}

void render_rooms(SDL_Surface *screen)
{
    int x, y;
    //SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));

    for(y = 0; y < MAXLINES-3; y++)
    {
        for(x = 0; x < MAXCOLS; x ++)
        {
            switch(chat(y,x))
            {
                case VWALL:
                case HWALL:
                case ULWALL:
                case URWALL:
                case LLWALL:
                case LRWALL:print_char_colored(screen, x*CHAR_WIDTH, (y+YOFFSET)*CHAR_HEIGHT, chat(y,x), BROWN);break;
                case FLOOR:print_char_colored(screen, x*CHAR_WIDTH, (y+YOFFSET)*CHAR_HEIGHT, chat(y,x), GREEN);break;
                default:
                    break;
            }
        }
    }
}

void render_passages(SDL_Surface *screen)
{
    int x, y;

    for(y = 0; y < MAXLINES-3; y++)
    {
        for(x = 0; x < MAXCOLS; x ++)
        {
            switch(chat(y,x))
            {
                case PASSAGE: print_char_colored(screen, x*CHAR_WIDTH, (y+YOFFSET)*CHAR_HEIGHT, chat(y,x), GREY);break;
                case DOOR: print_char_colored(screen, x*CHAR_WIDTH, (y+YOFFSET)*CHAR_HEIGHT, chat(y,x), BROWN);break;
            }
        }
    }
}

void render_maze(SDL_Surface *screen)
{
    int x, y;

    for(y = 0; y < MAXLINES-3; y++)
    {
        for(x = 0; x < MAXCOLS; x ++)
        {
            if(chat(y,x) == PASSAGE && (flat(y, x) & (F_MAZE|F_REAL)) == (F_MAZE|F_REAL))
            {
                print_char_colored(screen, x*CHAR_WIDTH, (y+YOFFSET)*CHAR_HEIGHT, chat(y,x), 0XFF00FF);
                //if(flat(y, x) & 2)
                    //print_char_colored(screen, x*CHAR_WIDTH, y*CHAR_HEIGHT, chat(y,x), 0XFFFF00);
            }
        }
    }
}

void render_objects(SDL_Surface *screen)
{
    int x, y;

    for(y = 0; y < MAXLINES-3; y++)
    {
        for(x = 0; x < MAXCOLS; x ++)
        {
            switch(chat(y,x))
            {
                case TRAP: print_char_colored(screen, x*CHAR_WIDTH, (y+YOFFSET)*CHAR_HEIGHT, TRAP, MAGENTA);break;
                case GOLD: print_char_colored(screen, x*CHAR_WIDTH, (y+YOFFSET)*CHAR_HEIGHT, GOLD, YELLOW);break;
                case FOOD: print_char_colored(screen, x*CHAR_WIDTH, (y+YOFFSET)*CHAR_HEIGHT, FOOD, RED);break;
                case POTION:
                case SCROLL:
                case STICK:
                case ARMOR:
                case AMULET:
                case RING:
                case WEAPON: print_char_colored(screen, x*CHAR_WIDTH, (y+YOFFSET)*CHAR_HEIGHT, chat(y,x), BLUE);break;
                case MAGIC:
                case BMAGIC:
                //case CALLABLE:
                case STAIRS: print_char_colored(screen, x*CHAR_WIDTH, (y+YOFFSET)*CHAR_HEIGHT, chat(y,x), GREEN);break;
            }
            if(moat(y,x))
                print_char_colored(screen, x*CHAR_WIDTH, (y+YOFFSET)*CHAR_HEIGHT, moat(y,x)->t_disguise, GREY);
        }
    }
}

void draw_symbols(SDL_Surface *screen)
{
    int x=10, y=10;
    print_char_colored(screen, (x++)*CHAR_WIDTH, y, TRAP, MAGENTA);
    print_char_colored(screen, (x++)*CHAR_WIDTH, y, GOLD, YELLOW);
    print_char_colored(screen, (x++)*CHAR_WIDTH, y, POTION, BLUE);
    print_char_colored(screen, (x++)*CHAR_WIDTH, y, SCROLL, BLUE);
    print_char_colored(screen, (x++)*CHAR_WIDTH, y, MAGIC, GREEN);
    print_char_colored(screen, (x++)*CHAR_WIDTH, y, BMAGIC, GREEN);
    print_char_colored(screen, (x++)*CHAR_WIDTH, y, FOOD, RED);
    print_char_colored(screen, (x++)*CHAR_WIDTH, y, STICK, BLUE);
    print_char_colored(screen, (x++)*CHAR_WIDTH, y, ARMOR, BLUE);
    print_char_colored(screen, (x++)*CHAR_WIDTH, y, AMULET, BLUE);
    print_char_colored(screen, (x++)*CHAR_WIDTH, y, RING, BLUE);
    print_char_colored(screen, (x++)*CHAR_WIDTH, y, WEAPON, BLUE);
}

void draw_level(SDL_Surface *screen)
{
    render_rooms(screen);
    render_passages(screen);
    render_maze(screen);
    render_objects(screen);
    //draw_symbols(screen);
    print_char_colored(screen, oldpos.x*CHAR_WIDTH, (oldpos.y+YOFFSET)*CHAR_HEIGHT, PLAYER, YELLOW);
}

