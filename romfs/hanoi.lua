-- Towers of Hanoi
local term = term

-------------------------------------------------------------------------------
-- Data structures

local schar = string.char( 65 )
local gxoff, lxoff = 4, 3
local gyoff = 28
local tower_height = 16
local topline = 10

-------------------------------------------------------------------------------
-- Disk class

local disk = {}
disk.__index = disk

disk.new = function( size, x, y )
  local self = {}
  self.size = size
  self.x = x
  self.y = y
  setmetatable( self, disk )
  return self
end

disk.draw = function( self, clear )  
  local c = clear and ' ' or schar
  local ddata = string.rep( c, self.size * 2 + 2 )
  term.print( self.x, self.y, ddata )
end

disk.move = function( self, dx, dy )
  self:draw( true )
  self.x = self.x + dx
  self.y = self.y + dy
  self:draw()
end

-------------------------------------------------------------------------------
-- Towers class


local tower = {}
tower.__index = tower

tower.new = function( id, ndisks )
  local self = {}
  self.id = id
  self.disks = {}
  local topsize = ndisks > 0 and 1 or 0
  self.x = gxoff + lxoff + 24 * id
  local y = gyoff - ( ndisks - 1 )
  for i = topsize, topsize + ndisks - 1 do
    self.disks[ #self.disks + 1 ] = disk.new( i, self.x + 8 - i, y )
    y = y + 1
  end
  setmetatable( self, tower )
  return self
end

tower.draw = function( self )
  -- Draw center line
  for y = gyoff - #self.disks, gyoff - tower_height - 1, -1 do
    term.print( self.x + 8, y, schar )
    term.print( self.x + 9, y, schar )
  end
  for i = 1, #self.disks do
    self.disks[ i ]:draw()
  end
end

tower._topsize = function( self )
  return #self.disks == 0 and 0 or self.disks[ 1 ].size
end

tower._numdisks = function( self )
  return #self.disks
end

tower.canmove = function( self, d )
  return #self.disks == 0 and 0 or d.size < self:_topsize()
end

tower.add_disk = function( self, d )
  if self:canmove( d ) then
    for i = #self.disks, 1, -1 do
      self.disks[ i + 1 ] = self.disks[ i ]
    end
    self.disks[ 1 ] = d
  end
end

tower.remove_disk = function( self )
  if self.ndisks == 0 then return nil end
  local d = self.disks[ 1 ]
  for i = 2, #self.disks do
    self.disks[ i - 1 ] = self.disks[ i ]
  end
  self.disks[ #self.disks ] = nil
  return d
end

tower.diskpos = function( self, d )
  local x = self.x + 8 - d.size
  local y = gyoff - ( #self.disks == 0 and 0 or #self.disks )
  return x, y
end

-------------------------------------------------------------------------------

local function totower( from, to )
  -- Step 1: go up
  local d = from:remove_disk()
  local y = d.y
  while y >= topline do
    d:move( 0, -1 )
    from:draw()
    tmr.delay( 1, 50000 )
    y = y - 1
  end
  to:add_disk( d )
  -- Step 1: move left/right
  local dir = to.id > from.id and 1 or -1
  local nx, ny = to:diskpos( d )
  local x = d.x
  while x ~= nx do
    d:move( dir, 0 )
    d:draw()
    tmr.delay( 1, 50000 )
    x = x + dir
  end
  -- Step 3: go down
  local y = d.y
  repeat
    d:move( 0, 1 )
    to:draw()
    tmr.delay( 1, 50000 )
    y = y + 1
  until y > ny
end

local towers = {}

for i = 1, 3 do
  towers[ i ] = tower.new( i - 1, i == 1 and 4 or 0 )
end

term.clrscr()
for i = 1, 3 do
  towers[ i ]:draw()
end  

local function canmove( t1, t2 )
  if t1:_numdisks() == 0 then return false end
  local d = t1.disks[ 1 ]
  return t2:canmove( d )
end

local A, B, C = towers[ 1 ], towers[ 2 ], towers[ 3 ]
while C:_numdisks() ~= 4 do
  -- A to B
  if canmove( A, B ) then totower( A, B ) else totower( B, A ) end
  -- A to C
  if canmove( A, C ) then totower( A, C ) else totower( C, A ) end
  -- B to C
  if canmove( B, C ) then totower( B, C ) else totower( C, B ) end
end

term.getchar()
term.clrscr()
term.gotoxy( 1, 1 )

