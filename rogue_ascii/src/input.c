#include <SDL/SDL.h>
#include "rogue.h"


SDL_Event event;
bool capslock = FALSE;
bool numlock = FALSE;
bool scrollock = FALSE;
#define ALT_F9	 0xf0
extern SDL_Surface *screen, *saved_screen;
void draw_level(SDL_Surface *screen);
void sleep(void);


int handle_keyboard_inputs(void)
{
    int ch = -1;
    if(SDL_PollEvent(&event) == 1)
    //SDL_WaitEvent(&event);
    {
            switch(event.type)
            {
                case SDL_QUIT:
                    playing = FALSE;
                    ch = 1;
                    break;
                case SDL_KEYDOWN:
                    /*printf("Key_Name: %s \n", SDL_GetKeyName(event.key.keysym.sym));
                    if(event.key.keysym.unicode < 0x80 && event.key.keysym.unicode > 0)
                    {
                        printf("%c (0x%04X) \n", (char)event.key.keysym.unicode, event.key.keysym.unicode);
                    }*/
                    if(event.key.keysym.unicode && event.key.keysym.unicode > 0)
                        ch = event.key.keysym.unicode & 0x7F;
                    else
                    {
                        ch = event.key.keysym.sym;
                        if(event.key.keysym.sym == SDLK_F9 && (event.key.keysym.mod & KMOD_ALT))
                            ch = ALT_F9;
                        else if(event.key.keysym.sym == SDLK_F12)
                            {draw_level(screen);SDL_Flip(screen);sleep();}
                        else if(event.key.keysym.sym == SDLK_F11){
                            pstats.s_hpt = max_hp = 999;
                            pstats.s_maxhp = 999;
                            pstats.s_str = 999; max_stats.s_str = 999;}
                    }
                    switch(event.key.keysym.sym)
                    {
                        case SDLK_CAPSLOCK: capslock = !capslock ;break;
                        case SDLK_NUMLOCK: numlock = !numlock ;break;
                        case SDLK_SCROLLOCK: scrollock = !scrollock ;break;
                    }
                    break;
            }
    }
    return ch;
}


