local ffi = require("ffi")
ffi.cdef[[
void set_entity_position(uint32_t e_id, float x, float y, float z);
]]

local set_entity_position
ffi.C.set_entity_position(1, 1.0, 0.0, 1.0)

io.write("This was written from lua")
io.flush()
