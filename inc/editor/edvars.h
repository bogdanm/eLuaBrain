// Editor variables

#ifndef __EDVARS_H__
#define __EDVARS_H__

#ifndef EDITOR_MAIN_FILE
#define EDSPEC
#else
#define EDSPEC                          extern
#endif

EDSPEC EDITOR_BUFFER *ed_crt_buffer;     // current editor file data
EDSPEC s16 ed_startx, ed_startline;      // start column and line
EDSPEC s16 ed_cursorx, ed_cursory;       // cursor position
EDSPEC s16 ed_nlines;                    // number of lines on screen
EDSPEC s16 ed_userstartx, ed_userx;     // cursor column as requested by user

#endif

