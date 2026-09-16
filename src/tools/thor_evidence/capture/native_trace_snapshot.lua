-- Called only when the existing discovery path emits a concrete occurrence.
local M = {}

local function register_value(name)
    local ok, value = pcall(emu.getregister, name)
    if not ok then return nil end
    return tonumber(value)
end

function M.capture(epoch, sequence)
    local registers = {A4 = register_value("A4"), A5 = register_value("A5")}
    local ok, snapshot = pcall(genesis.freeze_native_trace_snapshot, epoch, sequence)
    if not ok or type(snapshot) ~= "string" then
        return nil, nil, tostring(snapshot or "native snapshot API returned no data")
    end
    local values = {}
    for _, name in ipairs({"A4", "A5"}) do
        values[#values + 1] = '"' .. name .. '":' ..
            (registers[name] and string.format("%.0f", registers[name]) or "null")
    end
    return snapshot, "{" .. table.concat(values, ",") .. "}", nil
end

return M
