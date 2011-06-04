// Editor hardware related functions

#ifndef __EDHW_H__
#define __EDHW_H__

#include "type.h"
#include "editor.h"

// Message types
#define EDHW_MSG_ERROR        0
#define EDHW_MSG_INFO         1
#define EDHW_MSG_WARNING      2

// Input validator function
typedef int ( *p_ed_validate )( const char *crt, int c );

// Editor HW interface 
int edhw_init();
int edhw_getkey( void );
void edhw_clrscr();
void edhw_writechar( char c );
void edhw_writetext( const char* text );
//void edhw_setcolors( int fgcol, int bgcol );
void edhw_invertcols( int flag );
void edhw_gotoxy( int x, int y );
void edhw_setcursor( int type );
void edhw_longline( int enable );
void edhw_msg( const char *text, int type );
char* edhw_read( const char *title, const char *text, unsigned maxlen, p_ed_validate validator );

#endif

