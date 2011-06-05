// Editor variables

#ifndef __EDVARS_H__
#define __EDVARS_H__

#ifndef EDITOR_MAIN_FILE
#define EDSPEC
#else
#define EDSPEC                          extern
#endif

EDSPEC EDITOR_BUFFER *ed_crt_buffer;     // current editor file data

#define ed_startx             ed_crt_buffer->startx
#define ed_startline          ed_crt_buffer->startline
#define ed_cursorx            ed_crt_buffer->cursorx
#define ed_cursory            ed_crt_buffer->cursory
#define ed_nlines             ed_crt_buffer->nlines
#define ed_userstartx         ed_crt_buffer->userstartx
#define ed_userx              ed_crt_buffer->userx
#define ed_firstsel           ed_crt_buffer->firstsel
#define ed_lastsel            ed_crt_buffer->lastsel
#define ed_sellines           ed_crt_buffer->sellines

#endif

