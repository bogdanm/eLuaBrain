# Script for converting a bdf file to a C source 

import os, sys
import getopt, math
from bdflib import BDF, BDFChar
from PIL import Image

chars = None
charno = 0
x = 0
y = 0

def printchar( font, code, im ):
  ch = font.getchar( code )
  if ch is None:
    print "ERROR: cannot get data for char %d" % code
    return

  # Generate char data
  for line in xrange( 0, 12 ):
    data = ch.bitmap[ line ]
    mask = 1 << ( 8 - 1 )
    bitno = 0
    while mask != 0:
      if data & mask:
        sys.stdout.write( 'X' )
        im.putpixel( ( x + bitno, y + line ), 1 )
      else:
        sys.stdout.write(' ')
      mask = mask >> 1
      bitno = bitno + 1
    print

################################################################################    
# The program starts here
  
font = BDF("cp437-8x12.bdf")
if not font.open():
  print "Cannot open %s" % args_proper[0]
  sys.exit(0)
res, a1, a2 = font.readallchars()
if not res:
  print "Error %s at line %d" % (a1, a2)
  sys.exit(0)  
    
chars = range( 0, 256 )
  
im = Image.new( "1", ( 128, 192 ), 0 )
  
# Process all chars
for ch in chars: 
  printchar( font, ch, im )    
  charno = charno + 1
  if charno == 16:
    y = y + 12
    x = 0
    charno = 0
  else:
    x = x + 8

im.save( "fontdata.png" )

