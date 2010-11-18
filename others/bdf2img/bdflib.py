# BDF library

# This represents a char from BDF with all its attributes
class BDFChar:
	def __init__(self):
		self.code = 0
		self.width = 0
		self.height = 0
		self.numbits = 0
		self.xoff = 0
		self.yoff = 0
		self.bitmap = []
		self.vbitmap = []
		
# Matching functions for parsing
def _m_str(arg):
	if len(arg) < 2:
		return (False, arg)
	if arg[0] != '"' or arg[-1] != '"':
		return (False, arg)
	else:
		return (True, arg)
def _m_int(arg):
	base = 10
	if arg.startswith('0x') or arg.startswith('0X'):
		base = 16
	try:
		num = int(arg, base)
		return (True, num)
	except:
		return (False, arg)
def _m_eat_rest(arg):
	return (True, arg)		
	
class BDF:
	# Constants for parseline
	PL_PARSE_ERROR = -1
	PL_IO_ERROR = 0
	PL_EOF = 1
	PL_STARTFONT = 2
	PL_COMMENT = 3
	PL_FONT = 4
	PL_SIZE = 5
	PL_FONTBOUNDINGBOX = 6
	PL_STARTPROPERTIES = 7
	PL_PIXELSIZE = 8
	PL_POINTSIZE = 9
	PL_RESOLUTIONX = 10
	PL_RESOLUTIONY = 11
	PL_CHARSETREGISTRY = 12
	PL_CHARSETENCODING = 13
	PL_DEFAULTCHAR = 14
	PL_FONTASCENT = 15
	PL_FONTDESCENT = 16
	PL_ENDPROPERTIES = 17
	PL_CHARS = 18
	PL_STARTCHAR = 19
	PL_ENCODING = 20
	PL_SWIDTH = 21
	PL_DWIDTH = 22
	PL_BBX = 23
	PL_BITMAP = 24
	PL_ENDCHAR = 25
	PL_HEXNUM = 26
	PL_ENDFONT = 27
	PL_SEM_ERROR = 28
	
	def __init__(self, fname):
		self.fname = fname
		self.infile = None
		self.lineno = 0
		self.chars = []
		self.maxw = 0
		self.maxh = 0
		
	def open(self):
		try:
			self.infile = open(self.fname, "rt")
		except:
			return False
		return True
		
	def __del__(self):
		self.infile.close()
	
	# Description of parsing rules for each command above
	parse_rules =  {
		"STARTFONT": (PL_STARTFONT, [_m_eat_rest]),
		"COMMENT": (PL_COMMENT, [_m_eat_rest]),
		"FONT": (PL_FONT, [_m_eat_rest]),
		"SIZE": (PL_SIZE, [_m_int, _m_int, _m_int]),
		"FONTBOUNDINGBOX": (PL_FONTBOUNDINGBOX, [_m_int, _m_int, _m_int, _m_int]),
		"STARTPROPERTIES": (PL_STARTPROPERTIES, [_m_int]),
		"PIXEL_SIZE": (PL_PIXELSIZE, [_m_int]),	
		"POINT_SIZE": (PL_POINTSIZE, [_m_int]),
		"RESOLUTION_X": (PL_RESOLUTIONX, [_m_int]),
		"RESOLUTION_Y": (PL_RESOLUTIONY, [_m_int]),
		"CHARSET_REGISTRY": (PL_CHARSETREGISTRY, [_m_str]),
		"CHARSET_ENCODING": (PL_CHARSETENCODING, [_m_str]),
		"DEFAULT_CHAR": (PL_DEFAULTCHAR, [_m_int]),
		"FONT_DESCENT": (PL_FONTDESCENT, [_m_int]),
		"FONT_ASCENT": (PL_FONTASCENT, [_m_int]),	
		"ENDPROPERTIES": (PL_ENDPROPERTIES, []),	
		"CHARS": (PL_CHARS, [_m_int]),	
		"STARTCHAR": (PL_STARTCHAR, [_m_int]),	
		"ENCODING": (PL_ENCODING, [_m_int]),	
		"SWIDTH": (PL_SWIDTH, [_m_int, _m_int]),	
		"DWIDTH": (PL_DWIDTH, [_m_int, _m_int]),	
		"BBX": (PL_BBX, [_m_int, _m_int, _m_int, _m_int]),	
		"BITMAP": (PL_BITMAP, []),	
		"ENDCHAR": (PL_ENDCHAR, []),	
		"ENDFONT": (PL_ENDFONT, [])		
	}
	
	# Parse a single line from the BDF file
	def parseline(self):
		# Read data from file
		self.lineno = self.lineno + 1		
		try:
			data = self.infile.readline()
		except:
			return (self.PL_IO_ERROR, self.lineno, 'io error', [])
		if len(data) == 0:
			return (self.PL_EOF, self.lineno , 'eof', [])
		result = []
		data = data.rstrip("\r\n")
		# Split line
		temp = data.split(" ")
		tokens = [el for el in temp if len(el) > 0]
		if self.parse_rules.has_key(tokens[0]):
			# We found a key
			index = 1
			ruleno, funcs = self.parse_rules[tokens[0]]
			rtable = []
			for func in funcs:
				# Now validate the whole line
				if func == _m_eat_rest:
					return (ruleno, self.lineno, tokens[0], tokens[1:])
				else:
					parsed, res = apply(func, [tokens[index]])
					if not parsed:
						return (self.PL_PARSE_ERROR, self.lineno, 'parse error', [])
					rtable = rtable + [res]
				index = index + 1
			# If we still have arguments left, fail
			if index != len(tokens):
				return (self.PL_PARSE_ERROR, self.lineno, 'parse error', [])
			else:
				return (ruleno, self.lineno, tokens[0], rtable)
		else:
			# No key found, might as well be a hex number
			if len(tokens) != 1:
				return (self.PL_PARSE_ERROR, self.lineno, 'parse error', [])
			nrform = "0x" + tokens[0]
			try:
				nr = int(nrform, 16)
			except:
				return (self.PL_PARSE_ERROR, self.lineno, 'parse error', [])
			return (self.PL_HEXNUM, self.lineno, "hex", [nr])
			
	def accept(self, code, maxskip=-1):
		count = 0
		while True:
			retcode, line, tname, results = self.parseline()
			if retcode == code:
				break
			if retcode == self.PL_EOF or retcode == self.PL_PARSE_ERROR or retcode == self.PL_IO_ERROR:
				break
			count = count + 1
			if maxskip != -1 and count == maxskip:
				return self.PL_SEM_ERROR, self.lineno, 'semantic error', []
		return retcode, line, tname, results
		
	def getchars(self):
		return self.chars
		
	def getcharcnt(self):
		return len(self.chars)
		
	def getchar(self, code):
		for el in self.chars:
			if el.code == code:
				return el
		return None
		
	def getmaxw(self):
		return self.maxw
		
	def getmaxh(self):
		return self.maxh
		
	def _getvbitmap(self, ch):
		w = ch.width
		h = ch.height
		mask = 1 << (w - 1)
		for i in xrange(0, w):
			val = 0
			bit = 1 << (h - 1)
			for j in xrange(0, h):
				if ch.bitmap[j] & mask:
					val = val + bit
				bit = bit >> 1
			ch.vbitmap.append(val)
			mask = mask >> 1
		
	# Read all characters in font
	def readallchars(self):
		while True:
			retcode, line, tname, results = self.accept(self.PL_STARTCHAR)
			if retcode == self.PL_EOF:
				break
			if retcode in [self.PL_PARSE_ERROR, self.PL_IO_ERROR, self.PL_SEM_ERROR]:
				return (False, tname, line)
			ch = BDFChar()
			# Now we know the code
			ch.code = results[0]
			retcode, line, tname, results = self.accept(self.PL_BBX)
			if retcode in [self.PL_EOF, self.PL_PARSE_ERROR, self.PL_IO_ERROR, self.PL_SEM_ERROR]:
				return (False, tname, line)
			# Now we know the width and height
			w, h, xoff, yoff = results
			ch.width = w
			ch.height = h
			ch.xoff = xoff
			ch.yoff = yoff
			if w > 32:
				ch.numbits = 64
			elif w > 16:
				ch.numbits = 32
			elif w > 8:
				ch.numbits = 16
			else:
				ch.numbits = 8
			if w > self.maxw:
				self.maxw = w
			if h > self.maxh:
				self.maxh = h
			# All that's left is to read the bitmap
			retcode, line, tname, results = self.accept(self.PL_BITMAP)
			if retcode in [self.PL_EOF, self.PL_PARSE_ERROR, self.PL_IO_ERROR, self.PL_SEM_ERROR]:
				return (False, tname, line)			
			minzero = 1024
			dummyl = []
			for l in xrange(0, h):
				retcode, line, tname, results = self.accept(self.PL_HEXNUM, 1)
				if retcode in [self.PL_EOF, self.PL_PARSE_ERROR, self.PL_IO_ERROR, self.PL_SEM_ERROR]:
					return (False, tname, line)					
				dummyl.append(results[0])
				zeros = 0
				mask = 1
				while results[0] & mask == 0 and mask < (1 << ch.numbits):
					zeros = zeros + 1
					mask = mask << 1 
				if zeros < minzero:
					minzero = zeros
			if minzero == 1024:
				minzero = 0
			retcode, line, tname, results = self.accept(self.PL_ENDCHAR)
			if retcode in [self.PL_EOF, self.PL_PARSE_ERROR, self.PL_IO_ERROR, self.PL_SEM_ERROR]:
				return (False, tname, line)		
			ch.bitmap = [x >> minzero for x in dummyl]
			self._getvbitmap(ch)
			self.chars.append(ch)
		return (True, 'ok', self.chars)
