// Editor movement functions

#ifndef __EDMOVE_H__
#define __EDMOVE_H__

#include "type.h"
#include "editor.h"

int edmove_handle_key( int c );
void edmove_key_end();
void edmove_set_cursorx( int x );
void edmove_set_cursory( int y );
void edmove_goto_line( int y );
void edmove_save_cursorx();
void edmove_restore_cursor();

#endif

