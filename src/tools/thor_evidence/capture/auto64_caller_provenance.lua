-- Developer-only AUTO64 caller/A6 provenance probe. It never writes guest memory.
local request = assert(dofile(assert(os.getenv("TEE_AUTO64_REQUEST"))))
local raw = assert(io.open(assert(os.getenv("TEE_AUTO64_RAW")), "w"))
local log = assert(io.open(assert(os.getenv("BH_TEST_LOG")), "w"))
local rom_sha = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
local state_sha = "7fde47833ce70a1df34e75d95c84ed87afc8470d228af87967c6bd9dd38b3970"
local seq, phase, exec_count = 0, "inactive", 0
local previous_pc, previous_opcode = nil, nil
local previous_a6, last_a6_change = nil, nil
local ring, ring_limit = {}, 64
local max_events, event_count = 2048, 0

local function json(value)
    if type(value) == "string" then
        return '"' .. value:gsub('\\', '\\\\'):gsub('"', '\\"'):gsub('\n', '\\n') .. '"'
    end
    if type(value) == "boolean" or type(value) == "number" then return tostring(value) end
    if type(value) == "table" then
        local keys, parts = {}, {}
        for key in pairs(value) do keys[#keys + 1] = key end
        table.sort(keys)
        for _, key in ipairs(keys) do parts[#parts + 1] = json(tostring(key)) .. ":" .. json(value[key]) end
        return "{" .. table.concat(parts, ",") .. "}"
    end
    return "null"
end

local function reg(name)
    return emu.getregister("M68K " .. name)
end

local function snapshot()
    return {PC = reg("PC"), SR = reg("SR"), A0 = reg("A0"), A6 = reg("A6"), A7 = reg("A7"),
        D3 = reg("D3"), D4 = reg("D4"), D6 = reg("D6")}
end

local function stack_bytes(address, count)
    local values = {}
    if not address then return values end
    local ok, bytes = pcall(memory.read_bytes_as_array, address, count, "M68K BUS")
    if not ok or not bytes then return values end
    for index = 1, #bytes do values[#values + 1] = bytes[index] end
    return values
end

local function stack_return(address)
    if not address then return nil end
    local bytes = stack_bytes(address, 4)
    if #bytes < 4 then return nil end
    return ((bytes[1] & 0xFF) << 24) | ((bytes[2] & 0xFF) << 16) |
        ((bytes[3] & 0xFF) << 8) | (bytes[4] & 0xFF)
end

local function emit(kind, data)
    if event_count >= max_events then return end
    event_count = event_count + 1
    data.callback_pc = reg("PC")
    data.previous_pc = previous_pc
    data.previous_opcode = previous_opcode
    data.previous_a6 = previous_a6
    data.a6_definition_candidate = last_a6_change
    data.exec_count = exec_count
    raw:write(json({kind = kind, data = data, epoch = 1, seq = seq,
        frame = emu.framecount(), actor = "M68K", phase = phase}), "\n")
    seq = seq + 1
end

local function push_ring(address, opcode)
    ring[#ring + 1] = {pc = address, opcode = opcode, exec_count = exec_count,
        frame = emu.framecount()}
    if #ring > ring_limit then table.remove(ring, 1) end
end

local function on_exec(address, opcode, flags)
    exec_count = exec_count + 1
    local current_a6 = reg("A6")
    if previous_a6 ~= nil and current_a6 ~= previous_a6 then
        last_a6_change = {instruction_pc = address, old_a6 = previous_a6, new_a6 = current_a6,
            exec_count = exec_count, frame = emu.framecount()}
    end
    previous_a6 = current_a6
    if phase == "collect" and request.watched_exec[address] then
        local registers = snapshot()
        if not last_a6_change or exec_count - last_a6_change.exec_count > 256 or
            last_a6_change.frame ~= emu.framecount() then last_a6_change = nil end
        emit("TARGET_ENTRY_CONTEXT", {instruction_pc = address, opcode = opcode, flags = flags,
            registers = registers, stack_return_long = stack_return(registers.A7),
            stack_bytes = stack_bytes(registers.A7, 16), previous_pc_ring = ring})
    end
    push_ring(address, opcode)
    previous_pc, previous_opcode = address, opcode
end

event.on_bus_exec_any(on_exec, "M12-AUTO64-EXEC", "M68K BUS")

local function run()
    assert(emu.getsystemid() == "GEN", "Genesis not ready")
    raw:write(json({kind = "RAW_HEADER", schema = "oasis.m68k.m12-auto64-provenance.raw.v1",
        capture_id = assert(os.getenv("TEE_CAPTURE_ID")), rom_sha256 = rom_sha,
        state_sha256 = state_sha, request_sha256 = assert(os.getenv("TEE_AUTO64_REQUEST_SHA256")),
        scenario = request.runtime_if_needed.scenario, requested_frames = 20}), "\n")
    assert(savestate.load(assert(os.getenv("BH_TEST_STATE")), true), "state load failed")
    assert(emu.framecount() == 2117, "restore frame drift")
    emit("STATE_LOADED", {frame = 2117, state_sha256 = state_sha})
    for _ = 1, 3 do joypad.set({}); emu.frameadvance() end
    assert(emu.framecount() == 2120, "settle frame drift")
    phase = "collect"
    for frame = 1, 20 do
        joypad.set(frame <= 10 and {Right = true} or {})
        emu.frameadvance()
    end
    phase = "finished"
    emit("SUMMARY", {end_frame = emu.framecount(), exec_count = exec_count, ring_size = #ring})
    raw:write(json({kind = "RAW_END", schema = "oasis.m68k.m12-auto64-provenance.raw.v1",
        complete = true, events = seq}), "\n")
    log:write("result=PASS\n")
end

local ok, err = xpcall(run, debug.traceback)
if not ok then log:write("error=", tostring(err), "\nresult=FAIL\n") end
raw:close()
log:close()
client.exitCode(ok and 0 or 1)
