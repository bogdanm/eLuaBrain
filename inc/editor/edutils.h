// General utilities

#ifndef __EDUTILS_H__
#define __EDUTILS_H__

#include "type.h"
#include "editor.h"

char* edutils_line_get( int id );
void edutils_line_set( int id, char* pline );
int edutils_get_actsize( const char* pdata );
int edutils_is_flag_set( EDITOR_BUFFER* b, int flag );
void edutils_set_flag( EDITOR_BUFFER* b, int flag, int value );
void edutils_display_status();
void edutils_line_display( int scrline, int id );
void edutils_show_screen();

#endif

