/*
 *  Cursor motion stuff to simulate a "no refresh" version of curses
 */
#include	"rogue.h"
#include	"curses.h"
#include	"curses_dos.h"
#include <SDL/SDL.h>
#include	"font.h"
#include	"util.h"

extern SDL_Surface *screen, *saved_screen;
byte video_memory [25][80];
void print_rogue_char(SDL_Surface *surface, int x, int y, unsigned char c);

int *sbrk(int* addr){return addr;}
int _dsval = 0x00;
/*
 *  Globals for curses
 */
int LINES=25, COLS=80;
int is_saved = FALSE;
bool iscuron = TRUE;
int ch_attr = 0x7;
int old_page_no;
int no_check = FALSE;

int scr_ds=0xB800;
int svwin_ds;

char *savewin;
int scr_type = -1;
int tab_size = 8;
int page_no = 0;

#define MAXATTR 17
byte color_attr[] = {
    7,  /*  0 normal         */
    2,  /*  1 green          */
    3,  /*  2 cyan           */
    4,  /*  3 red            */
    5,  /*  4 magenta        */
    6,  /*  5 brown          */
    8,  /*  6 dark grey      */
    9,  /*  7 light blue     */
    10, /*  8 light green    */
    12, /*  9 light red      */
    13, /* 10 light magenta  */
    14, /* 11 yellow         */
    15, /* 12 uline          */
    1,  /* 13 blue           */
    112,/* 14 reverse        */
    15, /* 15 high intensity */
   112, /* bold				 */
    0   /* no more           */
} ;

byte monoc_attr[] = {
      7,  /*  0 normal         */
     7,  /*  1 green          */
     7,  /*  2 cyan           */
     7,  /*  3 red            */
     7,  /*  4 magenta        */
     7,  /*  5 brown          */
     7,  /*  6 dark grey      */
     7,  /*  7 light blue     */
     7,  /*  8 light green    */
     7,  /*  9 light red      */
     7,  /* 10 light magenta  */
     7,  /* 11 yellow         */
     17,  /* 12 uline          */
     7,  /* 13 blue           */
    120,  /* 14 reverse        */
     7,  /* 15 white/hight    */
    120,  /* 16 bold		   */
    0     /* no more           */
} ;

byte *at_table;

int c_row, c_col;   /*  Save cursor positions so we don't ask dos */
int scr_row[25];

byte dbl_box[BX_SIZE] = {
	0xc9, 0xbb, 0xc8, 0xbc, 0xba, 0xcd, 0xcd
};

byte sng_box[BX_SIZE] = {
	0xda, 0xbf, 0xc0, 0xd9, 0xb3, 0xc4, 0xc4
};

byte fat_box[BX_SIZE] = {
	0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdf, 0xdc
};

byte spc_box[BX_SIZE] = {
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

/*
 * clear screen
 */
void clear()
{
	if (scr_ds == svwin_ds)
		wsetmem(savewin, LINES*COLS, 0x0720);
	else
		blot_out(0,0,LINES-1,COLS-1);
}


/*
 *  Turn cursor on and off
 */
bool cursor(ison)
	bool ison;
{
	register bool oldstate;
	//register int w_state;


	if (iscuron == ison)
		return ison;
	oldstate = iscuron;
	iscuron = ison;

	regs->ax = 0x100;
	if (ison)
	{
		regs->cx = (is_color ? 0x607 : 0xb0c);
		swint(SW_SCR, regs);
		move(c_row, c_col);
	}
	else
	{
		regs->cx = 0xf00;
		swint(SW_SCR, regs);
	}
	return(oldstate);
}


/*
 * get curent cursor position
 */
void getrc(rp,cp)
	int *rp, *cp;
{
	*rp = c_row;
	*cp = c_col;
}

void real_rc(pn, rp,cp)
	int *rp, *cp;
{
	/*
	 * pc bios: read current cursor position
	 */
	regs->ax = 0x300;
	regs->bx = pn << 8;

	swint(SW_SCR, regs);

	*rp = regs->dx >> 8;
	*cp = regs->dx & 0xff;
}

/*
 *	clrtoeol
 */
void clrtoeol()
{
	int r,c;

	//if (scr_ds == svwin_ds)
		//return;
	getrc(&r,&c);
	blot_out(r,c,r,COLS-1);

    /*static SDL_Rect pos = {0, 0, CHAR_WIDTH, CHAR_HEIGHT};
    pos.x = c; pos.y = r; pos.w =  (COLS - c)*CHAR_WIDTH;
    SDL_FillRect(screen, &pos, SDL_MapRGB(screen->format, 255, 255, 0));*/
}

void mvaddstr(r,c,s)
	int r,c;
	char *s;
{
	move(r, c);
	addstr(s);
}

void mvaddch(r,c,chr)
	int r, c;
	byte chr;
{
	move(r, c);
	addch(chr);
}

byte mvinch(r, c)
	int r, c;
{
	move(r, c);
	return(curch()&0xff);
}

/*
 * put the character on the screen and update the
 * character position
 */

int addch(chr)
	byte chr;
{
	int r, c;
    //register int newc, newr;
	byte old_attr;

	old_attr = ch_attr;

	if (at_table == color_attr)
	{
	    /* if it is inside a room */
	    if (ch_attr == 7)
		switch(chr)
		{
		    case DOOR:
		    case VWALL:
		    case HWALL:
		    case ULWALL:
		    case URWALL:
		    case LLWALL:
		    case LRWALL:
		        ch_attr = 6;  /* brown */
		        break;
		    case FLOOR:
		    	ch_attr = 10;  /* light green */
		    	break;
		    case STAIRS:
		        ch_attr = 160; /* black on green*/
		        break;
		    case TRAP:
		        ch_attr = 5;  /* magenta */
		    	break;
		    case GOLD:
		    case PLAYER:
		        ch_attr = 14;  /* yellow */
		        break;
		    case POTION:
		    case SCROLL:
		    case STICK:
		    case ARMOR:
		    case AMULET:
		    case RING:
		    case WEAPON:
		    	ch_attr = 9;
		    	break;
		    case FOOD:
		        ch_attr = 4;
		        break;
		}
		/* if inside a passage or a maze */
	    else if (ch_attr == 112)
		switch(chr)
		{
		    case FOOD:
		        ch_attr = 116;   /* red */
		        break;
		    case GOLD:
		    case PLAYER:
		        ch_attr = 126;  /* yellow on white */
		        break;
		    case POTION:
		    case SCROLL:
		    case STICK:
		    case ARMOR:
		    case AMULET:
		    case RING:
		    case WEAPON:
		    	ch_attr = 113;    /* blue on white */
		    	break;
		}
	    else if (ch_attr == 15 && chr == STAIRS)
	        ch_attr = 160;
	}

	getrc(&r,&c);
	if (chr == '\n') {
		if (r == LINES-1) {
			scroll_up(0, LINES-1, 1);
			move(LINES-1, 0);
		} else
			move(r+1, 0);
			ch_attr = old_attr;
		return c_row;
	}
	putchr(chr);
	move(r,c+1);
	ch_attr = old_attr;
	/*
	 * if you have gone of the screen scroll the whole window
	 */
    return(c_row);
}

void addstr(s)
	char *s;
{
	while(*s)
		addch(*s++);
}

void set_attr(bute)
	int bute;
{
	if (bute < MAXATTR)
		ch_attr = at_table[bute];
		//printf("set_attr(0)--standed()\n");
	else
		ch_attr = bute;

}

void error(mline,msg,a1,a2,a3,a4,a5)
	int mline;
	char *msg;
	int a1,a2,a3,a4,a5;
{
	int row, col;

	getrc(&row,&col);
	move(mline,0);
	clrtoeol();
	printw(msg,a1,a2,a3,a4,a5);
	move(row,col);
}

/*
 * Called when rogue runs to move our cursor to be where DOS thinks
 * the cursor is
 */

void set_cursor()
{
/*
	regs->ax = 15 << 8;
	swint(SW_SCR, regs);
	real_rc(regs->bx >> 8, &c_row, &c_col);
*/
}

/*
 *  winit(win_name):
 *		initialize window -- open disk window
 *						  -- determine type of moniter
 *						  -- determine screen memory location for dma
 */
void winit(void)
{
    //char drive;
    register int i, cnt;
	extern int _dsval;

	/*
	 * Get monitor type
	 */
	regs->ax = 15 << 8;
	swint(SW_SCR, regs);
	old_page_no = regs->bx >> 8;
	scr_type = 1; //regs->ax = 0xff & regs->ax;
	/*
	 * initialization is any good because restarting game
	 * has old values!!!
	 * So reassign defaults
	 */
	LINES   =  25;
	COLS    =  80;
	scr_ds  =  0xB800;
	at_table = monoc_attr;
	memset(video_memory, ' ', sizeof(video_memory));

	switch (scr_type) {
		/*
		 *  It is a TV
		 */
		case 1:
			at_table = color_attr;
		case 0:
			COLS = 40;
			break;

		/*
		 * Its a high resolution monitor
		 */
		case 3:
			at_table = color_attr;
		case 2:
			break;
		case 7:
			scr_ds = 0xB000;
			no_check = TRUE;
			break;
		/*
		 * Just to save text space lets eliminate these
		 *
		case 4:
		case 5:
		case 6:
		    move(24,0);
		    fatal("Program can't be run in graphics mode");
	     */
		default:
			move(24,0);
			fatal("Unknown screen type (%d)",regs->ax);
	}
	/*
	 * Read current cursor position
	 */
	real_rc(old_page_no, &c_row, &c_col);
	if (savewin){//((savewin = sbrk((int*)4096)) == -1) {
		svwin_ds = -1;
		savewin = (char *) _flags;
		if (scr_type == 7)
			fatal(no_mem);
	} else {
		savewin = (char *) (((int) savewin + 0xf) & 0xfff0);
		svwin_ds = (((int) savewin >> 4) & 0xfff) + _dsval;
	}
	for (i = 0, cnt = 0; i < 25; cnt += 2*COLS, i++)
		scr_row[i] = cnt;
	newmem(2);
	switch_page(3);
	if (old_page_no != page_no)
		clear();
	move(c_row, c_col);
	if (isjr())
		no_check = TRUE;
}

void forcebw()
{
	at_table = monoc_attr;
}

/*
 *  wdump(windex)
 *		dump the screen off to disk, the window is save so that
 *		it can be retieved using windex
 */
void wdump()
{
	sav_win();
    dmain(savewin,LINES*COLS,scr_ds,0);
    is_saved = TRUE;
    memmove(saved_screen->pixels, screen->pixels, screen->pitch*screen->h);
}

char *sav_win()
{
	//if (savewin == _flags)
		dmaout(savewin,LINES*COLS,0xb800,8192);
	return(savewin);
}

void res_win()
{
	//if (savewin == _flags)
		dmain(savewin,LINES*COLS,0xb800,8192);
    memmove(screen->pixels, saved_screen->pixels, screen->pitch*screen->h);
}

/*
 *	wrestor(windex):
 *		restor the window saved on disk
 */
void wrestor(void)
{
	dmaout(savewin,LINES*COLS,scr_ds,0);
	res_win();
	is_saved = FALSE;
}

/*
 * wclose()
 *   close the window file
 */
void wclose()
{

	/*
	 * Restor cursor (really you want to restor video state, but be carefull)
 	 */
 	if (scr_type >= 0)
		cursor(TRUE);
	if (page_no != old_page_no)
		switch_page(old_page_no);
}

/*
 *  Some general drawing routines
 */

void box(ul_r, ul_c, lr_r, lr_c)
{
	vbox(dbl_box, ul_r, ul_c, lr_r, lr_c);
}

/*
 *  box:  draw a box using given the
 *        upper left coordinate and the lower right
 */

void vbox(box, ul_r,ul_c,lr_r,lr_c)
	byte box[BX_SIZE];
	int ul_r,ul_c,lr_r,lr_c;
{
	register int i, wason;
	int r,c;

	wason = cursor(FALSE);
	getrc(&r,&c);

	/*
	 * draw horizontal boundry
	 */
	move(ul_r, ul_c+1);
	repchr(box[BX_HT], i = (lr_c - ul_c - 1));
	move(lr_r, ul_c+1);
	repchr(box[BX_HB], i);
	/*
	 * draw vertical boundry
	 */
	for (i=ul_r+1;i<lr_r;i++) {
		mvaddch(i,ul_c,box[BX_VW]);
		mvaddch(i,lr_c,box[BX_VW]);
	}
	/*
	 * draw corners
	 */
	mvaddch(ul_r,ul_c,box[BX_UL]);
	mvaddch(ul_r,lr_c,box[BX_UR]);
	mvaddch(lr_r,ul_c,box[BX_LL]);
	mvaddch(lr_r,lr_c,box[BX_LR]);

	move(r,c);
	cursor(wason);
}

/*
 * center a string according to how many columns there really are
 */
void center(row,string)
	int row;
	char *string;
{
	mvaddstr(row,(COLS-strlen(string))/2,string);
}


/*@
 * Originally had this signature:
 * printw(msg,a1,a2,a3,a4,a5,a6,a7,a8)
 *   char *msg;
 *   int a1, a2, a3, a4, a5, a6, a7, a8;
 *
 * Guess there was no varargs in 1985...
 *
 * Also changed sprintf() to the more secure vsnprintf()
 * No buffer overflows in 85 either?
 *
 * Ieeeee indeed :)
 */
/*
 * printw(Ieeeee)
 */
void
printw(const char *msg, ...)
{
	char pwbuf[132];
	va_list argp;

	va_start(argp, msg);
	vsnprintf(pwbuf, sizeof(pwbuf), msg, argp);
	va_end(argp);
	addstr(pwbuf);
}

void scroll_up(start_row,end_row,nlines)
	int start_row,end_row,nlines;
{
	regs->ax = 0x600 + nlines;
	regs->bx = 0x700;
	regs->cx = start_row << 8;
	regs->dx = (end_row << 8) + COLS - 1;
	swint(SW_SCR,regs);
	move(end_row,c_col);
	SDL_Rect pos = {0, (start_row-nlines)*CHAR_HEIGHT, 0, 0};
	SDL_Rect size = {0, 0, COLS*CHAR_WIDTH, (end_row-start_row+1)*CHAR_HEIGHT};
	SDL_BlitSurface(screen, &size, screen, &pos);
	size.h  = nlines*CHAR_HEIGHT; size.y = end_row*CHAR_HEIGHT;
	//SDL_FillRect(screen, &size, SDL_MapRGB(screen->format, 255, 0, 0));
	SDL_FillRect(screen, &size, SDL_MapRGB(screen->format, 0, 0, 0));
}

void scroll_dn(start_row,end_row,nlines)
	int start_row,end_row,nlines;
{
	regs->ax = 0x700 + nlines;
	regs->bx = 0x700;
	regs->cx = start_row << 8;
	regs->dx = (end_row << 8) + COLS - 1;
	swint(SW_SCR,regs);
	move(start_row,c_col);
}

void scroll()
{
	scroll_up(0,24,1);
}



/*
 * blot_out region
 *    (upper left row, upper left column)
 *	  (lower right row, lower right column)
 */
void blot_out(ul_row,ul_col,lr_row,lr_col)
{
	regs->ax = 0x600;
	regs->bx = 0x700;
	regs->cx = (ul_row<<8) + ul_col;
	regs->dx = (lr_row<<8) + lr_col;
	swint(SW_SCR,regs);
	move(ul_row,ul_col);

    SDL_Rect pos = {0, 0, CHAR_WIDTH, CHAR_HEIGHT};
    pos.x = ul_col*CHAR_WIDTH; pos.y = ul_row*CHAR_HEIGHT;
    pos.w = ((lr_col+1)-ul_col)*CHAR_WIDTH; pos.h = ((lr_row+1)-ul_row)*CHAR_HEIGHT;
    //SDL_FillRect(screen, &pos, SDL_MapRGB(screen->format, 255, 255, 0));
    SDL_FillRect(screen, &pos, SDL_MapRGB(screen->format, 0, 0, 0));
}

void repchr(chr,cnt)
	byte chr; int cnt;
{
	while(cnt-- > 0) {
		putchr(chr);
		c_col++;
	}
}

/*
 * try to fixup screen after we get a control break
 */
void fixup()
{
    blot_out(c_row,c_col,c_row,c_col+1);
}

/*
 * Clear the screen in an interesting fashion
 */

void implode()
{
	int j, delay, r, c, cinc = COLS/10/2, er, ec;

	er = (COLS == 80 ? LINES-3 : LINES-4);
	/*
	 * If the curtain is down, just clear the memory
	 */
	if (scr_ds == svwin_ds) {
		wsetmem(savewin, (er + 1) * COLS, 0x0720);
		return;
	}
	delay = scr_type == 7 ? 500 : 10;
	for (r = 0,c = 0,ec = COLS-1; r < 10; r++,c += cinc,er--,ec -= cinc) {
		vbox(sng_box, r, c, er, ec);
		for (j = delay; j--; )
			;
		for (j = r+1; j <= er-1; j++) {
			move(j, c+1); repchr(' ', cinc-1);
			move(j, ec-cinc+1); repchr(' ', cinc-1);
		}
		vbox(spc_box, r, c, er, ec);
	}
}

/*
 * drop_curtain:
 *	Close a door on the screen and redirect output to the temporary buffer
 */
static int old_ds;
void drop_curtain()
{
	register int r, j, delay;

	if (svwin_ds == -1)
		return;
	old_ds = scr_ds;
	dmain(savewin, LINES * COLS, scr_ds, 0);
	cursor(FALSE);
	delay = (scr_type == 7 ? 3000 : 2000);
	green();
	//vbox(sng_box, 0, 0, LINES-1, COLS-1);
	vbox(spc_box, 0, 0, LINES-1, COLS-1);
	yellow();
	for (r = 1; r < LINES-1; r++) {
		move(r, 1);
		//repchr(0xb1, COLS-2);  SDL_Flip(screen); SDL_Delay(50);
		repchr(0, COLS-2);
		for (j = delay; j--; )
			;
	}
	scr_ds = svwin_ds;
	move(0,0);
	standend();
}

void raise_curtain()
{
	register int i, j, o, delay;

	if (svwin_ds == -1)
		return;
	scr_ds = old_ds;
	delay = (scr_type == 7 ? 3000 : 2000);
	for (i = 0, o = (LINES-1)*COLS*2; i < LINES; i++, o -= COLS*2) {
		dmaout(savewin + o, COLS, scr_ds, o);
		for (j = delay; j--; )
			;
	}
}

void switch_page(pn)
{
	register int pgsize;

	if (scr_type == 7) {
		page_no = 0;
		return;
	}
	if (COLS == 40)
		pgsize = 2048;
	else
		pgsize = 4096;
	regs->ax = 0x0500 | pn;
	swint(SW_SCR, regs);
	scr_ds = 0xb800 + ((pgsize * pn) >> 4);
	page_no = pn;
}

byte get_mode(void)
{
	struct sw_regs regs;

	regs.ax = 0xF00;
	swint(SW_SCR,&regs);
	return 0xff & regs.ax;
}

byte video_mode(type)
{
	struct sw_regs regs;

	regs.ax = type;
	swint(SW_SCR,&regs);
	return regs.ax;
}




/*@
 * Fills a buffer with "extended" chars (char + attribute)
 *
 * This is the n-bytes version of setmem(). There is no POSIX or ncurses
 * direct replacement other than a loop writing multiple bytes at a time.
 *
 * For DOS compatibility, chtype (and by proxy wsetmem()) is currently set to
 * operate on 16-bit words. But chtype size in <curses.h> may be set up to
 * a whooping 64-byte unsigned long, so make *really* sure buffer size and
 * count argument are consistent with chtype size!
 *
 * This function could be generic enough to be in mach_dep.c, but it was only
 * by curses.c to write a character + attribute to window buffer.
 *
 * Originally in dos.asm
 */
void
wsetmem(buffer, count, attrchar)
	void *buffer;
	int count;
	chtype attrchar;  // enforced to prevent misuse
{
	while (count--)
		((chtype *)buffer)[count] = (chtype)attrchar;
}



/*@
 * Move the cursor to the given row and column
 *
 * Originally in zoom.asm
 *
 * As per original code, also updates C global variables c_col_ and c_row,
 * used as "cache" by curses. See getrc(). Actual cursor movement is only
 * performed if iscuron is set. See cursor().
 *
 * Used BIOS INT 10h/AH=02h to set the cursor on C variable page_no.
 * The BIOS call is now performed via swint()
 *
 * INT 10h/AH=02h - Set Cursor Position
 * BH = page number
 * DH = row
 * DL = col
 */
void move(row, col)
	int row;
	int col;
{
	c_row = row;
	c_col = col;

	if (iscuron)
	{
		regs->ax = HIGH(2);
		regs->bx = HIGH(page_no);
		regs->dx = HILO(row, col);
		swint(SW_SCR, regs);
	}
}

int swint(intno, rp)
	int intno;
	struct sw_regs *rp;
{
	//@ DS register value. Originally an extern set by begin.asm, now a dummy
	int _dsval = 0x00;

	rp->ds = rp->es = _dsval;
	sysint(intno, rp, rp);
	return rp->ax;
}


/*@
 * sysint() - System Interrupt Call
 * This was available as a C library function in old DOS compilers
 * Created here as a stub: output general registers are zeroed,
 * index and segment register values are copied from input.
 * Return FLAGS register, or rather a dummy with reasonable values
 */
int sysint(intno, inregs, outregs)
	int intno;

	struct sw_regs *inregs, *outregs;
{

	outregs->ax = 0;
	outregs->bx = 0;
	outregs->cx = 0;
	outregs->dx = 0;
	outregs->si = inregs->si;
	outregs->di = inregs->di;
	outregs->ds = inregs->ds;
	outregs->es = inregs->es;

	// reserved flags and IF set, all others unset
	return 0xF22A;
}

/*@
 * Return character (without any attributes) at current cursor position
 *
 * Wrapper to inch() that strips attributes
 *
 * Asm function returned character with attributes, but all callers stripped
 * out attributes via 0xFF-anding, considering only the character. With inch()
 * the proper stripping would require exporting A_CHARTEXT macro, and perhaps
 * chtype, to the public API, so to simplify both usage and implementation
 * stripping is now performed here, and it returns a character of type byte,
 * the type consistently used by Rogue to indicate a CP850 character.
 *
 * Originally in zoom.asm by the name curch()
 *
 * By my understanding, asm function works very similar to putchr():
 * If iscuron, invokes BIOS INT 10h/AH=02h to set cursor position from cache
 * and 10h/AH=08h to read character, else wait retrace (unless no_check) and
 * read directly from Video Memory. The set cursor BIOS call seems quite
 * redundant, as virtually all calls to this, from both the inch() macro and
 * the mvinch() wrapper, are preceded by a move().
 *
 * This function replicates this behavior, except the redundant move.
 *
 * BIOS INT 10h/AH=08h - Read character and attribute at cursor position
 * BH = page number
 * Return:
 * AH = attribute
 * AL = character
 */
byte curch(void)
{
	chtype chrattr = 0;

	if (iscuron)
	{
		regs->ax = HIGH(8);
		regs->bx = HIGH(page_no);
		chrattr = swint(SW_SCR, regs);
	}
	else
	{
		if (!no_check){;}
		dmain(&chrattr, 1, scr_ds, scr_row[c_row] + 2 * c_col);
	}
	//return (byte)LOW(chrattr);
	return video_memory[c_row][c_col];
}

/*@
 * Put the given character on the screen
 *
 * Character is put at current (c_row, c_col) cursor position, and set with
 * current ch_attr attributes.
 *
 * Works as a stripped-down <curses.h> addch(), or as an improved <stdio.h>
 * putchar(): it uses attributes but always operate on current ch_attr instead
 * of extracting attributes from ch, and put at cursor position but does not
 * update its location, nor has any special CR/LF/scroll up handling for '\n'.
 *
 * A curses replacement could be:
 *    delch();
 *    insch(ch);
 *
 * Originally in zoom.asm
 *
 * By my understanding, asm function works as follows: if cursor is on (via
 * iscuron C var), it invokes BIOS INT 10h/AH=09h to put char with attributes.
 * If not, it waits for video retrace (unless no_check C var was TRUE) and
 * then write directly in Video Memory, using C vars scr_row and scr_ds to
 * calculate position address.
 *
 * This function replicates this behavior using dmaout() and swint().
 *
 * BIOS INT 10h/AH=09h - Write character with attribute at cursor position
 * AL = character
 * BH = page number
 * BL = character attribute
 * CX = number of times to write character
 *
 */
void
putchr(byte ch)
{
	/*if (iscuron)
	{
		// Use BIOS call
		regs->ax = HILO(9, ch);
		regs->bx = HILO(page_no, ch_attr);
		regs->cx = 1;
		swint(SW_SCR, regs);
	}
	else
	{
		if (!no_check){;}  // "wait" for video retrace*/
		/*
		 * Write to video memory
		 * Each char uses 2 bytes in video memory, hence doubling c_col.
		 * scr_row[] array takes that into account, so we can use c_row
		 * directly. See winit().
		 */
		/*dmaout((void*)HILO(ch_attr, ch), 1,
			scr_ds, scr_row[c_row] + 2 * c_col);
	}*/
    //if(isascii(ch))
    {
        //printf("ch=%c c_row=%d c_col=%d ch_attr=%d \n", ch, c_row, c_col, ch_attr);
        SDL_Rect pos = {c_col*CHAR_WIDTH, c_row*CHAR_HEIGHT, CHAR_WIDTH, CHAR_HEIGHT};
        /*pos.x = c_col*CHAR_WIDTH; pos.y = c_row*CHAR_HEIGHT;
        SDL_FillRect(screen, &pos, SDL_MapRGB(screen->format, 0, 0, 0));
        print_char(screen, c_col*CHAR_WIDTH, c_row*CHAR_HEIGHT, ch);*/
        print_rogue_char(screen, pos.x, pos.y, ch);
        video_memory[c_row][c_col] = ch;
        //SDL_UpdateRect(screen, pos.x, pos.y, pos.w, pos.h);
    }
}

/*@
 * Return TRUE if the system is identified as an IBM PCJr ("PC Junior")
 *
 * Moved from mach_dep.c, only used for setting no_check in winit().
 *
 * 0xF000:0xFFFE 1  IBM computer-type code; see also BIOS INT 15h/C0h
 *  0xFF = Original PC
 *  0xFE = XT or Portable PC
 *  0xFD = PCjr
 *  0xFC = AT (or XT model 286) (or PS/2 Model 50/60)
 *  0xFB = XT with 640K motherboard
 *  0xFA = PS/2 Model 30
 *  0xF9 = Convertible PC
 *  0xF8 = PS/2 Model 80
 */
#define PC  0xff
#define XT  0xfe
#define JR  0xfd
#define AT  0xfc
bool
isjr()
{
	static int machine = 0;

	if (machine == 0) {
		dmain(&machine,1,0xf000,0xfffe);
		machine &= 0xff;
	}
	return machine == JR;
}

/*@
 * Write data to memory starting at segment:offset address
 *
 * Length of data is measured in words (16-bit), the size of int in DOS
 *
 * Dummy no-op, see pokeb().
 *
 * Originally in dos.asm.
 *
 * While original is technically "direct memory access", the name is misleading
 * as it has nothing to do with DMA channels. And despite documentation on
 * dos.asm, it has no particular ties to video: it is a general use memory
 * writer that happens to be most often used to write to video memory address.
 * Asm works by setting the arguments and calling REP MOVSW
 */
void
dmaout(data, wordlength, segment, offset)
	void* data;
	unsigned int wordlength;
	unsigned int segment;
	unsigned int offset;
{
	//printf("dmaout(%p, %d, %04x:%04x)\n",
			//data, wordlength, segment, offset);
}


/*@
 * Read memory starting at segment:offset address and store contents in buffer
 *
 * Length of buffer is measured in words (16-bit), the size of int in DOS
 *
 * Dummy no-op, leave buffer unchanged.
 *
 * Originally in dos.asm. See notes on dmaout()
 */
void
dmain(buffer, wordlength, segment, offset)
	void UNUSED(*buffer);
	unsigned int UNUSED(wordlength);
	unsigned int UNUSED(segment);
	unsigned int UNUSED(offset);
{
	;
}


/*@
 * Beep a an audible beep, if possible
 *
 * Originally in dos.asm
 *
 * Used hardware port 0x61 (Keyboard Controller) for direct PC Speaker access.
 * This behavior is reproduced here, to the best of my knowledge. Comments
 * were copied from original.
 *
 * The debug code prints a BEL (0x07) character in standard output, which is
 * supposed to make terminals play a beep.
 */
void
beep(void)
{
	byte speaker = 0x61;       //@ speaker port
	byte saved = in(speaker);  // input control info from keyboard/speaker port
	byte cmd = saved;
	int cycles = 300;          // count of speaker cycles
	int c;

	while (--cycles)
	{
		cmd &= 0x0fc;          // speaker off pulse (mask out bit 0 and 1)
		out(speaker, cmd);     // send command to speaker port
		for(c=50; c; c--) {;}  // kill time for tone half-cycle
		cmd |= 0x10;           // speaker on pulse (bit 1 on)
		out(speaker, cmd);     // send command to speaker port
		for(c=50; c; c--) {;}  // kill time for tone half-cycle
	}
	out(speaker, saved);       // restore speaker/keyboard port value
}

/*@
 * Read a character from user input. getch() with non-blocking capability
 *
 * msdelay has same meaning as delay in timeout(): If no key was pressed after
 * msdelay milliseconds, return ERR. Negative values will block until a key
 * is pressed. Both blocking and timeout mode require a properly initialized
 * curses with initscr() (in winit()), otherwise it will be non-blocking.
 * This requirement did not exist in original
 *
 * After the wgetch() call, input will always restore to blocking mode using
 * nodelay(FALSE);
 *
 * Return ERR on non-ASCII chars and on window resize.
 */
int handle_keyboard_inputs(void);
int
getch()
{
	/*@
	 * getchar() is not a true replacement, as asm version has no echo and no
	 * buffering. Asm was also non-blocking, but only used after no_char(),
	 * a combination that effectively blocks until input.
	 */
	//char c = handle_keyboard_inputs();
	//SDL_Flip(screen);
	return handle_keyboard_inputs();//c;//getchar();
}

/*
 * Table for IBM extended key translation
 * moved from march_dep.c
 */
/*static struct xlate {
	int keycode;
	byte keyis;
} xtab[] = {
	{C_HOME,	'y'},
	{C_UP,		'k'},
	{C_PGUP,	'u'},
	{C_LEFT,	'h'},
	{C_RIGHT,	'l'},
	{C_END,		'b'},
	{C_DOWN,	'j'},
	{C_PGDN,	'n'},
	{C_INS,		'>'},
	{C_DEL,		's'},
	{C_F1,		'?'},
	{C_F2,		'/'},
	{C_F3,		'a'},
	{C_F4,		CTRL('R')},
	{C_F5,		'c'},
	{C_F6,		'D'},
	{C_F7,		'i'},
	{C_F8,		'^'},
	{C_F9,		CTRL('F')},
	{C_F10,		'!'},
	{ALT_F9,	'F'}
};*/

static struct xlate {
	int keycode;
	byte keyis;
} xtab[] = {
    {SDLK_RETURN,   '\n'},
	{SDLK_HOME,	     'y'},
	{SDLK_UP,	     'k'},
	{SDLK_PAGEUP,    'u'},
	{SDLK_LEFT,	     'h'},
	{SDLK_RIGHT,     'l'},
	{SDLK_END,	     'b'},
	{SDLK_DOWN,	     'j'},
	{SDLK_PAGEDOWN,	 'n'},
	{SDLK_INSERT,	 '>'},
	{SDLK_DELETE,    's'},
	{SDLK_F1,		 '?'},
	{SDLK_F2,		 '/'},
	{SDLK_F3,		 'a'},
	{SDLK_F4,  CTRL('R')},
	{SDLK_F5,		 'c'},
	{SDLK_F6,		 'D'},
	{SDLK_F7,		 'i'},
	{SDLK_F8,		 '^'},
	{SDLK_F9,  CTRL('F')},
	{SDLK_F10,		 '!'},
	{ALT_F9,	     'F'}
};


/*
 * This routine reads information from the keyboard
 * It should do all the strange processing that is
 * needed to retrieve sensible data from the user
 */
int getinfo(str,size)
    char *str;
    int size;
{
    register char *retstr;
    int ch;
    int readcnt = 0;
    int wason, ret = 1;
    char buf[160];

	dmain(buf, 80, scr_ds, 0);
	retstr = str;
	*str = 0;
	wason = cursor(TRUE);
	while(ret == 1 && playing)
    {
        switch(ch = getch()) {
            case NOCHAR: break;
        	case ESCAPE:
        		while(str != retstr) {
        		    backspace();
                	readcnt--;
                	str--;
                }
                ret = *str = ESCAPE;
                cursor(wason);
                break;
            case '\b':
                if (str != retstr) {
                    putchr(' ');
                    backspace();
                	readcnt--;
                	str--;
                }
                break;
            default:
                if((ch < 32) || (ch > 127)) break;
                if ( readcnt >= size) {
                   beep();
                   break;
                }
                readcnt++;
            	addch(ch);
                *str++ = ch;
               	if ((ch & 0x80) == 0)
					break;
            case '\n':
            case '\r':
                *str = 0;
                cursor(wason);
                ret = ch;
                break;
        }
        if(readcnt < size)
            cursor_flash();
        if(ch == '\r')
            putchr(' ');
        refresh();
    }
	dmaout(buf, 80, scr_ds, 0);
    return ret;
}

/*@
 * No need of #ifdef ROGUE_DOS_CURSES here as the non-curses input of getch()
 * used by getinfo() is performed by <stdio.h> getchar(), which is line
 * buffered anyway, so a backspace char will never be seen and this will never
 * be called.
 */
void
backspace()
{
	int x, y;
	getxy(&x,&y);
	if (--y<0)
		y = 0;
	move(x,y);
    putchr(' ');
    SDL_Rect pos = {c_col*CHAR_WIDTH, c_row*CHAR_HEIGHT, CHAR_WIDTH, CHAR_HEIGHT};
    SDL_UpdateRect(screen, pos.x, pos.y, pos.w, pos.h);
}



/*@
 * Map a (possibly multi-byte or control) character to an 8-bit character using
 * the game translation table.
 *
 * Moved from mach_dep.c as part of readchar()
 */
byte
xlate_ch(int ch)
{
	struct xlate *x;
	bool match = FALSE;
	/*
	 * Now read a character and translate it if it appears in the
	 * translation table
	 */
	for (x = xtab; x < xtab + (sizeof xtab) / sizeof *xtab; x++)
	{
		if (ch == x->keycode) {
			ch = x->keyis;
			match = TRUE;
			break;
		}
	}
	if (!match && !isascii(ch)) ch = 1;
	return (byte)ch;
}

void cursor_flash(void)
{
     static int flash_time = 0;
     static bool flash_it = FALSE;
     SDL_Rect pos = {c_col*CHAR_WIDTH, c_row*CHAR_HEIGHT+12, CHAR_WIDTH, 4};
     flash_time++;
     if(flash_time >= 30)
     {
        flash_it = !flash_it;
        flash_time = 0;
     }
    if(flash_it)
        SDL_FillRect(screen, &pos, 0xc0c0c0);
    else
        SDL_FillRect(screen, &pos, 0x000000);
}

void refresh(void){SDL_Flip(screen); sleep();}


