#!/usr/bin/env lua
--[[
This script generates the cos/sin lookup table
--]]
io.stdout:write([[
#include <stdint.h>
#include <portalib/complex.h>

/* table of cos/sin pairs for x=0..1023:
 * cos(x*pi*2/1024) * 127
 * sin(x*pi*2/1024) * 127
 */
static const int8_t cos_sin_tbl[] = {
]])

for phase = 0, 1023 do
    local s_precise = math.sin(phase*math.pi*2/1024)
    local c_precise = math.cos(phase*math.pi*2/1024)
    local s_dumb = math.floor(s_precise * 127 + .5)
    local c_dumb = math.floor(c_precise * 127 + .5)
    local s_err = s_precise*127 - s_dumb
    local c_err = c_precise*127 - c_dumb

    if phase > 0 then
        io.stdout:write(",\n")
    end
    io.stdout:write(string.format(
        "%d,\t%d\t/* %+0.3f, %+0.3f */",
        c_dumb, s_dumb, c_err, s_err
    ))
end
io.stdout:write([[

};

const complex_s8_t *cos_sin = (complex_s8_t*) cos_sin_tbl;
]])
