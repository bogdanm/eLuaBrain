''***************************************
''*  VGA High-Res Text Driver v1.0      *
''*  Author: Chip Gracey                *
''*  Copyright (c) 2006 Parallax, Inc.  *
''*  See end of file for terms of use.  *
''***************************************
''
'' This object generates a 640x480 VGA signal which contains 80 columns x 30
'' rows of 8x16 characters. Each character can have a unique forground/background
'' color combination and each character can be inversed and high-lighted.
'' There are also two cursors which can be independently controlled (ie. mouse
'' and keyboard). A sync indicator signals each time the screen is refreshed
'' (you may ignore).
''
'' You must provide buffers for the screen, cursors, and sync. Once started,
'' all interfacing is done via memory. To this object, all buffers are
'' read-only, with the exception of the sync indicator which gets written with
'' -1. You may freely write all buffers to affect screen appearance. Have fun!
''

CON

' 640 x 480 @ 69Hz settings: 80 x 30 characters

        hp = 640        ' horizontal pixels
        vp = 480        ' vertical pixels
        hf = 24         ' horizontal front porch pixels
        hs = 40         ' horizontal sync pixels
        hb = 128        ' horizontal back porch pixels
        vf = 20         ' vertical front porch lines
        vs = 3          ' vertical sync lines
        vb = 17         ' vertical back porch lines
        hn = 1          ' horizontal normal sync state (0|1)
        vn = 1          ' vertical normal sync state (0|1)
        pr = 30         ' pixel rate in MHz at 80MHz system clock (5MHz granularity)

        'hp = 640        ' horizontal pixels
        'vp = 400        ' vertical pixels
        'hf = 24         ' horizontal front porch pixels
        'hs = 40         ' horizontal sync pixels
        'hb = 128        ' horizontal back porch pixels
        '23/6/20
        'vf = 23         ' vertical front porch lines
        'vs = 3          ' vertical sync lines
        'vb = 20         ' vertical back porch lines
        'hn = 1          ' horizontal normal sync state (0|1)
        'vn = 1          ' vertical normal sync state (0|1)
        'pr = 30         ' pixel rate in MHz at 80MHz system clock (5MHz granularity)

    '' Pins
    VBUF_PIN                = 4
    VBUF_MASK               = 1 << VBUF_PIN   

' columns and rows

        cols = hp / 8
        rows = vp / 12

VAR long cog[2]

PUB start(BasePin, ScreenPtr, ScreenPtr2, CursorPtr, SyncPtr) : okay | i, j

'' Start VGA driver - starts two COGs
'' returns false if two COGs not available
''
''      BasePin = VGA starting pin (0, 8, 16, 24, etc.)
''
''      ScreenPtr = Pointer to 80x30 words containing Latin-1 codes and colors for
''              each of the 80x30 screen characters. The lower byte of the word
''              contains the Latin-1 code to display. The upper byte contains
''              the foreground colour in bits 11..8 and the background colour in
''              bits 15..12.
''
''              screen word example: %00011111_01000001 = "A", white on blue
''
''      CursorPtr = Pointer to 6 bytes which control the cursors:
''
''              bytes 0,1,2: X, Y, and MODE of cursor 0
''              bytes 3,4,5: X, Y, and MODE of cursor 1
''
''              X and Y are in terms of screen characters
''              (left-to-right, top-to-bottom)
''
''              MODE uses three bottom bits:
''
''                      %x00 = cursor off
''                      %x01 = cursor on
''                      %x10 = cursor on, blink slow
''                      %x11 = cursor on, blink fast
''                      %0xx = cursor is solid block
''                      %1xx = cursor is underscore
''
''              cursor example: 127, 63, %010 = blinking block in lower-right
''
''      SyncPtr = Pointer to long which gets written with -1 upon each screen
''              refresh. May be used to time writes/scrolls, so that chopiness
''              can be avoided. You must clear it each time if you want to see
''              it re-trigger.

        ' if driver is already running, stop it
        stop

        ' implant pin settings
        reg_vcfg := $200000FF + (BasePin & %111000) << 6
        i := $FF << (BasePin & %011000)
        j := BasePin & %100000 == 0
        reg_dira := i & j
        reg_dirb := i & !j

        ' implant CNT value to sync COGs to
        sync_cnt := cnt + $10000

        ' implant pointers
        screen_base := ScreenPtr
        screen_base2 := ScreenPtr2
        cursor_base := CursorPtr
        font_base := @font

        ' implant unique settings and launch first COG
        vf_lines.byte := vf
        vb_lines.byte := vb
        font_part := 1
        cog[1] := cognew(@d0, SyncPtr) + 1

        ' allow time for first COG to launch
        waitcnt($2000 + cnt)

        ' differentiate settings and launch second COG
        vf_lines.byte := vf+4
        vb_lines.byte := vb-4
        font_part := 0
        cog[0] := cognew(@d0, SyncPtr) + 1

        ' if both COGs launched, return true
        if cog[0] and cog[1]
                return true

        ' else, stop any launched COG and return false
        stop


PUB stop | i

'' Stop VGA driver - frees two COGs

        repeat i from 0 to 1
        if cog[i]
        cogstop(cog[i]~ - 1)


CON

        #1, scanbuff[80], colorbuff[80], scancode[2*80-1+3], maincode   'enumerate COG RAM usage

        main_size = $1F0 - maincode                             'size of main program

        hv_inactive = (hn << 1 + vn) * $0101                    'H,V inactive states 


DAT

'*****************************************************
'* Assembly language VGA high-resolution text driver *
'*****************************************************

' This program runs concurrently in two different COGs.
'
' Each COG's program has different values implanted for front-porch lines and
' back-porch lines which surround the vertical sync pulse lines. This allows
' timed interleaving of their active display signals during the visible portion
' of the field scan. Also, they are differentiated so that one COG displays
' even four-line groups while the other COG displays odd four-line groups.
'
' These COGs are launched in the PUB 'start' and are programmed to synchronize
' their PLL-driven video circuits so that they can alternately prepare sets of
' four scan lines and then display them. The COG-to-COG switchover is seemless
' due to two things: exact synchronization of the two video circuits and the
' fact that all COGs' driven output states get OR'd together, allowing one COG
' to output lows during its preparatory state while the other COG effectively
' drives the pins to create the visible and sync portions of its scan lines.
' During non-visible scan lines, both COGs output together in unison.
'
' COG RAM usage:        $000    = d0 - used to inc destination fields for indirection
'                $001-$050 = scanbuff - longs which hold 4 scan lines
'                $051-$010 = colorbuff - longs which hold colors for 80 characters
'                $0a1-$142 = scancode - stacked WAITVID/SHR for fast display
'                $143-$1EF = maincode - main program loop which drives display

                        org     0                               ' set origin to $000 for start of program

d0                      long    1 << 9                          ' d0 always resides here at $000, executes as NOP


' Initialization code and data - after execution, space gets reused as scanbuff

                        ' Move main program into maincode area

:move                   mov     $1EF, main_begin + main_size - 1
                        sub     :move,d0s0                      ' (do reverse move to avoid overwrite)
                        djnz    main_ctr,#:move

                        ' Build scanbuff display routine into scancode

:waitvid                mov     scancode+0, i0                  ' org   scancode
:shr                    mov     scancode+1, i1                  ' waitvid colorbuff+0, scanbuff+0
                        add     :waitvid, d1                    ' shr   scanbuff+0,#8
                        add     :shr, d1                        ' waitvid colorbuff+1, scanbuff+1
                        add     i0, d0s0                        ' shr   scanbuff+1,#8
                        add     i1, d0                          ' ...
                        djnz    scan_ctr, #:waitvid             ' waitvid colorbuff+cols-1, scanbuff+cols-1

                        mov     scancode+cols*2-1, i2           ' mov   vscl,#hf
                        mov     scancode+cols*2+0, i3           ' waitvid hvsync,#0
                        mov     scancode+cols*2+1, i4           ' jmp   #scanret

                        ' Init I/O registers and sync COGs' video circuits

                        mov     dira, reg_dira                  ' set pin directions
                        mov     dirb, reg_dirb
                        movi    frqa, #(pr / 5) << 2            ' set pixel rate
                        mov     vcfg, reg_vcfg                  ' set video configuration
                        mov     vscl, #1                        ' set video to reload on every pixel
                        waitcnt sync_cnt, colormask             ' wait for start value in cnt, add ~1ms
                        movi    ctra, #%00001_110               ' COGs in sync! enable PLLs now - NCOs locked!
                        waitcnt sync_cnt, #0                    ' wait ~1ms for PLLs to stabilize - PLLs locked!
                        mov     vscl, #100                      ' insure initial WAITVIDs lock cleanly

                        ' Jump to main loop

                        jmp     #vsync                          ' jump to vsync - WAITVIDs will now be locked!

                        ' Data

d0s0                    long    1 << 9 + 1
d1                      long    1 << 10
main_ctr                long    main_size
scan_ctr                long    cols

i0                      waitvid colorbuff+0, scanbuff+0
i1                      shr     scanbuff+0, #8
i2                      mov     vscl, #hf
i3                      waitvid hvsync, #0
i4                      jmp     #scanret

reg_dira                long    0                               ' set at runtime
reg_dirb                long    0                               ' set at runtime
reg_vcfg                long    0                               ' set at runtime
sync_cnt                long    0                               ' set at runtime

                        ' Directives

                        fit     scancode                        ' make sure initialization code and data fit
main_begin              org     maincode                        ' main code follows (gets moved into maincode)


' Main loop, display field - each COG alternately builds and displays four scan lines

vsync                   mov     x, #vs                          ' do vertical sync lines
                        call    #blank_vsync

vb_lines                mov     x, #vb                          ' do vertical back porch lines (# set at runtime)
                        call    #blank_vsync

                        mov     row, #VBUF_MASK
                        test    row, ina          wz
        if_z            mov     screen_ptr, screen_base         ' reset screen pointer to upper-left character
        if_nz           mov     screen_ptr, screen_base2

                        mov     row, #0                         ' reset row counter for cursor insertion
                        mov     fours, #rows * 3 / 2            ' set number of 4-line builds for whole screen

                        ' Build four scan lines into scanbuff

fourline                mov     font_ptr, font_part             ' get address of appropriate font section
                        shl     font_ptr, #8+2
                        add     font_ptr, font_base

                        movd    :pixa, #scanbuff-1              ' reset scanbuff address (pre-decremented)
                        movd    :cola, #colorbuff-1             ' reset colorbuff address (pre-decremented)
                        movd    :colb, #colorbuff-1

                        mov     y, #2                           ' must build scanbuff in two sections because
                        mov     vscl, vscl_line2x               ' ..pixel counter is limited to twelve bits

:halfrow                waitvid underscore, #0                  ' output lows to let other COG drive VGA pins
                        mov     x, #cols/2                      ' ..for 2 scan lines, ready for half a row

:column                 rdword  z, screen_ptr                   ' get character and colors from screen memory
                        mov     bg, z
                        and     z, #$ff                         ' mask character code
                        shl     z, #2                           ' * 4
                        add     z, font_ptr                     ' add font section address to point to 8*4 pixels
                        add     :pixa, d0                       ' increment scanbuff destination addresses
                        add     screen_ptr, #2                  ' increment screen memory address
:pixa                   rdlong  scanbuff, z                     ' read pixel long (8*4) into scanbuff

                        ror     bg, #12                         ' background color in bits 3..0
                        mov     fg, bg                          ' foreground color in bits 31..28
                        shr     fg, #28                         ' bits 3..0
                        add     fg, #fg_clut                    ' + offset to foreground CLUT
                        movs    :cola, fg
                        add     :cola, d0
                        add     bg, #bg_clut                    ' + offset to background CLUT
                        movs    :colb, bg
                        add     :colb, d0
:cola                   mov     colorbuff, 0-0
:colb                   or      colorbuff, 0-0

                        djnz    x, #:column                     ' another character in this half-row?

                        djnz    y, #:halfrow                    ' loop to do 2nd half-row, time for 2nd WAITVID

                        sub     screen_ptr, #2*cols             ' back up to start of same row in screen memory

                        ' Insert cursors into scanbuff

                        mov     z, #2                           ' ready for two cursors

:cursor                 rdbyte  x, cursor_base                  ' x in range?
                        add     cursor_base, #1
                        cmp     x, #cols        wc

                        rdbyte  y, cursor_base                  ' y match?
                        add     cursor_base, #1
                        cmp     y, row          wz

                        rdbyte  y, cursor_base                  ' get cursor mode
                        add     cursor_base, #1

        if_nc_or_nz     jmp     #:nocursor                      ' if cursor not in scanbuff, no cursor

                        add     x, #scanbuff                    ' cursor in scanbuff, set scanbuff address
                        movd    :xor, x

                        test    y, #%010        wc              ' get mode bits into flags
                        test    y, #%001        wz
        if_nc_and_z     jmp     #:nocursor                      ' if cursor disabled, no cursor

        if_c_and_z      test    slowbit, cnt    wc              ' if blink mode, get blink state
        if_c_and_nz     test    fastbit, cnt    wc

                        test    y, #%100        wz              ' get box or underscore cursor piece
        if_z            mov     x, longmask
        if_nz           mov     x, underscore
        if_nz           cmp     font_part, #2   wz              ' if underscore, must be last font section

:xor    if_nc_and_z     xor     scanbuff, x                     ' conditionally xor cursor into scanbuff

:nocursor               djnz    z, #:cursor                     ' second cursor?

                        sub     cursor_base, #3*2               ' restore cursor base

                        ' Display four scan lines from scanbuff

                        mov     y, #4                           ' ready for four scan lines

scanline                mov     vscl, vscl_chr                  ' set pixel rate for characters
                        jmp     #scancode                       ' jump to scanbuff display routine in scancode
scanret                  mov    vscl, #hs                       ' do horizontal sync pixels
                        waitvid hvsync, #1                      ' #1 makes hsync active
                        mov     vscl, #hb                       ' do horizontal back porch pixels
                        waitvid hvsync, #0                      ' #0 makes hsync inactive
                        shr     scanbuff+cols-1, #8             ' shift last column's pixels right by 8
                        djnz    y, #scanline                    ' another scan line?

                        ' Next group of four scan lines

                        add     font_part, #2                   ' if font_part + 2 => 4, subtract 4 (new row)
                        cmpsub  font_part, #3           wc      ' c=0 for same row, c=1 for new row
        if_c            add     screen_ptr, #2*cols             ' if new row, advance screen pointer
        if_c            add     row, #1                         ' if new row, increment row counter
                        djnz    fours, #fourline                ' another 4-line build/display?

                        ' Visible section done, do vertical sync front porch lines

                        wrlong  longmask,par                    ' write -1 to refresh indicator

vf_lines                mov     x,#vf                           ' do vertical front porch lines (# set at runtime)
                        call    #blank

                        jmp     #vsync                          ' new field, loop to vsync

                        ' Subroutine - do blank lines

blank_vsync             xor     hvsync,#$101                    ' flip vertical sync bits

blank                   mov     vscl, hx                        ' do blank pixels
                        waitvid hvsync, #0
                        mov     vscl, #hf                       ' do horizontal front porch pixels
                        waitvid hvsync, #0
                        mov     vscl, #hs                       ' do horizontal sync pixels
                        waitvid hvsync, #1
                        mov     vscl, #hb                       ' do horizontal back porch pixels
                        waitvid hvsync, #0
                        djnz    x,#blank                        ' another line?
blank_ret
blank_vsync_ret
                        ret

                        ' Data

screen_base             long    0                               ' set at runtime (3 contiguous longs)
screen_base2            long    0
cursor_base             long    0                               ' set at runtime

font_base               long    0                               ' set at runtime
font_part               long    0                               ' set at runtime

hx                      long    hp                              ' visible pixels per scan line
vscl_line               long    hp + hf + hs + hb               ' total number of pixels per scan line
vscl_line2x             long    (hp + hf + hs + hb) * 2         ' total number of pixels per 2 scan lines
vscl_chr                long    1 << 12 + 8                     ' 1 clock per pixel and 8 pixels per set
colormask               long    $FCFC                           ' mask to isolate R,G,B bits from H,V
longmask                long    $FFFFFFFF                       ' all bits set
slowbit                 long    1 << 25                         ' cnt mask for slow cursor blink
fastbit                 long    1 << 24                         ' cnt mask for fast cursor blink
underscore              long    $FFFF0000                       ' underscore cursor pattern
hv                      long    hv_inactive                     ' -H,-V states
hvsync                  long    hv_inactive ^ $200              ' +/-H,-V states

bg_clut                 long    %00000011_00000011              ' black
                        long    %00000011_00001011              ' dark blue
                        long    %00000011_00100011              ' dark green
                        long    %00000011_00101011              ' dark cyan
                        long    %00000011_10000011              ' dark red
                        long    %00000011_10001011              ' dark magenta
                        long    %00000011_10100011              ' brown
                        long    %00000011_10101011              ' light gray
                        long    %00000011_01010111              ' dark gray
                        long    %00000011_00001111              ' light blue
                        long    %00000011_00110011              ' light green
                        long    %00000011_00111111              ' light cyan
                        long    %00000011_11000011              ' light red
                        long    %00000011_11001111              ' light magenta
                        long    %00000011_11110011              ' light yellow
                        long    %00000011_11111111              ' white

fg_clut                 long    %00000011_00000011              ' black
                        long    %00000111_00000011              ' dark blue
                        long    %00010011_00000011              ' dark green
                        long    %00010111_00000011              ' dark cyan
                        long    %01000011_00000011              ' dark red
                        long    %01000111_00000011              ' dark magenta
                        long    %01010011_00000011              ' brown
                        long    %10101011_00000011              ' light gray
                        long    %01010111_00000011              ' dark gray
                        long    %00001011_00000011              ' blue
                        long    %00100011_00000011              ' green
                        long    %00101011_00000011              ' cyan
                        long    %10000011_00000011              ' red
                        long    %10001011_00000011              ' magenta
                        long    %10100011_00000011              ' yellow
                        long    %11111111_00000011              ' white

                        ' Uninitialized data

screen_ptr              res     1
font_ptr                res     1

x                       res     1
y                       res     1
z                       res     1
fg                      res     1
bg                      res     1

row                     res     1
fours                   res     1
'sequence                res     1


                        fit     $1f0

' 8 x 12 font - characters 0..127
'
' Each long holds four scan lines of a single character. The longs are arranged into
' groups of 128 which represent all characters (0..127). There are four groups which
' each contain a vertical part of all characters. They are ordered top, middle, and
' bottom.

font    long
  long $00000000,$81c37e00,$ffff7e00,$ee440000,$7c381000,$3c3c1800,$7e3c1800,$00000000
  long $ffffffff,$fc780000,$81c3ffff,$b8e0f800,$cccc7800,$9898f800,$fec6fe00,$db180000
  long $0e060200,$e0c08000,$fc783000,$cccccc00,$dbdbfe00,$0cc67e00,$00000000,$fc783000
  long $fc783000,$30303000,$30000000,$18000000,$00000000,$24000000,$10100000,$fefe0000
  long $00000000,$f0f06000,$cccccc00,$fe6c6c00,$0cf83030,$8c000000,$36361c00,$c0c0c000
  long $3060c000,$60301800,$66000000,$30000000,$00000000,$00000000,$00000000,$c0800000
  long $e6c67c00,$3c302000,$cccc7800,$c0cc7800,$78706000,$0c0cfc00,$0c187000,$c6c6fe00
  long $cccc7800,$cccc7800,$e0000000,$e0000000,$3060c000,$00000000,$30180c00,$c0cc7800
  long $c6c67c00,$cc783000,$cccc7e00,$c6cc7800,$cc6c3e00,$0c8cfe00,$8cccfe00,$c6cc7800
  long $cccccc00,$6060f000,$6060f000,$6cccce00,$0c0c1e00,$feeec600,$cec6c600,$c66c3800
  long $cccc7e00,$c66c3800,$cccc7e00,$cccc7800,$30b4fc00,$cccccc00,$cccccc00,$c6c6c600
  long $cccccc00,$cccccc00,$32e6fe00,$3030f000,$06020000,$c0c0f000,$c66c3810,$00000000
  long $00c06060,$00000000,$0c0c0e00,$00000000,$60607000,$00000000,$18d87000,$00000000
  long $0c0c0e00,$00303000,$00c0c000,$0c0c0e00,$30303c00,$00000000,$00000000,$00000000
  long $00000000,$00000000,$00000000,$00000000,$18100000,$00000000,$00000000,$00000000
  long $00000000,$00000000,$00000000,$3030e000,$c0c0c000,$30301c00,$735bce00,$10000000
  long $cccc7800,$00666600,$003060c0,$00663c18,$00666600,$00180c06,$386c6c38,$00000000
  long $00cc7830,$00cccc00,$0030180c,$006c6c00,$006c3810,$0030180c,$3000cc00,$78cccc78
  long $fc3060c0,$00000000,$363cf800,$00cc7830,$00cccc00,$0030180c,$00663c18,$00180c06
  long $00cccc00,$cc7800cc,$cccc00cc,$78303000,$0c0ccc78,$cccccccc,$1111110f,$1818d870
  long $00183060,$00183060,$003060c0,$00183060,$0076dc00,$c60076dc,$66663c00,$66663c00
  long $00303000,$00000000,$00000000,$33634200,$3667c600,$00606000,$00000000,$00000000
  long $24924924,$55aa55aa,$b66ddbb6,$c0c0c0c0,$c0c0c0c0,$c0c0c0c0,$cccccccc,$00000000
  long $00000000,$cccccccc,$cccccccc,$00000000,$cccccccc,$cccccccc,$c0c0c0c0,$00000000
  long $18181818,$18181818,$00000000,$18181818,$00000000,$18181818,$18181818,$66666666
  long $66666666,$00000000,$66666666,$00000000,$66666666,$00000000,$66666666,$18181818
  long $66666666,$00000000,$00000000,$66666666,$18181818,$00000000,$00000000,$66666666
  long $18181818,$c0c0c0c0,$00000000,$ffffffff,$00000000,$f0f0f0f0,$f0f0f0f0,$ffffffff
  long $00000000,$cccc7800,$ccccfc00,$6c6cfe00,$988cfc00,$00000000,$00000000,$dc000000
  long $7830fc00,$cccc7800,$c6c67c00,$3018f000,$6e000000,$7cc00000,$0c18f000,$cc780000
  long $00fc0000,$30300000,$60301800,$18306000,$d8700000,$c0c0c0c0,$30300000,$dbce0000
  long $cccc7800,$00000000,$00000000,$2020e000,$d8d86c00,$60c07800,$f0f00000,$00000000

  long $00000000,$99bd81a5,$e7c3ffdb,$7cfefefe,$387cfefe,$18e7e7ff,$187effff,$78fcfc78
  long $c38181c3,$cc8484cc,$99bdbd99,$66663e9c,$fc3078cc,$1e1818f8,$e6c6c6c6,$7ee7e77e
  long $0e3efe3e,$e0f8fef8,$fc303030,$0000cccc,$d8d8dedb,$3c66663c,$fe000000,$fc303030
  long $30303030,$fc303030,$3060fe60,$180cfe0c,$fe060606,$2466ff66,$7c7c3838,$38387c7c
  long $00000000,$006060f0,$00000048,$fe6c6c6c,$c0c0780c,$183060cc,$66f6be1c,$00000060
  long $30181818,$60c0c0c0,$663cff3c,$3030fc30,$00000000,$0000fe00,$00000000,$0c183060
  long $ceded6f6,$30303030,$183060c0,$c0c070c0,$60fe666c,$c0c07c0c,$cccc7c0c,$183060c0
  long $ccec78dc,$6060f8cc,$e00000e0,$e00000e0,$30180c18,$00fc00fc,$3060c060,$00303060
  long $06f6f6f6,$ccfccccc,$cccc7ccc,$c6060606,$cccccccc,$0c4c7c4c,$0c4c7c4c,$c6e60606
  long $ccccfccc,$60606060,$66666060,$6c6c3c6c,$cc8c0c0c,$c6c6d6fe,$e6f6fede,$c6c6c6c6
  long $0c0c7ccc,$f6e6c6c6,$cc6c7ccc,$cc60380c,$30303030,$cccccccc,$cccccccc,$6cd6d6c6
  long $cc783078,$303078cc,$8c0c1830,$30303030,$6030180c,$c0c0c0c0,$00000000,$00000000
  long $00000000,$667c603c,$cccccc7c,$0c0ccc78,$6666667c,$0cfccc78,$18187c18,$666666dc
  long $ccccdc6c,$3030303c,$c0c0c0f0,$6c3c6ccc,$30303030,$d6d6d67e,$cccccc7c,$cccccc78
  long $cccccc76,$666666dc,$0cdcec6e,$6018cc78,$181818fc,$66666666,$cccccccc,$d6d6c6c6
  long $38386cc6,$cccccccc,$1860c4fc,$30180c18,$c0c000c0,$3060c060,$00000000,$c6c66c38
  long $cc0c0c0c,$66666666,$0cfccc78,$667c603c,$667c603c,$667c603c,$667c603e,$0c0ccc78
  long $0cfccc78,$0cfccc78,$0cfccc78,$3030303c,$3030303c,$3030303c,$fccccc78,$fccccc78
  long $0c7c0c8c,$1bfed87f,$3636fe36,$cccccc78,$cccccc78,$cccccc78,$66666666,$66666666
  long $cccccccc,$cccccccc,$cccccccc,$cc0c0ccc,$0c0c7e0c,$fc30fc78,$3179110f,$1818187e
  long $667c603c,$3030303c,$cccccc78,$66666666,$6666663e,$e6f6dece,$00fe00fc,$00fe003c
  long $0c0c1830,$0c0cfc00,$c0c0fc00,$c3760c1b,$dbf6ec1e,$f0f06060,$333366cc,$cccc6633
  long $49249249,$55aa55aa,$dbb66ddb,$c0c0c0c0,$c0c0f8c0,$f8c0c0f8,$cccccecc,$ccccfe00
  long $f8c0c0f8,$cec0c0ce,$cccccccc,$cec0c0fe,$fec0c0ce,$0000fecc,$f8c0c0f8,$c0c0f800
  long $0000f818,$0000ff18,$1818ff00,$1818f818,$0000ff00,$1818ff18,$f81818f8,$6666e666
  long $fe0606e6,$e60606fe,$ff0000e7,$e70000ff,$e60606e6,$ff0000ff,$e70000e7,$ff0000ff
  long $0000ff66,$ff0000ff,$6666ff00,$0000fe66,$f81818f8,$f81818f8,$6666fe00,$6666e766
  long $ff0000ff,$0000f8c0,$1818f800,$ffffffff,$ffff0000,$f0f0f0f0,$f0f0f0f0,$0000ffff
  long $6666f6dc,$cccccc6c,$0c0c0c0c,$6c6c6c6c,$98183018,$666626fc,$66666666,$30303076
  long $78cccccc,$ccccfccc,$6c6cc6c6,$cccccc78,$76dbdbdb,$7cded6f6,$0c0cfc0c,$cccccccc
  long $0000fc00,$003030fc,$00183060,$00603018,$181818d8,$d8c0c0c0,$3000fc00,$dbce0073
  long $000078cc,$0000e0e0,$0000c000,$2c262220,$0000d8d8,$0000f830,$f0f0f0f0,$00000000

  long $00000000,$00007ec3,$00007eff,$00001038,$00000010,$00007e18,$00007e18,$00000000
  long $ffffffff,$000078fc,$ffffc381,$00003c66,$00003030,$00000e1f,$000367e7,$000018db
  long $00000206,$000080c0,$00003078,$0000cccc,$0000d8d8,$007e6330,$0000fefe,$00fc3078
  long $00003030,$00003078,$00000000,$00000000,$00000000,$00000000,$0000fefe,$00001010
  long $00000000,$00006060,$00000000,$00006c6c,$0030307c,$0000c4cc,$0000dc76,$00000000
  long $0000c060,$00001830,$00000000,$00000000,$0030e0e0,$00000000,$0000e0e0,$00000206
  long $00007cc6,$0000fc30,$0000fccc,$000078cc,$0000f060,$000078cc,$000078cc,$00001818
  long $000078cc,$00003830,$000000e0,$0060c0e0,$0000c060,$00000000,$00000c18,$00003030
  long $00007c06,$0000cccc,$00007ecc,$000078cc,$00003e6c,$0000fe8c,$00001e0c,$0000f8cc
  long $0000cccc,$0000f060,$00003c66,$0000cecc,$0000fecc,$0000c6c6,$0000c6c6,$0000386c
  long $00001e0c,$00f0607c,$0000cecc,$000078cc,$00007830,$000078cc,$00003078,$00006c6c
  long $0000cccc,$00007830,$0000fec6,$0000f030,$000080c0,$0000f0c0,$00000000,$00ff0000
  long $00000000,$0000dc66,$000076cc,$000078cc,$0000dc66,$000078cc,$00003c18,$3c66607c
  long $0000cecc,$0000fc30,$78ccccc0,$0000cecc,$0000fc30,$0000c6d6,$0000cccc,$000078cc
  long $1e0c7ccc,$f0607c66,$00001e0c,$000078cc,$000070d8,$0000dc66,$00003078,$00006c6c
  long $0000c66c,$1e306078,$0000fc8c,$0000e030,$0000c0c0,$00001c30,$00000000,$000000fe
  long $3c3078cc,$0000dc66,$000078cc,$0000dc66,$0000dc66,$0000dc66,$0000dc66,$3c3078cc
  long $0000f80c,$0000f80c,$0000f80c,$0000fc30,$0000fc30,$0000fc30,$0000cccc,$0000cccc
  long $0000fc8c,$0000f71b,$0000f636,$000078cc,$000078cc,$000078cc,$0000dc66,$0000dc66
  long $1e306078,$000078cc,$000078cc,$00303078,$0000fe06,$00003030,$000061b1,$00000e1b
  long $0000dc66,$0000fc30,$000078cc,$0000dc66,$00006666,$0000c6c6,$00000000,$00000000
  long $000078cc,$0000000c,$000000c0,$00f83061,$00c0fccd,$000060f0,$0000cc66,$00003366
  long $92492492,$55aa55aa,$6ddbb66d,$c0c0c0c0,$c0c0c0c0,$c0c0c0c0,$cccccccc,$cccccccc
  long $c0c0c0c0,$cccccccc,$cccccccc,$cccccccc,$00000000,$00000000,$00000000,$c0c0c0c0
  long $00000000,$00000000,$18181818,$18181818,$00000000,$18181818,$18181818,$66666666
  long $00000000,$66666666,$00000000,$66666666,$66666666,$00000000,$66666666,$00000000
  long $00000000,$18181818,$66666666,$00000000,$00000000,$18181818,$66666666,$66666666
  long $18181818,$00000000,$18181818,$ffffffff,$ffffffff,$f0f0f0f0,$f0f0f0f0,$00000000
  long $0000dcf6,$00180c7c,$00000c0c,$0000cc6c,$0000fc8c,$00003c66,$0306de66,$0000e030
  long $0000fc30,$000078cc,$0000ee6c,$000078cc,$00000000,$00000006,$0000f018,$0000cccc
  long $000000fc,$000000fc,$000000fc,$000000fc,$18181818,$000070d8,$00000030,$00000073
  long $00000000,$00000000,$00000000,$00003038,$00000000,$00000000,$0000f0f0,$00000000


{{
+------------------------------------------------------------------------------------------------------------------------------+
|                                   TERMS OF USE: Parallax Object Exchange License                                             |
+------------------------------------------------------------------------------------------------------------------------------+
|Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation    | |files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,    |
|modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software|
|is furnished to do so, subject to the following conditions:                                                                   |
|                                                                                                                              |
|The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.|
|                                                                                                                              |
|THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE          |
|WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR         |
|COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,   |
|ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                         |
+------------------------------------------------------------------------------------------------------------------------------+
}}