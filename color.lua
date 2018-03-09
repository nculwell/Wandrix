-- vim: nu et ts=2 sts=2 sw=2

local function combine(a,b) return { a[1]+b[1], a[2]+b[2], a[3]+b[3] } end

local LRED    = {255,0,0}
local RED     = {127,0,0}
local LGREEN  = {0,255,0}
local GREEN   = {0,127,0}
local LBLUE   = {0,0,255}
local BLUE    = {0,0,127}
local WHITE   = {255,255,255}
local BLACK   = {0,0,0}

local color = {
  LRED=LRED, RED=RED, LGREEN=LGREEN, GREEN=GREEN, LBLUE=LBLUE, BLUE=BLUE,
  WHITE=WHITE, BLACK=BLACK,
  LYELLOW = combine(LRED, LGREEN),
  YELLOW  = combine(RED, GREEN),
  ORANGE  = combine(LRED, GREEN),
}

return color

