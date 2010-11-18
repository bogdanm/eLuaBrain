from PIL import Image
import sys

cheight = 12
sects = cheight / 4

def outdigit( x ):
  if x < 10:
    sys.stdout.write( chr( x + ord( "0" ) ) )
  else:
    sys.stdout.write( chr( x - 10 + ord( "a" ) ) )

def outhex( x ):
  outdigit( ( x & 0xF0 ) >> 4 )
  outdigit( x & 0x0F )

im = Image.open( "fontdata.png" )
chars = [ 0 ] * 256 * cheight
for ch in xrange( 0, 256 ):
  line = ( ch / 16 ) * cheight
  col = ( ch % 16 ) * 8
  for i in xrange( 0, cheight ):
    ldata = 0
    mask = 1
    for j in xrange( 0, 8 ):
      p = im.getpixel( ( col + j, line + i ) )
      if p != 0:
        ldata = ldata | mask
      mask = mask << 1
    chars[ ch * cheight + i ] = ldata

for section in xrange( 0, sects ):
  longcnt = 0
  sys.stdout.write( "  long ");
  for ch in xrange( 0, 256 ):
    start = ch * cheight + section * 4
    sys.stdout.write( "$" )    
    outhex( chars[ start + 3 ])
    outhex( chars[ start + 2 ])  
    outhex( chars[ start + 1 ])  
    outhex( chars[ start ] )
    longcnt = longcnt + 1
    if longcnt == 8:
      longcnt = 0      
      print
      if start != 255 * cheight + section * 4:    
        sys.stdout.write( "  long ");      
    else:
      sys.stdout.write( "," )              
  print  
