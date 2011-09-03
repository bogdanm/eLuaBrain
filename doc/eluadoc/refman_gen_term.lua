-- eLua reference manual - term module

data_en = 
{

  -- Title
  title = "eLua reference manual - term module",

  -- Menu name
  menu_name = "term",

  desc = "Screen and keyboard (terminal) operations",

  -- Overview
  overview = [[This module contains functions for accessing ANSI-compatible terminals (and terminal emulators) from Lua. For $STMBrain$ this module is an interface to both the PS/2 keyboard and the screen (VGA interface).]],

   -- Data structures, constants and types
  structures = 
  {
    { text = [[term.COL_BLACK
term.COL_DARK_BLUE
term.COL_DARK_GREEN
term.COL_DARK_CYAN
term.COL_DARK_RED
term.COL_DARK_MAGENTA
term.COL_BROWN
term.COL_LIGHT_GRAY
term.COL_DARK_GRAY
term.COL_LIGHT_BLUE
term.COL_LIGHT_GREEN
term.COL_LIGHT_CYAN
term.COL_LIGHT_RED
term.COL_LIGHT_MAGENTA
term.COL_LIGHT_YELLOW
term.COL_WHITE
term.COL_DEFAULT
term.COL_DONT_CHANGE]],
      name = "Colors",
      desc = [[These are the color codes that can be used with $term$ functions that accept colors. $term.COL_DEFAULT$ specifies a default color (currently light gray for foreground and black for background) and $term.COL_DONT_CHANGE$ means "don't change color". This last constant can be used when calling a function that needs both foreground and background colors, but only one of them needs to be actually changed.]]
    },
  },
 

  -- Functions
  funcs = 
  {
    { sig = "#term.clrscr#()",
      desc = "Clear the screen",
    },

    { sig = "#term.clreol#()",
      desc = "Clear from the current cursor position to the end of the line",
    },

    { sig = "#term.moveto#( x, y )",
      desc = "Move the cursor to the specified coordinates",
      args=
      {
        "$x$ - the column (starting with 0)",
        "$y$ - the line (starting with 0)"
      }
    },

    { sig = "#term.moveup#( delta )",
      desc = "Move the cursor up",
      args = "$delta$ - number of lines to move the cursor up"
    },

    { sig = "#term.movedown#( delta )",
      desc = "Move the cursor down",
      args = "$delta$ - number of lines to move the cursor down",
    },

    { sig = "#term.moveleft#( delta )",
      desc = "Move the cursor left",
      args = "$delta$ - number of columns to move the cursor left",
    },
 
    { sig = "#term.moveright#( delta )",
      desc = "Move the cursor right",
      args = "$delta$ - number of columns to move the cursor right",
    },

    { sig = "numlines = #term.getlines#()",
      desc = "Get the number of lines in the terminal",
      ret = "The number of lines in the terminal",
    },

    { sig = "numcols = #term.getcols#()",
      desc = "Get the number of columns in the terminal",
      ret = "The number of columns in the terminal",
    },

    { sig = "#term.print#( [ x, y ], str1, [ str2, ..., strn ] )",
      desc = "Write one or more strings in the terminal",
      args = 
      {
        "$x (optional)$ - write the string at this column. If $x$ is specified, $y$ must also be specified.",
        "$y (optional)$ - write the string at this line. If $y$ is specified, $x$ must also be specified.",
        "$str1$ - the first string to write.",
        "$str2 (optional)$ - the second string to write.",
        "$strn (optional)$ - the nth string to write."
      }
    },
    
    { sig = "#term.cprint#( [ x, y ], str, [fgcol], [bgcol] )",
      desc = "Writes a string in the terminal with the given colors",
      args = 
      {
        "$x (optional)$ - write the string at this column. If $x$ is specified, $y$ must also be specified.",
        "$y (optional)$ - write the string at this line. If $y$ is specified, $x$ must also be specified.",
        "$str$ - the string to write.",
        "$fgcol (optional)$ - foreground color (default is $term.COL_DONT_CHANGE$).",
        "$bgcol (optional)$ - background color (default is $term.COL_DONT_CHANGE$)."
      }
    },


    { sig = "cx = #term.getcx#()",
      desc = "Get the current column of the cursor",
      ret = "The column of the cursor"
    },

    { sig = "cy = #term.getcy#()",
      desc = "Get the current line of the cursor",
      ret = "The line of the cursor" 
    },

    { sig = "ch = #term.getchar#( [ mode ] )",
      desc = "Read a char (a key press) from the terminal",
      args = [[$mode (optional)$ - terminal input mode. It can be either:</p>
  <ul>
    <li>$term.WAIT$ - wait for a key to be pressed, then return it. This is the default behaviour if $mode$ is not specified. </li>
    <li>$term.NOWAIT$ - if a key was pressed on the terminal return it, otherwise return -1.</li></ul><p>]],
      ret = [[The char read from a terminal or -1 if no char is available. The 'char' can be an actual ASCII char, or a 'pseudo-char' which encodes special keys on the keyboard. The list of the special chars` and their meaning ` `is given` in the table ` `below:`</p>
<table style="text-align: left; margin-left: 2em;">
<tbody>
<tr>
  <th style="text-align: left;">Key code</th>
  <th style="text-align: left;">Meaning</th>
</tr>
<tr>
  <td>$term.KC_UP$</td>
  <td>the UP key on the terminal</td>
</tr>
<tr>
  <td>$term.KC_DOWN$</td>
  <td>the DOWN key on the terminal</td>
</tr>
<tr>
  <td>$term.KC_LEFT$</td>
  <td>the LEFT key on the terminal</td>
</tr>
<tr>
  <td>$term.KC_RIGHT$</td>
  <td>the RIGHT key on the terminal</td>
</tr>
<tr>
  <td>$term.KC_HOME$</td>
  <td>the HOME key on the terminal</td>
</tr>
<tr>
  <td>$term.KC_END$</td>
  <td>the END key on the terminal</td>
</tr>
<tr>
  <td>$term.KC_PAGEUP$</td>
  <td>the PAGE UP key on the terminal</td>
</tr>
<tr>
  <td>$term.KC_PAGEDOWN$</td>
  <td>the PAGE DOWN key on the terminal</td>
</tr>
<tr>
  <td>$term.KC_ENTER$</td>
  <td>the ENTER (CR) key on the terminal</td>
</tr>
<tr>
  <td>$term.KC_TAB$</td>
  <td>the TAB key on the terminal</td>
</tr>
<tr>
  <td>$term.KC_BACKSPACE$</td>
  <td>the BACKSPACE key on the terminal</td>
</tr>
<tr>
  <td>$term.KC_ESC$</td>
  <td>the ESC (escape) key on the terminal</td>
</tr>
</tbody>
</table>
<p>`   
  $term.KC_UP$        $term.KC_DOWN$       $term.KC_LEFT$         $term.KC_RIGHT$    
  $term.KC_HOME$      $term.KC_END$        $term.KC_PAGEUP$       $term.KC_PAGEDOWN$
  $term.KC_ENTER$     $term.KC_TAB$        $term.KC_BACKSPACE$    $term.KC_ESC$
  $term.KC_UP$        $term.KC_DOWN$       $term.KC_LEFT$         $term.KC_RIGHT$
  $term.KC_HOME$      $term.KC_END$        $term.KC_PAGEUP$       $term.KC_PAGEDOWN$
  $term.KC_ENTER$     $term.KC_TAB$        $term.KC_BACKSPACE$    $term.KC_ESC$
  $term.KC_CTRL_Z$    $term.KC_CTRL_A$     $term.KC_CTRL_E$       $term.KC_CTRL_C$
  $term.KC_CTRL_T$    $term.KC_CTRL_U$     $term.KC_CTRL_K$       $term.KC_CTRL_Y$
  $term.KC_CTRL_B$    $term.KC_CTRL_X$     $term.KC_CTRL_V$       $term.KC_DEL$
  $term.KC_F1$        $term.KC_F2$         $term.KC_F3$           $term.KC_F4$
  $term.KC_F5$        $term.KC_F6$         $term.KC_F7$           $term.KC_F8$
  $term.KC_F9$        $term.KC_F10$        $term.KC_F11$          $term.KC_F12$
  $term.KC_CTRL_F1$   $term.KC_CTRL_F2$    $term.KC_CTRL_F4$      $term.KC_INSERT$`]]
    },
    
    
    { sig = "#term.setcolor#( [fgcol, bgcol] )",
      desc = "Set screen color. If called with no arguments the default colors will be set. If called with arguments, both $fgcol$ and $bgcol$ must be specified.",
      args = 
      {
        "$fgcol (optional)$ - the foreground color.",
        "$bgcol (optional)$ - the background color.",
      }
    },

    { sig = "#term.setfg#( [fgcol] )",
      desc = "Set the foreground color.",
      args = "$fgcol (optional)$ - the foreground color (defaut is $term.COL_DEFAULT$)."
    },
    
    { sig = "#term.setbg#( [bgcol] )",
      desc = "Set the background color.",
      args = "$bgcol (optional)$ - the background color (defaut is $term.COL_DEFAULT$)."
    },

    { sig = "#term.setcursor#( type )",
      desc = "Set the cursor type.",
      args = [[$type$ - can be either $term.CURSOR_OFF$ to hide the cursor, $term.CURSOR_BLOCK$ for a blinking block cursor or $term.CURSOR_UNDERLINE$ for a blinking underline cursor.]]
    },

    { sig = "#term.reset#()",
      desc = "Resets the terminal: sets the colors to default, clears the screen, move the cursor home (0, 0) and sets the cursor to a blinking block." 
    },

    { sig = "id, x, y = #term.box#( x, y, width, height, title, [attrs] )",
      desc = "Creates a box (a window) on the screen. The area under the box can be optionally restored when calling @#term.close_box@term.close_box@.",
      args = 
      {
        "$x$ - horizontal box position.",
        "$y$ - vertical box position.",
        "$width$ - box width.",
        "$height$ - box height.",
        "$title$ - the title will be displayed on the first line of the window. It can be an empty string (in which case no title will be displayed).",
        [[$attrs (optional)$ - box attributes. This can be 0 or a combination of the following values:
<ul>
    <li>$term.BOX_BORDER$ - the box will be shown with a surrounding border.</li>
    <li>$term.BOX_RESTORE$ - the screen content under the box will be saved in a buffer and will be restored when calling @#term.close_box@term.close_box@.</li>
    <li>$term.BOX_CENTER$ - centers the box on the screen. In this case the $x$ and $y$ arguments will be ignored.</li>
<ul>
      <p>The default value of this argument is $term.BOX_BORDER + term.BOX_RESTORE + term.BOX_CENTER$.</p>]]
      },
      ret = 
      {
        "$id$ - the ID of the box (needed when calling @#term.close_box@term.close_box@.",
        "$x$ - the final horizontal position of the box.",
        "$y$ - the final horizontal position of the box."
      }
    },

    { sig = "#term.close_box#( id )",
      desc = "Closes a box created with @#term.box@term.box@. If the box was created with attribute $term.BOX_RESTORE$, the screen area under the box will be restored.",
      args = "$id$ - the box ID as returned by @#term.box@term.box@."
    },

    { sig = "#term.setpaging#( [enable] )",
      desc = [[Enables or disables paging mode. In paging mode the terminal will wait for a key press after displaying a full screen of text. This way, data that contains lots of information (such as help pages) can be visualized completely.]],
      args = "$enable$ - $true$ to enable paging, $false$ to disable paging."
    },

    { sig = "#term.getstr#( x, y, prompt, maxlen, [validator] )",
      desc = "Prompts for user input and reads a string from the terminal with optional validation.",
      args = 
      {
        "$x$ - the horizontal position of the prompt.",
        "$y$ - the vertical position of the prompt.",
        "$prompt$ - the user prompt.",
        "$maxlen$ - maximum length of user input.",
        [[$validator (optional)$ - a validator function. If specified, it will be called on each key press with two arguments: the current user input and the key pressed. The validator must return $true$ if the key pressed must be accepted or $false$ if the key pressed must be ignored.]]
      },
      ret = [[The string entered by the user or $nil$ if $ESC$ was pressed while entering the text.]]
    },

    { sig = "#term.set_last_line#( line )",
      desc = [[Sets the 'last line' of the screen. The screen will automatically scroll if text must be written after the 'last line'. Normally set to the number of screen lines - 1 (to take advantage of the whole screen) it can be changed to have only a part of the screen scroll.]],
      args = "$line$ - the last line on the screen."
    },

    { sig = "result = #term.menu#( choices, default, configuration )",
      desc = [[Displays a menu on the screen and lets the user choose an entry from the menu. The menu will scroll automatically (if needed) in its allocated screen space and can call a callback function on each key press.]],
      args = 
      {
        "$choices$ - an array with all the menu entries.",
        "$default$ - the position of the default menu entry. The menu will start with this position selected.",
        [[$configuration$ - a table with various configuration data for the menu:
<ul>
    <li>$x (required)$ - the horizontal position of the menu.</li>
    <li>$y (required)$ - the vertical position of the menu.</li>
    <li>$width (required)$ - the width of the menu.</li>
    <li>$height (required)$ - the height of the menu.</li>
    <li>$fgsel (optional)$ - the foreground color of the currently selected item (the default is the current foreground color).</li>
    <li>$bgsel (optional)$ - the background color of the currently selected item (the default is the current background color).</li>
    <li>$attrs (optional)$ - menu attributes. This can be 0 or a combination of $term.MENU_NO_ENTER$ (the menu won't accept the ENTER key) and $term.MENU_NO_ESC$ (the menu won't accept the ESC key).</li>
    <li>$callback (optional)$ - a function that will be called each time the menu selection changes or a key is pressed. It is called with two arguments: the position of the currently selected item and the key pressed.</li></ul>]]
      },
      ret = "The position of the item selected by the user or $-1$ if ESC was pressed while navigating the menu and $term.MENU_NO_ESC$ was not set as an attribute of the menu."
    },

    { sig = "#term.center#( y, text, [fgcol], [bgcol] )",
      desc = "Centers a text on a line with optional color attributes.",
      args = 
      {
        "$y$ - the line on which the text will be centered.",
        "$text$ - the text to be centered.",
        "$fgcol (optional)$ - text foreground color (default is $term.COL_DONT_CHANGE$)",
        "$bgcol (optional)$ - text background color (default is $term.COL_DONT_CHANGE$)"
      }
    },

    { sig = "#term.clrline#( y, [bgcol] )",
      desc = "Clears a line on the screen and optionally changes its background color.",
      args =
      {
        "$y$ - the line to be cleared.",
        "$bgcol (optional)$ - the new background color of the line. If not specified, the background color is not changed."
      }
    },

    { sig = "key = #term.msgbox#( msg, title, style, [fgcol], [bgcol] )",
      desc = [[Displays a message box with optional user interaction. The message box is automatically centered on screen. The message box can have a number of closing options (pseudo-buttons) specified by the $style$ argument. When the message box is dismissed from the screen, the screen area under the box is automatically restored.]],
      args = 
      {
        "$msg$ - the text to be displayed in the message box.",
        "$title$ - the message box title. It can be an empty string in which case no title will be displayed.",
        [[$style$ - the message box style. It can be 0 or a combination of $term.OK$, $term.YES$, $term.NO$ and $term.CANCEL$. Each of these styles will result in a 'button' with the corresponding text ($OK$, $Yes$, $No$ and $Cancel$ respectively). If one or more buttons are specified, the user will be able to close the box only by pressing the first letter corresponding to the text of the button(s) ($o$, $y$, $n$ and $c$ respectively). If no buttons are specified (style == 0) any key press will close the dialog box.]],
        "$fgcol (optional)$ - the foreground color of the message box (the default is $term.COL_LIGHT_BLUE$).",
        "$bgcol (optional)$ - the background color of the message box (the default is $term.COL_DARK_GRAY$)."
      },
      ret = "The code of the key that was pressed to close the message box."
    },

    { sig = "#term.setattr#( x, y, len, newfg, [newbg] )",
      desc = "Changes the color attributes of an area of the screen. The actual data on the screen is not modified.",
      args =
      {
        "$x$ - the horizontal position of the area.",
        "$y$ - the vertical position of the area.",
        "$len$ - the length (width) of the area.",
        "$newfg$ - the new foreground color of the area.",
        "$newbg (optional)$ - the new background color of the area (the default is $term.COL_DONT_CHANGE$)."
      }
    },

    { sig = "#term.setmode#( mode )",
      desc = [[Sets the terminal mode. The normal mode is $term.MODE_ASCII$ in which all characters with ASCII codes larger than 127 (extended ASCII) are printed just like characters in the standard ASCII set. In the $term.MODE_COLS$ mode the terminal enters a mode in which color attributes can be easily mixed with regular text in order to produce simple colored outputs. In this mode, ASCII chars with codes from 128 to 143 are assigned to foreground colors and ASCII codes with codes from 144 to 159 are assigned to background colors (the color order is the same as the one specified `@#term.colors@here@`in the description of the $term$ module`). Also, ASCII code 255 means "reset the terminal to the default colors" in this mode. As an example, in order to print a text consisting of the words "RED" printed in light red and "BLUE" printed in light blue and then reset to the default attributes when the terminal mode is $term.MODE_COLS$, it's enough to issue the following statement from Lua:
~print( "\140RED \137BLUE\255" )~]],
     args = "$mode$ - the terminal mode. This can be either $term.MODE_ASCII$ or $term.MODE_COLS$."
    },

  },
}

data_pt = data_en
