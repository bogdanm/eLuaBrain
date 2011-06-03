// Editor hardware related functions

#ifndef __EDHW_H__
#define __EDHW_H__

#include "type.h"
#include "editor.h"

// Editor HW interface 
int edhw_init();
int edhw_getkey();
void edhw_clrscr();
void edhw_writechar( char c );
void edhw_writetext( const char* text );
//void edhw_setcolors( int fgcol, int bgcol );
void edhw_invertcols( int flag );
void edhw_gotoxy( int x, int y );
void edhw_setcursor( int type );
void edhw_longline( int enable );

#endif

