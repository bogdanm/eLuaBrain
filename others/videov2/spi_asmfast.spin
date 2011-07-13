{{SPI_AsmFast
  This is a modified version of SPI_Asm.
  I removed the delay portions of this object
  to use with fast SPI devices.
  Modifications (slight) by Duane Degn
  on April 9, 2011
  Below are original comments.

  BogdanM - modified heavily from STMBrain
        
************************************************
* Propeller SPI Engine                    v1.2 *
* Author: Beau Schwabe                         *
* Copyright (c) 2009 Parallax                  *
* See end of file for terms of use.            *
************************************************

Revision History:
         V1.0   - original program
         
         V1.1   - fixed problem with SHIFTOUT MSBFIRST option
                - fixed argument allocation in the SPI Engines main loop

         V1.2   - Added Clock delay option and fixed bug in SHIFTIN function       
}}

CON
    '' Pins
    NRF_SPI_MOSI                = 10
    NRF_SPI_MOSI_MASK           = 1 << NRF_SPI_MOSI
    NRF_SPI_MISO                = 11
    NRF_SPI_MISO_MASK           = 1 << NRF_SPI_MISO
    NRF_SPI_CLK                 = 12
    NRF_SPI_CLK_MASK            = 1 << NRF_SPI_CLK

VAR
    long     cog, command
    
PUB rw(data)|val
    command := data << 24 + @data
    repeat while command
    Result := val
    
PUB start : okay
    outa[NRF_SPI_MOSI]~
    outa[NRF_SPI_CLK]~
    dira[NRF_SPI_MOSI]~~
    dira[NRF_SPI_CLK]~~
    dira[NRF_SPI_MISO]~    
    okay := cog := cognew(@loop, @command) + 1
    
PUB stop
'' Stop SPI Engine - frees a cog
    if cog
       cogstop(cog~ - 1)
    command := 0
    
'################################################################################################################
DAT           org
'  
'' SPI Engine - main loop
'
loop          rdlong    t1,     par           wz        '' wait for command
        if_z  jmp       #loop           
              mov       t2,     t1
              and       t2,     address_mask            '' t2 has the argument address
              mov       t3,     #8                      '' number of bits
              mov       t4,     #0                      '' data out
send          rcl       t1,     #1            wc        '' MSB in carry bit
              muxc      outa,   mosi_mask
              xor       outa,   clk_mask                '' send bit
              shl       t4,     #1                      '' prepare position for new bit from device
              nop                                       '' a bit more delay (maybe not needed?)
              nop
              xor       outa,   clk_mask                '' read bit edge (falling)
              test      ina,    miso_mask     wz        '' bit set?
              muxnz     t4,     #1
              djnz      t3,     #send
              '' Done with write/read cycle, send data back to spin
              add       t2,     #4                      '' skip argument, write back data
              wrlong    t4,     t2                      
              wrlong    zero,   par                     '' signal end of operation                                                                                      
NotUsed_      jmp       #loop

'------------------------------------------------------------------------------------------------------------------------------
{
########################### Assembly variables ###########################
}
zero                    long    0
t1                      long    0 
t2                      long    0
t3                      long    0
t4                      long    0
address_mask            long    $00FFFFFF

miso_mask               long    NRF_SPI_MISO_MASK
mosi_mask               long    NRF_SPI_MOSI_MASK
clk_mask                long    NRF_SPI_CLK_MASK

{{
--------------------------------------------------------------------------------------------------------------------------------
-                                                   TERMS OF USE: MIT License                                                  -                                                            
--------------------------------------------------------------------------------------------------------------------------------
-Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation    - 
-files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,    -
-modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software-
-is furnished to do so, subject to the following conditions:                                                                   -
-                                                                                                                              -
-The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.-
-                                                                                                                              -
-THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE          -
-WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR         -
-COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,   -
-ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                         -
--------------------------------------------------------------------------------------------------------------------------------
}}
