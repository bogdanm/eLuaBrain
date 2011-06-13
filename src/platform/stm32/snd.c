// Platform specific sound module for Moonlight

#include "type.h"
#include "platform.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "modcommon.h"
#include "lrotable.h"
#include <math.h>
#include <ctype.h>
#include "common.h"
#include "platform_conf.h"

// Duration of a full note in ms
#define SND_FULL_NOTE_DURATION_US     2000000
#define SND_PAUSE_INDEX               100
#define SND_BASE_FREQUENCY            1000000

// Plays a "string" that contains the song, as follows:
// Sounds:
//   A - G : a "natural" note
//   a, c, d, f, g: accidents
//   p - a pause (no sound)
//   1, 2, 3, 4,... - change octave (a single digit) (3 by default)
//   + - octave up
//   - - octave down
// Durations:
//   % - full note (default)
//   * - 1/2
//   ; - 1/4
//   : - 1/8
//   , - 1/6
//   . - 1/32
//   > - tempo up (1/4sec)
//   < - tempo down (1/4sec)

// Lua: play( "song" )
static int snd_play( lua_State *L )
{
  const char *psong = luaL_checkstring( L, 1 );
  unsigned freq, oct = 3;
  unsigned tempo_correction = 0, tempo_divider = 1;
  char c;
  int note = -1;
  const int natural_note_to_index[] = { 0, 2, 3, 5, 7, 8, 10 };
  const int accident_to_index[] = { 1, -1, 4, 6, -1, 9, 11 };
  
  platform_pwm_op( SND_PWM_ID, PLATFORM_PWM_OP_SET_CLOCK, SND_BASE_FREQUENCY );
  while( ( c = *psong ++ ) )
  {
    note = -1;
    if( isdigit( c ) )
      oct = c - '0';
    else if( 'A' <= c && c <= 'G' )
      note = natural_note_to_index[ c - 'A' ];
    else if( 'a' <= c && c <= 'g' )
    {
      note = accident_to_index[ c - 'a' ];
      if( note == -1 )
        return luaL_error( L, "invalid note '%c'", c );
    }
    else if( c == 'p' )
      note = SND_PAUSE_INDEX;
    else 
    {
      switch( c )
      {
        case '>':
          tempo_correction --;
          break;
          
        case '<':
          tempo_correction ++;
          break;
          
        case '%':
          tempo_divider = 1;
          break;
          
        case '*':
          tempo_divider = 2;
          break;
          
        case ';':
          tempo_divider = 4;
          break;
          
        case ':':
          tempo_divider = 8;
          break;          
          
        case ',':
          tempo_divider = 16;
          break;
          
        case '.':
          tempo_divider = 32;
          break;    
          
        case '+':
          oct ++;
          break;
          
        case '-':
          oct --;
          break;   
          
        default:
          return luaL_error( L, "invalid char '%c'", c ); 
      }
    }
    if( note != -1 )
    {
      if( note != SND_PAUSE_INDEX )
      {
        freq = ( unsigned )( 55.0 * pow( 2, ( double )( note + ( oct - 1 ) * 12 ) / 12.0 ) );
        platform_pwm_setup( SND_PWM_ID, freq, 50 );
        platform_pwm_op( SND_PWM_ID, PLATFORM_PWM_OP_START, 0 );
      } 
      platform_timer_delay( SND_TIMER_ID, ( SND_FULL_NOTE_DURATION_US + 250 * tempo_correction ) / tempo_divider ); 
      platform_pwm_op( SND_PWM_ID, PLATFORM_PWM_OP_STOP, 0 );
    }
  }
  return 0;  
}

#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
const LUA_REG_TYPE snd_map[] =
{
  { LSTRKEY( "play" ), LFUNCVAL( snd_play ) },
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_snd( lua_State *L )
{
#if LUA_OPTIMIZE_MEMORY > 0
  return 0;
#else // #if LUA_OPTIMIZE_MEMORY > 0
  #error "snd module doesn't run in LUA_OPTIMIZE_MEMORY != 0 "
#endif // #if LUA_OPTIMIZE_MEMORY > 0  
}

