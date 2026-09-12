-- Developer-only BizHawk capability probe for M12.
-- It observes one frame and never writes emulator or ROM state.

local output_path = os.getenv("OASIS_BIZHAWK_CAPABILITY_OUTPUT") or
    "m12-bizhawk-capabilities.json"
local output = assert(io.open(output_path, "w"))
local observed = { frame_callback = 0, exec_callback = 0, write_callback = 0 }

local function json(value)
    value = tostring(value)
    return '"' .. value:gsub('\\', '\\\\'):gsub('"', '\\"') .. '"'
end

local function probe(name, callback)
    local ok, value = pcall(callback)
    return { name = name, present = ok, detail = ok and tostring(value or "ok") or tostring(value) }
end

local function register(name)
    local ok, value = pcall(emu.getregister, name)
    return ok and value or nil
end

local function has_function(root, name)
    local ok, value = pcall(function() return root[name] end)
    return ok and type(value) == "function"
end

local api = {
    probe("emu.getregister", function() return register("M68K PC") end),
    probe("memory.read_bytes_as_array.M68K_BUS", function()
        local bytes = memory.read_bytes_as_array(0, 2, "M68K BUS")
        return #bytes
    end),
    probe("memory.read_bytes_as_array.RAM", function()
        local bytes = memory.read_bytes_as_array(0xFF0000, 2, "M68K BUS")
        return #bytes
    end),
    probe("memory.getmemorydomainlist", function() return has_function(memory, "getmemorydomainlist") end),
    probe("memory.write_bytes_api_only", function() return has_function(memory, "write_bytes") end),
    probe("event.on_bus_exec_any", function() return has_function(event, "on_bus_exec_any") end),
    probe("event.on_bus_exec", function() return has_function(event, "on_bus_exec") end),
    probe("event.on_bus_read", function() return has_function(event, "on_bus_read") end),
    probe("event.on_bus_write", function() return has_function(event, "on_bus_write") end),
    probe("event.onframeend", function() return has_function(event, "onframeend") end),
    probe("emu.frameadvance", function() return has_function(emu, "frameadvance") end),
    probe("savestate.save_api_only", function() return has_function(savestate, "save") end),
    probe("savestate.load_api_only", function() return has_function(savestate, "load") end),
    probe("joypad.set", function() return has_function(joypad, "set") end),
}

local function register_probe(name)
    local value = register(name)
    return value and string.format("0x%08X", value & 0xFFFFFFFF) or "unavailable"
end

local domains = {}
local domain_ok, domain_values = pcall(memory.getmemorydomainlist)
if domain_ok and domain_values then
    for _, name in ipairs(domain_values) do domains[#domains + 1] = tostring(name) end
end

if has_function(event, "onframeend") then
    event.onframeend(function() observed.frame_callback = observed.frame_callback + 1 end)
end
if has_function(event, "on_bus_exec_any") then
    event.on_bus_exec_any(function() observed.exec_callback = observed.exec_callback + 1 end)
end
if has_function(event, "on_bus_write") then
    event.on_bus_write(function() observed.write_callback = observed.write_callback + 1 end)
end
if has_function(emu, "frameadvance") then emu.frameadvance() end

local function array(values)
    local parts = {}
    for _, value in ipairs(values) do parts[#parts + 1] = json(value) end
    return "[" .. table.concat(parts, ",") .. "]"
end

local function bool(value) return value and "true" or "false" end
output:write('{"schema":"oasis.m68k.m12-bizhawk-capabilities.v1",')
output:write('"emulator":"bizhawk","version":', json(client.getversion()), ',')
output:write('"registers":{"pc":', json(register_probe("M68K PC")),
    ',"sr":', json(register_probe("M68K SR")),
    ',"d0":', json(register_probe("M68K D0")),
    ',"a7":', json(register_probe("M68K A7")), '},')
output:write('"memory_domains":', array(domains), ',"api":[')
local api_values = {}
for _, item in ipairs(api) do
    api_values[#api_values + 1] = '{"name":' .. json(item.name) ..
        ',"present":' .. bool(item.present) .. ',"detail":' .. json(item.detail) .. '}'
end
output:write(table.concat(api_values, ","), '],"observed_callbacks":{"frame":',
    tostring(observed.frame_callback), ',"exec":', tostring(observed.exec_callback),
    ',"write":', tostring(observed.write_callback),
    '},"state_writes_emitted":false,"notes":',
    json("DMA/VDP semantic events require correlation from bus writes and domains"), '}')
output:close()
client.exitCode(0)
