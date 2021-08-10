local ffi = require("ffi")
ffi.cdef[[
void set_entity_position(uint32_t e_id, float x, float y, float z);
]]

function set_position_impl(entity, x, y, z)
    ffi.C.set_entity_position(entity, x, y, z)
end

return {
    set_position = set_position_impl
}
