{{
Video RAM engine
Uses the SPI engine by Beau Schwabe, optimized for speed
}}

CON
    '' Pins
    RAM_MISO_PIN            = 1
    RAM_MISO_MASK           = 1 << RAM_MISO_PIN    
    RAM_MOSI_PIN            = 2
    RAM_MOSI_MASK           = 1 << RAM_MOSI_PIN    
    RAM_CLK_PIN             = 3
    RAM_CLK_MASK            = 1 << RAM_CLK_PIN    
    VBUF_PIN                = 4
    VBUF_MSK                = 1 << VBUF_PIN    
    TEMP_PIN                = 6
    TEMP_PIN_MASK           = 1 << TEMP_PIN

    VMEM_SIZE_LONGS         = (80 * 30 * 2) / 4
    VMEM_SIZE_CHARS         = (80 * 30)
    CURSOR_DEFAULT_MODE     = 2
    CURSOR_DATA_SIZE        = 1

VAR
    long     cog    
    
'------------------------------------------------------------------------------------------------------------------------------
PUB start(SyncPtr, ScreenPtr, ScreenPtr2, CursorPtr) : okay
    stop
    screen_base := ScreenPtr
    screen_base2 := ScreenPtr2
    cursor_ptr := CursorPtr
    okay := cog := cognew(@main, SyncPtr) + 1
    
PUB stop
'' Stop SPI Engine - frees a cog
    if cog
       cogstop(cog~ - 1)
    
'################################################################################################################
DAT           org
'  
'' SPI Engine - main loop
'
main
              ' Setup MOSI and CLK as outputs
              mov     outa,           #0
              or      dira,           #(TEMP_PIN_MASK | VBUF_MSK | RAM_MOSI_MASK | RAM_CLK_MASK)  wz   ' z = 0
             
              ' Actual setup of pin directions
              mov     vbuf_mask,      #VBUF_MSK
              mov     miso_mask,      #RAM_MISO_MASK
              
loop          ' Wait for vertical sync
              rdlong  t0,             par           wz
        if_z  jmp     #loop                         
        
              ' Got vertical sync indication from vgacolour, so change VBUF pin to signal buffer change
              ' and set cursor data
              xor     outa,           vbuf_mask
              wrlong  cursor_data,    cursor_ptr
              or      outa,           #TEMP_PIN_MASK
              
              ' Get destination buffer address
              test    vbuf_mask,      ina           wz
        if_z  mov     dataptr,        screen_base2
       if_nz  mov     dataptr,        screen_base              

              ' NEW CODE - Start copying the data from the serial SRAM to dataptr
              mov     datacnt,        vmem_size
              add     datacnt,        #CURSOR_DATA_SIZE
              mov     cursor_data,    #0            wz
cploop        call    #sram_rd        ' 4 times in a row, it will read a full long (32 bits)
              call    #sram_rd
              call    #sram_rd
              call    #sram_rd
              rol     data, #16
        if_z  mov     cursor_data,    data                         
       if_nz  wrlong  data,           dataptr
       if_nz  add     dataptr,        #4
              djnz    datacnt,        #cploop       wz                         

              ' All done, clear sync indication now
              wrlong  zero,           par
              muxnz    outa,          #TEMP_PIN_MASK
                            
              jmp     #loop
               
'------------------------------------------------------------------------------------------------------------------------------

sram_rd
              ' Unroll loop to maximize speed
              ' 8 bits      
              test    miso_mask,      ina     wc        ''          Read Data Bit into 'C' flag
              rcl     data,           #1                ''          rotate "C" flag into return value
              xor     outa,           #RAM_CLK_MASK 
              xor     outa,           #RAM_CLK_MASK
              test    miso_mask,      ina     wc        ''          Read Data Bit into 'C' flag
              rcl     data,           #1                ''          rotate "C" flag into return value
              xor     outa,           #RAM_CLK_MASK
              xor     outa,           #RAM_CLK_MASK                          
              test    miso_mask,      ina     wc        ''          Read Data Bit into 'C' flag
              rcl     data,           #1                ''          rotate "C" flag into return value
              xor     outa,           #RAM_CLK_MASK
              xor     outa,           #RAM_CLK_MASK                          
              test    miso_mask,      ina     wc        ''          Read Data Bit into 'C' flag
              rcl     data,           #1                ''          rotate "C" flag into return value
              xor     outa,           #RAM_CLK_MASK
              xor     outa,           #RAM_CLK_MASK                          
              test    miso_mask,      ina     wc        ''          Read Data Bit into 'C' flag
              rcl     data,           #1                ''          rotate "C" flag into return value
              xor     outa,           #RAM_CLK_MASK
              xor     outa,           #RAM_CLK_MASK                          
              test    miso_mask,      ina     wc        ''          Read Data Bit into 'C' flag
              rcl     data,           #1                ''          rotate "C" flag into return value
              xor     outa,           #RAM_CLK_MASK
              xor     outa,           #RAM_CLK_MASK                          
              test    miso_mask,      ina     wc        ''          Read Data Bit into 'C' flag
              rcl     data,           #1                ''          rotate "C" flag into return value
              xor     outa,           #RAM_CLK_MASK
              xor     outa,           #RAM_CLK_MASK                          
              test    miso_mask,      ina     wc        ''          Read Data Bit into 'C' flag
              rcl     data,           #1                ''          rotate "C" flag into return value
              xor     outa,           #RAM_CLK_MASK
              xor     outa,           #RAM_CLK_MASK
               ' Consider removing these NOPS, they are here to allow the
               ' host to DMA the new byte, but they might not be needed at all
               nop
               nop
sram_rd_ret    ret

{
########################### Assembly variables ###########################
}
zero                  long            0

vbuf_mask             long            0
miso_mask             long            0
data                  long            0
longdata              long            0
temp                  long            0
datacnt               long            0
vmem_size             long            (VMEM_SIZE_LONGS)
vmem_chars            long            (VMEM_SIZE_CHARS)

' Temps
t0                    long            0

screen_base           long            0
screen_base2          long            0
cursor_ptr            long            0
cursor_data           long            0
dataptr               long            0

{{
                                                   TERMS OF USE: MIT License                                                                                                              
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation     
files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,    
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:                                                                   

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE          
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR         
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,   
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                         

}}
