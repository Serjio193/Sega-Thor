-- Developer-only narrow register/source capture; never writes guest memory.
local plan = assert(dofile(assert(os.getenv("TEE_FOCUS_PLAN"))))
local raw = assert(io.open(assert(os.getenv("TEE_FOCUS_RAW")), "w"))
local log = assert(io.open(assert(os.getenv("BH_TEST_LOG")), "w"))
local state_sha = "7fde47833ce70a1df34e75d95c84ed87afc8470d228af87967c6bd9dd38b3970"
local rom_sha = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
local seq, phase = 0, "inactive"
local last_exec_pc, last_exec_opcode, last_exec_seq = nil, nil, nil
local exec_counter, focus_exec_count, focus_read_count = 0, 0, 0
local max_events = 4096

local function json(value)
    if type(value) == "string" then
        return '"' .. value:gsub('\\', '\\\\'):gsub('"', '\\"'):gsub('\n', '\\n') .. '"'
    end
    if type(value) == "boolean" or type(value) == "number" then return tostring(value) end
    if type(value) == "table" then
        local keys, parts = {}, {}
        for key in pairs(value) do keys[#keys + 1] = key end
        table.sort(keys)
        for _, key in ipairs(keys) do
            parts[#parts + 1] = json(tostring(key)) .. ":" .. json(value[key])
        end
        return "{" .. table.concat(parts, ",") .. "}"
    end
    return "null"
end

local function reg(name)
    return emu.getregister("M68K " .. name)
end

local function snapshot()
    return {
        PC = reg("PC"), SR = reg("SR"),
        D0 = reg("D0"), D1 = reg("D1"), D2 = reg("D2"), D3 = reg("D3"),
        D4 = reg("D4"), D5 = reg("D5"), D6 = reg("D6"), D7 = reg("D7"),
        A0 = reg("A0"), A1 = reg("A1"), A2 = reg("A2"), A3 = reg("A3"),
        A4 = reg("A4"), A5 = reg("A5"), A6 = reg("A6"), A7 = reg("A7"),
    }
end

local function emit(kind, data)
    data.callback_pc = reg("PC")
    data.last_exec_pc = last_exec_pc
    data.last_exec_opcode = last_exec_opcode
    data.last_exec_seq = last_exec_seq
    data.exec_counter = exec_counter
    local event = {kind = kind, data = data, epoch = 1, seq = seq,
        frame = emu.framecount(), actor = "M68K", phase = phase}
    raw:write(json(event), "\n")
    seq = seq + 1
end

local function on_exec(address, opcode, flags)
    exec_counter = exec_counter + 1
    last_exec_pc, last_exec_opcode, last_exec_seq = address, opcode, seq
    if phase ~= "collect" or not plan.watched_exec[address] or focus_exec_count >= max_events then return end
    focus_exec_count = focus_exec_count + 1
    emit("EXEC_FOCUS", {instruction_pc = address, opcode = opcode, flags = flags,
        registers = snapshot()})
end

local function on_read(address, value, flags)
    if phase ~= "collect" or focus_read_count >= max_events then return end
    local registers = snapshot()
    local source_base = (registers.A6 or -1) + (plan.source_offset or 0)
    local kind = nil
    if address >= plan.rom_start and address < plan.rom_end then
        kind = "ROM_READ_FOCUS"
    elseif plan.source_register and (address == source_base or address == source_base + 2) then
        kind = "RAM_SOURCE_READ"
    end
    if not kind then return end
    focus_read_count = focus_read_count + 1
    emit(kind, {address = address, value = value, flags = flags,
        source_base = source_base, registers = registers})
end

event.on_bus_exec_any(on_exec, "M12-AUTO63-EXEC", "M68K BUS")
event.on_bus_read(on_read, nil, "M12-AUTO63-READ", "M68K BUS")

local function apply_input(frame)
    joypad.set({}, 1)
    if frame <= 10 then joypad.set({Right = true}, 1) end
end

local function run()
    assert(emu.getsystemid() == "GEN", "Genesis not ready")
    raw:write(json({kind = "RAW_HEADER", schema = "oasis.m68k.m12-focused-register.raw.v1",
        capture_id = assert(os.getenv("TEE_CAPTURE_ID")), rom_sha256 = rom_sha,
        state_sha256 = state_sha, plan_sha256 = assert(os.getenv("TEE_FOCUS_PLAN_SHA256")),
        scenario = "quicksave1-focused-register-slice-af02-v1",
        requested_frames = plan.requested_frames}), "\n")
    assert(savestate.load(assert(os.getenv("BH_TEST_STATE")), true), "state load failed")
    assert(emu.framecount() == 2117, "restore frame drift")
    emit("STATE_LOADED", {frame = 2117, state_sha256 = state_sha})
    for _ = 1, 3 do joypad.set({}); emu.frameadvance() end
    assert(emu.framecount() == 2120, "settle frame drift")
    phase = "collect"
    for frame = 1, plan.requested_frames do
        apply_input(frame)
        emu.frameadvance()
    end
    phase = "finished"
    emit("SUMMARY", {end_frame = emu.framecount(), exec_count = focus_exec_count,
        focus_read_count = focus_read_count})
    raw:write(json({kind = "RAW_END", schema = "oasis.m68k.m12-focused-register.raw.v1",
        complete = true, events = seq}), "\n")
    log:write("result=PASS\n")
end

local ok, err = xpcall(run, debug.traceback)
if not ok then log:write("error=", tostring(err), "\nresult=FAIL\n") end
raw:close()
log:close()
client.exitCode(ok and 0 or 1)
