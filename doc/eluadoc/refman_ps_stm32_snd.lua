-- eLua platform interface - stm32.snd module

data_en = 
{

  -- Title
  title = "eLua reference manual - STM32 snd module",

  -- Menu name
  menu_name = "snd",
  mod_name = "stm32.snd",
  desc = "STM32 PWM based sound generator",

  -- Overview
  overview = "This module implements PWM based sound generation with a simple syntax.",

  -- Functions
  funcs = 
  {
    { sig = "#stm32.snd.play#( song )",
      desc = [[Plays the specified song. The song is described with a simple syntax detailed below. This function is blocking (it doesn't return until the song finished playing).]],
      args = [[$song$ - the song to be played. The song is given in a string which contains notes and indications about their duration. Notes are specified like below:
<ul>
    <li>$A$, $B$ ... $G$ - the given note.</li>
    <li>$a$, $c$, $d$, $f$ and $g$ - sharp notes.</li>
    <li>$p$ - a pause (no sound).</li>
    <li>$1$, $2$ ... $9$ - octave number (default is $3$).</li>
    <li>$+$ - octave up.</li>
    <li>$-$ - octave down.</li></ul>
  <p>Notes durations and tempo are specified like below:</p>
<ul>
    <li>$%%$ - play a full note (2 seconds - default).</li>
    <li>$*$ - 1/2 of a note.</li>
    <li>$;$ - 1/4 of a note.</li>
    <li>$,$ - 1/6 of a note.</li>
    <li>$:$ - 1/8 of a note.</li>
    <li>$.$ - 1/32 of a note.</li>
    <li>$>>$ - tempo up (1/4 sec).</li>
    <li>$<<$ - tempo down (1/4 sec).</li></ul>]]      
    },

  },

}

data_pt = data_en

