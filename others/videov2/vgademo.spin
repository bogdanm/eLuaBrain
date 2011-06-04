CON
        _xinfreq                = 5_000_000                     ' Quartz is 5MHz
        _clkmode                = xtal1 + pll16x                ' System clock is 80MHz

         CHAR_W                 = 80
         CHAR_H                 = 30

OBJ
        vga     : "vgacolour"
        vram    : "vram"

PUB main|tmp, ramcnt, i
        
        ' Start components                    
        vga.start(16, @screen, @screen2, @cursor, @sync)
        vram.start(@sync, @screen, @screen2, @cursor)
                
        repeat
          waitcnt(10)


DAT
        screen          word    $0720[CHAR_W*CHAR_H]
        screen2         word    $0720[CHAR_W*CHAR_H]
        cursor          byte    0,0,%010,0
        sync            long    0                                                                               
