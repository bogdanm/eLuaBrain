// Online help reader

#ifndef __HELP_H__
#define __HELP_H__

#define HELP_COLOR_SUPPORT
#define HELP_FILE_NAME        "/rom/eluadoc.bin"

#include "type.h"

// This defines a module
typedef struct
{
  const char *name;
  const char *desc;
  u16 nfuncs;
  const char **pfuncs;
} HELP_MOD_DATA;

int help_init( const char *fname );
void help_close();
int help_get_num_modules();
const char* help_get_module_name_at( int index );
const char* help_get_module_overview_at( int index );
HELP_MOD_DATA* help_load_module_idx( int index );
HELP_MOD_DATA* help_load_module_name( const char *name );
void help_unload_module();
void help_help( const char *topic );

#ifdef HELP_COLOR_SUPPORT
#define HRESET "\xFF"
#define HBLACK "\x80"
#define HDBLUE "\x81"
#define HDGREEN "\x82"
#define HDCYAN "\x83"
#define HDRED "\x84"
#define HDMAGENTA "\x85"
#define HDBROWN "\x86"
#define HLGRAY "\x87"
#define HDGRAY "\x88"
#define HLBLUE "\x89"
#define HLGREEN "\x8A"
#define HLCYAN "\x8B"
#define HLRED "\x8C"
#define HLMAGENTA "\x8D"
#define HLYELLOW "\x8E"
#define HWHITE "\x8F"
#else // #ifdef HELP_COLOR_SUPPORT
#define HRESET ""
#define HBLACK ""
#define HDBLUE ""
#define HDGREEN ""
#define HDCYAN ""
#define HDRED ""
#define HDMAGENTA ""
#define HDBROWN ""
#define HLGRAY ""
#define HDGRAY ""
#define HLBLUE ""
#define HLGREEN ""
#define HLCYAN ""
#define HLRED ""
#define HLMAGENTA ""
#define HLYELLOW ""
#define HWHITE ""
#endif // #ifdef HELP_COLOR_SUPPORT

#endif // #ifndef __HELP_H__

