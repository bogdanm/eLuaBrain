// Common platform functions

#ifndef __COMMON_H__
#define __COMMON_H__

#include "elua_int.h"

// Virtual timers data
#define VTMR_FIRST_ID           ( 32 )
#define VTMR_GET_ID( x )        ( ( x ) - VTMR_FIRST_ID )
#define TIMER_IS_VIRTUAL( x )   ( ( VTMR_NUM_TIMERS > 0 ) && ( ( x ) >= VTMR_FIRST_ID ) && ( ( x ) < VTMR_NUM_TIMERS + VTMR_FIRST_ID ) )

// Path types
#define PATH_TYPE_INVALID       0
#define PATH_TYPE_FS            1
#define PATH_TYPE_FILE          2
#define PATH_TYPE_WC            3 // wildcards

// Functions exported by the common platform layer
void cmn_platform_init();
void cmn_virtual_timer_cb();
void cmn_int_handler( elua_int_id id, elua_int_resnum resnum );
// Timer-specific functions
int cmn_tmr_int_set_status( elua_int_resnum resnum, int status );
int cmn_tmr_int_get_status( elua_int_resnum resnum );
int cmn_tmr_int_get_flag( elua_int_resnum resnum, int clear );
void cmn_uart_setup_sermux();

unsigned int intlog2( unsigned int v );
int cmn_match_fname( const char *fname, const char *pattern );
int cmn_get_path_type( const char *path, const char **pname );

#endif // #ifndef __COMMON_H__

