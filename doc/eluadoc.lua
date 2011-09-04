-------------------------------------------------------------------------------
-- eLua doc builder module (for the eluadoc/ directory)

module( ..., package.seeall )

local sf = string.format

-- True if generating HTML docs, false otherwise
local htmld

-------------------------------------------------------------------------------
-- Data structure declarations

-- List here all the sections for which we're generating the documentation
local doc_sections_html = { "arch_platform", "refman_gen", "refman_ps_lm3s", "refman_ps_str9", "refman_ps_mbed", "refman_ps_mizar32", "refman_ps_stm32" }
local doc_sections_target = { "refman_gen", "refman_ps_stm32" }
local target_doc_modules =  { "elua", "cpu", "pd", "bit", "pack", "uart", "spi", "pio", "pwm", "i2c", "tmr", "term", "can", "nrf", "adc", "snd", "net" }

-- List here all the components of each section
local components = 
{ 
  arch_platform = { "ll", "pio", "spi", "uart", "timers", "pwm", "cpu", "eth", "adc", "i2c", "can" },
  refman_gen = { "bit", "pd", "cpu", "pack", "adc", "term", "pio", "uart", "spi", "tmr", "pwm", "net", "can", "rpc", "elua", "i2c", "nrf" },
  refman_ps_lm3s = { "disp" },
  refman_ps_str9 = { "pio", "rtc" },
  refman_ps_mbed = { "pio" },
  refman_ps_mizar32 = { "lcd" },
  refman_ps_stm32 = { "snd" }
}

-------------------------------------------------------------------------------
-- Generic helpers and doc text formatting functions

-- Format a name to a link by changing all the spaces to "_" and
-- making all letters lowercase
local function name2link( str )
  str = str:gsub( " ", "_" )
  return str:lower()
end

-- Returns the part of the string enclosed between two '#' chars
-- Used for parsing function sig. 
local function namefromsig( str )
  local _, _, name = str:find( "#(.*)#" )
  return name
end

-- Adds a "." to the end of the string if it's not already present
local function dot( str )
  -- return str:sub( -1 ) == "." and str or str .. "."
  return str
end

-- Check if an element is part of a list
local function is_in_list( el, list )
  for i = 1, #list do
    if list[ i ] == el then return true end
  end
end

--[[ Process the given string as follows:
- $string$ becomes <b>string</b>
- %string% becomes <i>string</i>
- @ref@text@ becomes <a href="ref">text</a>
- ^ref^text^ also becomes <a href="ref">text</a>
- `html`target` gives a text that will be different in the online and target versions
- $$, %%, @@, ^^, ~~ become $, %, @, ^, ~ respectively
- the string "eLua" becomes <b>eLua</b>
- strings between two tildas (~~) get special code-like formatting
- newlines are changed to ' ' if 'keepnl' isn't true
- '&' is translated to its corresponding HTML code.
- '<<' and '>>" are also translated to the corresponding HTML codes (note the repetition).
--]]
local function format_string( str, keepnl )
  -- replace double "special chars" with "temps" for later use
  str = str:gsub( "%$%$", "\001" )
  str = str:gsub( "%%%%", "\002" )
  str = str:gsub( "@@", "\003" )
  str = str:gsub( "%^%^", "\004" )
  str = str:gsub( "~~", "\005" )
  str = str:gsub( "``", "\007" )

  -- In target mode add spaces in front of <li></li> items
  if not htmld then str = str:gsub( "<li>(.-)</li>", "  - %1" ) end

  -- In target mode remove all HTML tags
  if not htmld then str = str:gsub( "<br%s-/>", "" ) end
  if not htmld then str = str:gsub( "<(%S-)>", function( t ) return t:sub( 1, 1 ) == "<" and "<" .. t .. ">" or "" end ) end

  -- Translate 'special' HTML chars to their equivalents
  local tr_table_html = { [ "%&" ] = "&amp;" }
  local tr_table_online = { [ "%&" ] = "&" }
  for char, rep in pairs( htmld and tr_table_html or tr_table_online ) do
    str = str:gsub( char, rep )
  end

  -- some double chars are replaced directly with their HTML codes
  str = str:gsub( "<<", htmld and "&lt;" or "<" )
  str = str:gsub( ">>", htmld and "&gt;" or ">" )

  -- replace eLua with <b>eLua</b>
  str = str:gsub( "eLua", htmld and "<b>eLua</b>" or "eLua" )

  -- $string$ becomes <b>string></b>
  str = str:gsub( "%$(.-)%$", htmld and "<b>%1</b>" or CLMAGENTA .. "%1" .. CRESET )

  -- %string% becomes <i>string</i>
  str = str:gsub( "%%(.-)%%", htmld and "<i>%1</i>" or CLGREEN .. "%1" .. CRESET )

  -- `html`target` is selected depeding on htmld
  str = str:gsub( "`(.-)`(.-)`", htmld and '%1' or '%2'  )

  -- @ref@text@ becomes <a href="ref">text</a>
  str = str:gsub( "@(.-)@(.-)@", htmld and '<a href="%1">%2</a>' or CLCYAN .. '%2' .. CRESET  )

  -- ^ref^text^ becomes <a href="ref">text</a>
  str = str:gsub( "%^(.-)%^(.-)%^", htmld and '<a href="%1">%2</a>' or CLCYAN .. '%2' .. CRESET )

  -- strings between two tildas (~~) get special code-like formatting
  -- must keep '\n', so replace it with "temps" for now
  str = str:gsub( "~(.-)~", function( data ) return htmld and '<pre class="code">' .. data:gsub( "\n", "\006" ) .. "</pre>" or CLYELLOW .. data .. CRESET end )
  str = str:gsub( "~~", "~" )

  -- other "\n" chars should dissapear now
  if not keepnl and htmld then str = str:gsub( "\n", " " ) end

  -- In target mode remove all empty lines
  if not htmld then str = str:gsub( "([^\n]-\n)", function( data ) return data:sub( 1, -2 ):find( "%S" ) and data or ""  end ) end

  -- put back the "temps"
  str = str:gsub( "\001", "%$" )
  str = str:gsub( "\002", "%%" )
  str = str:gsub( "\003", "@" )
  str = str:gsub( "\004", "%^" )
  str = str:gsub( "\005", "~" )
  str = str:gsub( "\006", "\n" )
  str = str:gsub( "\007", "`" )

  -- all done
  return str
end

-------------------------------------------------------------------------------
-- Content generation

-- Build the documentation starting from the given file
local function build_file( fname, section )
  dofile( fname )
  local res = {}

  for _, lang in pairs( languages ) do
    res[ lang ] = {}
    res[ lang ].menu = {}
    res[ lang ].data = {}
    local menu = res[ lang ].menu
    local data = res[ lang ].data

    -- we need english always
    -- the other languages will be substituted with english if not found
    local resname = string.format( "data_%s", lang )
    local r = _G[ resname ]
    if not r then
      if lang == "en" then
        return false, "data_en must be part of the description"
      else
        print( string.format( "'%s': data for language '%s' not found, defaulting to english", fname, lang ) )
        r = _G.data_en
      end
    end

    -- Get the module name and overview
    data.mod_name = r.mod_name or r.menu_name
    data.mod_overview = r.desc or ""

    -- process names
    if not r.menu_name then
      return false, "menu_names not found"
    end
    menu.name = r.menu_name

    -- process title
    if not r.title then
      return false, "title not found"
    end
    local page = "$$HEADER$$\n"
    menu.title = r.title

    -- process overview
    if not r.overview then
      return false, "overview not found"
    end
    local temp = format_string( r.overview )
    page = page .. '<a name="overview" /><h3>Overview</h3>\n<p>' .. temp .. "</p>\n\n"
    data.mod_desc = format_string( r.target_overview or r.overview ) .. "\n"

    -- process structures if needed
    if r.structures then
      local structures = r.structures
      menu.structs = {}
      page = page .. '<a name="structures" /><h3>Data structures, constants and types</h3>\n'
      data.mod_desc = data.mod_desc .. CLMAGENTA .. "\n== Data structures and types ==\n\n" .. CRESET
      for i = 1, #structures do
        local s = structures[ i ]
        menu.structs[ #menu.structs + 1 ] = s.name
        if not s.text or not s.desc or not s.name then
          return false, "structure without text, desc or name fields"
        end
        -- text/name. The link name is ALWAYS the one in ENGLISH.
        page = page .. string.format( '<a name="%s" />', name2link( res.en.menu.structs[ i ] ) )
        page = page .. "<pre><code>" .. format_string( s.text, true ) .. "</code></pre>\n"
        -- description
        temp = format_string( s.desc )
        page = page .. '<div class="docdiv">\n<p>' .. format_string( temp ) .. "</p>\n</div>\n\n"
        temp = target_desc or temp
        -- data.mod_desc = data.mod_desc .. CLBLUE .. s.name .. ":\n" .. CRESET .. temp .. "\n"
        data.mod_desc = data.mod_desc .. CLBLUE .. s.name .. ":\n" .. CRESET
        local t2 = format_string( s.text )
        if t2:sub( -1, -1 ) ~= "\n" then t2 = t2 .. "\n" end
        for l in t2:gmatch( "([^\n]-)\n" ) do data.mod_desc = data.mod_desc .. "  " .. CLCYAN .. l .. "\n" .. CRESET end
        data.mod_desc = data.mod_desc .. temp .. "\n"
      end 
    end
    data.funcs = {}
    -- process functions now
    if not r.funcs then
      return false, "funcs not found"
    end
    local funcs = r.funcs
    page = page .. '<a name="funcs" /><h3>Functions</h3>\n<div class="docdiv">\n'
    menu.funcs = {}
    for i = 1, #funcs do
      local f = funcs[ i ]
      local df = {}
      if not f.sig or not f.desc then
        return false, "function without sig or desc fields"
      end
      local funcname = namefromsig( f.sig )
      if not funcname then
        return false, string.format( "'%s' should contain the function name between '#' chars", f.sig )
      end
      df.name = funcname
      df.desc = CLBLUE .. f.sig:gsub( "#(.-)#", CLYELLOW .. "%1" .. CLBLUE ) .. "\n  " .. CRESET .. dot( format_string( f.target_desc or f.desc ) ) .. "\n"
      menu.funcs[ #menu.funcs + 1 ] = funcname
      -- signature
      page = page .. string.format( '<a name="%s" />', funcname )
      page = page .. "<pre><code>" .. f.sig:gsub( '#', '' ) .. "</code></pre>"
      -- description
      page = page .. "\n<p>" .. dot( format_string( f.desc ) ) .. "</p>\n"
      -- arguments
      page = page .. "<p><b>Arguments</b>: "
      if f.args then
        local a = f.args
        if type( a ) == "string" or ( type( a ) == "table" and #a == 1 ) then
          local text = type( a ) == "string" and a or a[ 1 ]
          page = page .. dot( format_string( text ) ) .. "</p>"
          df.desc = df.desc .. CLGREEN .. "  Arguments:\n" .. CRESET .. "    " .. dot( format_string( text ) ) .. "\n"
        else
          page = page .. "</p>\n<ul>\n"
          df.desc = df.desc .. CLGREEN .. "  Arguments:\n" .. CRESET
          for i = 1, #a do 
            page = page .. "  <li>" .. dot( format_string( a[ i ] ) ) .. "</li>\n" 
            df.desc = df.desc .. "    * " .. dot( format_string( a[ i ] ) ) .. "\n"
          end
          --df.desc = df.desc .. "\n"
          page = page .. "</ul>"
        end
      else
        page = page .. "none.</p>"
        df.desc = df.desc .. CLGREEN .. "  Arguments: " .. CRESET .. "none.\n"
      end
      page = page .. "\n"
      -- return value
      page = page .. "<p><b>Returns</b>: "
      if f.ret then
        local r = f.ret
        if type( r ) == "string" or ( type( r ) == "table" and #r == 1 ) then
          local text = type( r ) == "string" and r or r[ 1 ]
          page = page .. dot( format_string( text ) ) .. "</p>"
          df.desc = df.desc .. CLGREEN .. "  Returns:\n" .. CRESET .. "    " .. dot( format_string( text ) )  .. "\n"
        else
          page = page .. "</p>\n<ul>\n"
          df.desc = df.desc .. CLGREEN .. "  Returns:\n" .. CRESET
          for i = 1, #r do 
            page = page .. "  <li>" .. dot( format_string( r[ i ] ) ) .. "</li>\n"
            df.desc = df.desc .. "    * " .. dot( format_string( r[ i ] ) ) .. "\n"
          end
          -- df.desc = df.desc .. "\n"
          page = page .. "</ul>"
        end
      else
        page = page .. "nothing.</p>"
        df.desc = df.desc .. CLGREEN .. "  Returns: " .. CRESET .. "nothing.\n"
      end
      page = page .. "\n\n"
      -- print( df.name )
      -- print( df.desc )
      -- os.exit( 0 )
      data.funcs[ #data.funcs + 1 ] = df
    end
    page = page .. "</div>\n"

    -- aux data (if any)
    -- TOOD: integrate this in the online docs?
    if r.auxdata then
      local auxdata = r.auxdata
      menu.auxdata = {}
      for i = 1, #auxdata do
        local a = auxdata[ i ]
        menu.auxdata[ #menu.auxdata + 1 ] = a.title
        if not a.title or not a.desc then
          return false, "auxdata without title or desc"
        end
        -- the link name is ALWAYS the one in ENGLISH
        page = page .. string.format( '<a name="%s" />', name2link( res.en.menu.auxdata[ i ] ) )
        page = page .. "<h3>" .. a.title .. "</h3>"
        page = page .. "\n<p>" .. format_string( a.desc ) .. "</p>\n\n"
      end
    end

    -- footer
    page = page .. "$$FOOTER$$\n"
    -- Cleanup: remove "<p></p>" (which might appear due to formatting)
    page = page:gsub( "<p>%s-</p>", "" )
    res[ lang ].page = page
  end
  --print( res.en.data.mod_name )
  --print( res.en.data.mod_desc )
  return res
end

-------------------------------------------------------------------------------
-- Menu generation

-- Helper function to get strings in all languages when needed
local function all_langs( getstr )
  local langs =  {}
  for _, lang in pairs( languages ) do
    langs[ #langs + 1 ] = getstr( lang )
  end
  return langs
end

-- Transform the data from the menu dictionary (in 'fulldata') for component 'component' and section 'sect' to a menu structure
local function gen_menu( fulldata, component, sect )
  local relfname = sect .. "_" .. component .. ".html"
  local res = fulldata[ component ]
  local themenu = { all_langs( function( x ) return res[ x ].menu.name end ), relfname, {}, all_langs( function( x ) return res[ x ].menu.title end ) }
  local sub = themenu[ submenu_idx ] 
  
  -- Overview
  sub[ #sub + 1 ] = { all_langs( function( x ) return getstr( "Overview", x ) end ), sf( "%s#overview", relfname ) }

  -- Data structures (if needed)
  if res.en.menu.structs then
    sub[ #sub + 1 ] = { all_langs( function( x ) return getstr( "Data structures", x ) end ), sf( "%s#structures", relfname ), {} }
    local s_sub = sub[ #sub ][ submenu_idx ]
    for i = 1, #res.en.menu.structs do
      local v = res.en.menu.structs[ i ]
      s_sub[ #s_sub + 1 ] = { all_langs( function( x ) return res[ x ].menu.structs[ i ] end ), sf( "%s#%s", relfname, name2link( v ) ) }
    end
  end

  -- Functions
  sub[ #sub + 1 ] = { all_langs( function( x ) return getstr( "Functions", x ) end ), sf( "%s#funcs", relfname ), {} }
  local f_sub = sub[ #sub ][ submenu_idx ]
  for _, v in pairs( res.en.menu.funcs ) do
    f_sub[ #f_sub + 1 ] = { all_langs( function( x ) return v end ), sf( "%s#%s", relfname, name2link( v ) ) }
  end

  -- Aux data (if needed)
  if res.en.menu.auxdata then
    for i = 1, #res.en.menu.auxdata do
      local v = res.en.menu.auxdata[ i ]
      sub[ #sub + 1 ] = { all_langs( function( x ) return res[ x ].menu.auxdata[ i ] end ), sf( "%s#%s", relfname, name2link( v ) ) }
    end
  end

  return themenu
end

-------------------------------------------------------------------------------
-- Generate documentation from eluadoc for all languages

function gen_html_doc( is_target )
  local menu, genfiles = {}, {}

  htmld = not is_target
  for k, v in pairs( components ) do
    table.sort( v )
  end
  
  local targetdata = {}
  for _, section in pairs( htmld and doc_sections_html or doc_sections_target ) do 
    -- Generate documentation for each module in turn
    local fulldata = {}
    menu[ section ] = {}
    local ms = menu[ section ]
    -- First generate HTML documentation
    for _, modname in pairs( components[ section ] ) do
      if htmld or is_in_list( modname, target_doc_modules ) then
        local descfname = string.format( "eluadoc/%s_%s.lua", section, modname )
        local res, err = build_file( descfname, section )
        if res then
          fulldata[ modname ] = res
          targetdata[ modname ] = res
          -- Write doc for each language
          if htmld then
            for _, lang in pairs( languages ) do
              local fname = string.format( "%s/%s_%s.html", lang, section, modname )
              local f = io.open( fname, "wb" )
              if not f then
                print( string.format( "Unable to open %s for writing", fname ) )
                return
              else
                f:write( res[ lang ].page )
                f:close()
                print( ( "Wrote %s" ):format( fname ) )
                genfiles[ #genfiles + 1 ] = fname
              end
            end
          else
            print( string.format( "Processed file %s", descfname ) )
          end
        else
          print( string.format( "Error processing module '%s': %s", modname, err ) )
          return
        end
      end
    end 

    -- Then generate menu data
    if htmld then
      for _, modname in pairs( components[ section ] ) do
        local submenu= gen_menu( fulldata, modname, section )
        ms[ #ms + 1 ] = submenu
      end
    end
  end
  return menu, genfiles, targetdata
end

