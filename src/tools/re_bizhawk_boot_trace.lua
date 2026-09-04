-- Developer-only BizHawk 2.11.1 boot_initial probe.
-- It emits the neutral text format consumed by oasis_re_emulator_trace.

local output_path = os.getenv("OASIS_BIZHAWK_TRACE_OUTPUT") or "bizhawk-trace.txt"
local instruction_limit = tonumber(os.getenv("OASIS_BIZHAWK_INSTRUCTION_LIMIT") or "512")
local output = assert(io.open(output_path, "w"))
local sequence = 0
local instructions = 0
local writes = 0

local function hex(value)
    return string.format("0x%08X", value & 0xFFFFFFFF)
end

local function register(name)
    local ok, value = pcall(emu.getregister, name)
    if ok and value ~= nil then return value end
    return nil
end

local function line(value)
    output:write(value, "\n")
end

local function snapshot()
    local values = {}
    for index = 0, 7 do
        local value = register("M68K D" .. index)
        if value then values[#values + 1] = string.format("d%d=%s", index, hex(value)) end
    end
    for index = 0, 6 do
        local value = register("M68K A" .. index)
        if value then values[#values + 1] = string.format("a%d=%s", index, hex(value)) end
    end
    local stack = register("M68K A7") or register("M68K SP")
    if stack then values[#values + 1] = "a7=" .. hex(stack) end
    local status = register("M68K SR")
    if status then values[#values + 1] = "sr=" .. hex(status) end
    return table.concat(values, " ")
end

local function emit(kind, pc, extra)
    local value = string.format("event seq=%d pc=%s kind=%s", sequence, hex(pc), kind)
    if extra then value = value .. " " .. extra end
    line(value)
    sequence = sequence + 1
end

line("oasis.m68k.external-trace.v1")
line("emulator=bizhawk")
line("backend=bizhawk-lua-bus")
line("version=" .. client.getversion())
line("scenario=boot_initial")
line("stop_condition=instruction_limit:" .. instruction_limit)
line("limit=" .. instruction_limit)

event.on_bus_write(function(address, value, flags)
    if writes < 128 and instructions < instruction_limit then
        local pc = register("M68K PC") or 0
        emit("write", pc, string.format("address=%s width=2", hex(address)))
        writes = writes + 1
    end
end)

event.on_bus_exec_any(function(address, value, flags)
    if instructions < instruction_limit then
        emit("instruction", address, snapshot())
        instructions = instructions + 1
    end
end)

while instructions < instruction_limit do
    emu.frameadvance()
end

output:close()
client.exitCode(0)
