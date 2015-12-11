#!/usr/bin/env lua
--[[
This script generates the cos/sin lookup table
--]]
io.stdout:write([[
#include <stdint.h>
#include <portalib/complex.h>

/* table of sin/cos pairs for x=0..1023:
 * sin(x*pi*2/1024) * 127
 * cos(x*pi*2/1024) * 127
 */
static const int8_t sin_cos_tbl[] = {
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
        s_dumb, c_dumb, s_err, c_err
    ))
end
io.stdout:write([[

};

/* the complex_s8_t type stores the I value in the first
 * octet, the Q value in the second octet.
 * Since when thinking about complex numbers I tend to
 * start with the Q value myself (a + bi), the accessor
 * is named the other way around here to present the fact
 * that we have a cosine function for the Q value and
 * a sine function for the I value
 */
const complex_s8_t *cos_sin = (complex_s8_t*) sin_cos_tbl;
]])
