''********************************************
''*  Full-Duplex Serial Driver v1.2          *
''*  Author: Chip Gracey, Jeff Martin        *
''*  Copyright (c) 2006-2009 Parallax, Inc.  *
''*  See end of file for terms of use.       *
''********************************************

{-----------------REVISION HISTORY-----------------
 v1.2 - 5/7/2009 fixed bug in dec method causing largest negative value (-2,147,483,648) to be output as -0.
 v1.1 - 3/1/2006 first official release.

 RR20090815  _rr001   Allow variable buffer size in CON
                      Add rxavail - returns true if received bytes available.

 RR20090816  _rr002   Add compatable calls from Kye's SerialEngine for testing by Dr_Acula
 RR20090817  _rr003   Add mode bit 4 (add LF after CR), remove some calls
 RR20090818  _rr004   Remove mode bit 4 (add LF after CR) as per Dr_Acula

Adapted by BogdanM for STMBrain: added hardware flow control, moved pin declaration to compile time 
}

CON

  bufsiz = 256 '16              'buffer size (16, 32, 64, 128, 256, 512) must be factor of 2, max is 512
  bufmsk = bufsiz - 1           'buffer mask used for wrap-around ($00F, $01F, $03F, $07F, $0FF, $1FF)
  TX_PIN = 6              
  TX_PIN_MASK = 1 << TX_PIN    
  RX_PIN  = 7
  RX_PIN_MASK = 1 << RX_PIN
  CTS_PIN = 8
  CTS_PIN_MASK = 1 << CTS_PIN
  RTS_PIN = 9
  RTS_PIN_MASK = 1 << RTS_PIN
                
VAR

  long  cog                     'cog flag/id

  long  rx_head                 '6 contiguous longs
  long  rx_tail
  long  tx_head
  long  tx_tail
  long  bit_ticks
  long  buffer_ptr
                     
  byte  rx_buffer[bufsiz]           'transmit and receive buffers
  byte  tx_buffer[bufsiz]


PUB start(baudrate) : okay

'' Start serial driver - starts a cog
'' returns false if no cog available

  stop
  longfill(@rx_head, 0, 4)
  bit_ticks := clkfreq / baudrate
  buffer_ptr := @rx_buffer
  okay := cog := cognew(@entry, @rx_head) + 1


PUB stop

'' Stop serial driver - frees a cog

  if cog
    cogstop(cog~ - 1)
  longfill(@rx_head, 0, 6)

PUB rxflush

'' Flush receive buffer

  repeat while rxcheck => 0
  
PUB rxavail : truefalse

'' Check if byte(s) available
'' returns true (-1) if bytes available

  truefalse := rx_tail <> rx_head



PUB rxcheck : rxbyte

'' Check if byte received (never waits)
'' returns -1 if no byte received, $00..$FF if byte

  rxbyte--
  if rx_tail <> rx_head
    rxbyte := rx_buffer[rx_tail]
    rx_tail := (rx_tail + 1) & bufmsk


PUB rxtime(ms) : rxbyte | t

'' Wait ms milliseconds for a byte to be received
'' returns -1 if no byte received, $00..$FF if byte

  t := cnt
  repeat until (rxbyte := rxcheck) => 0 or (cnt - t) / (clkfreq / 1000) > ms
  

PUB rx : rxbyte

'' Receive byte (may wait for byte)
'' returns $00..$FF

  repeat while (rxbyte := rxcheck) < 0


PUB tx(txbyte)

'' Send byte (may wait for room in buffer)

  repeat until (tx_tail <> (tx_head + 1) & bufmsk)
  tx_buffer[tx_head] := txbyte
  tx_head := (tx_head + 1) & bufmsk


DAT

'***********************************
'* Assembly language serial driver *
'***********************************

                        org     0
'
'
' Entry
'
entry                   mov     t1,par                'get structure address
                        add     t1,#4 << 2            'skip past heads and tails

                        mov     rxmask, #1
                        shl     rxmask,#RX_PIN
                        mov     txmask, #1
                        shl     txmask,#TX_PIN
                        mov     rtsmask, #1
                        shl     rtsmask,#RTS_PIN
                        mov     ctsmask, #1
                        shl     ctsmask,#CTS_PIN

                        rdlong  bitticks,t1
                        add     t1,#4                 'get buffer_ptr
                        rdlong  rxbuff,t1
                        mov     txbuff,rxbuff
                        add     txbuff,#bufsiz

                        or      outa,txmask
                        mov     t1, txmask
                        or      t1, rtsmask
                        or      dira, t1

                        mov     txcode,#transmit      'initialize ping-pong multitasking
'
'
' Receive
'
receive                 jmpret  rxcode,txcode         'run a chunk of transmit code, then return

                        test    rxmask,ina      wz
        if_nz           jmp     #receive

                        mov     rxbits,#9             'ready to receive byte
                        mov     rxcnt,bitticks
                        shr     rxcnt,#1
                        add     rxcnt,cnt                          

:bit                    add     rxcnt,bitticks        'ready next bit period

:wait                   jmpret  rxcode,txcode         'run a chuck of transmit code, then return

                        mov     t1,rxcnt              'check if bit receive period done
                        sub     t1,cnt
                        cmps    t1,#0           wc
        if_nc           jmp     #:wait

                        test    rxmask,ina      wc    'receive bit on rx pin
                        rcr     rxdata,#1
                        djnz    rxbits,#:bit

                        shr     rxdata,#32-9          'justify and trim received byte
                        and     rxdata,#$FF

                        'mov     t1,par
                        'add     t1,#1 << 2
                        'rdlong  t3,t1                 't3 = rx_tail
                        rdlong  t2,par                'save received byte and inc head
                        add     t2,rxbuff
                        wrbyte  rxdata,t2
                        sub     t2,rxbuff
                        add     t2,#1
                        and     t2,#bufmsk
                        wrlong  t2,par
                        'cmp     t2,t3           wz
                        'muxz    rtsmask, outa         'if head==tail assert RTS
                        jmp     #receive              'byte done, receive next byte
'
'
' Transmit
'
transmit                jmpret  txcode,rxcode         'run a chunk of receive code, then return
                        test    ctsmask, ina    wz    'check CTS; if high, don't transmit anything
        if_nz           jmp     #transmit
                        mov     t1,par                'check for head <> tail
                        add     t1,#2 << 2
                        rdlong  t2,t1                 't2: head
                        add     t1,#1 << 2
                        rdlong  t3,t1                 't3: tail
                        cmp     t2,t3           wz
        if_z            jmp     #transmit

                        add     t3,txbuff             'get byte and inc tail
                        rdbyte  txdata,t3
                        sub     t3,txbuff
                        add     t3,#1
                        and     t3,#bufmsk
                        wrlong  t3,t1

                        or      txdata,#$100          'ready byte to transmit
                        shl     txdata,#2
                        or      txdata,#1
                        mov     txbits,#11
                        mov     txcnt,cnt

:bit                    shr     txdata,#1       wc
                        muxc    outa,txmask        
                        add     txcnt,bitticks        'ready next cnt

:wait                   jmpret  txcode,rxcode         'run a chunk of receive code, then return

                        mov     t1,txcnt              'check if bit transmit period done
                        sub     t1,cnt
                        cmps    t1,#0           wc
        if_nc           jmp     #:wait

                        djnz    txbits,#:bit          'another bit to transmit?

                        jmp     #transmit             'byte done, transmit next byte
'
'
' Uninitialized data
'
t1                      res     1
t2                      res     1
t3                      res     1

bitticks                res     1

rxmask                  res     1
rxbuff                  res     1
rxdata                  res     1
rxbits                  res     1
rxcnt                   res     1
rxcode                  res     1

txmask                  res     1
txbuff                  res     1
txdata                  res     1
txbits                  res     1
txcnt                   res     1
txcode                  res     1

rtsmask                 res     1
ctsmask                 res     1

{{          

+------------------------------------------------------------------------------------------------------------------------------+
¦                                                   TERMS OF USE: MIT License                                                  ¦                                                            
+------------------------------------------------------------------------------------------------------------------------------¦
¦Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation    ¦ 
¦files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,    ¦
¦modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software¦
¦is furnished to do so, subject to the following conditions:                                                                   ¦
¦                                                                                                                              ¦
¦The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.¦
¦                                                                                                                              ¦
¦THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE          ¦
¦WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR         ¦
¦COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,   ¦
¦ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                         ¦
+------------------------------------------------------------------------------------------------------------------------------+
}}