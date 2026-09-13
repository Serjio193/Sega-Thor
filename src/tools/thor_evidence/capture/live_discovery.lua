-- Developer-only bounded live discovery; no guest writes or ownership claims.
local plan = assert(dofile(assert(os.getenv("TEE_WATCH_PLAN"))))
local raw = assert(io.open(assert(os.getenv("TEE_RAW")), "w"))
local log = assert(io.open(assert(os.getenv("BH_TEST_LOG")), "w"))
local state_sha = "7fde47833ce70a1df34e75d95c84ed87afc8470d228af87967c6bd9dd38b3970"
local rom_sha = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
local seq, phase, done = 0, "inactive", false
local seen_exec, seen_read, seen_write = {}, {}, {}
local novel_exec, novel_read, novel_write = 0, 0, 0
local known_exec, known_read, known_write = 0, 0, 0
local max_each = 512

local function json(value)
    if type(value) == "string" then
        return '"' .. value:gsub('\\', '\\\\'):gsub('"', '\\"'):gsub('\n', '\\n') .. '"'
    end
    if type(value) == "boolean" or type(value) == "number" then return tostring(value) end
    if type(value) == "table" then
        local keys, parts = {}, {}
        for key in pairs(value) do keys[#keys + 1] = key end
        table.sort(keys)
        for _, key in ipairs(keys) do parts[#parts + 1] = json(tostring(key)) .. ':' .. json(value[key]) end
        return '{' .. table.concat(parts, ',') .. '}'
    end
    return "null"
end

local function reg(name) return emu.getregister("M68K " .. name) end
local function snapshot()
    return {PC = reg("PC"), D0 = reg("D0"), D2 = reg("D2"), D5 = reg("D5"),
        A0 = reg("A0"), A1 = reg("A1"), A5 = reg("A5"), SR = reg("SR")}
end

local function emit(kind, data)
    local event = {kind = kind, data = data, epoch = 1, seq = seq,
        frame = emu.framecount(), actor = "M68K", phase = phase}
    raw:write(json(event), "\n")
    seq = seq + 1
end

local function is_unknown_rom_page(address)
    return address >= 0 and address < 0x300000 and plan.unknown_rom_pages[math.floor(address / 0x1000)]
end

local function on_exec(address, opcode, flags)
    if phase ~= "collect" then return end
    if plan.known_exec[address] then known_exec = known_exec + 1; return end
    if seen_exec[address] or novel_exec >= max_each then return end
    seen_exec[address] = true
    novel_exec = novel_exec + 1
    emit("EXEC_NOVELTY", {address = address, opcode = opcode, flags = flags,
        registers = snapshot(), rom_page_unknown = is_unknown_rom_page(address),
        known_by_plan = false})
end

local function on_read(address, value, flags)
    if phase ~= "collect" or not is_unknown_rom_page(address) then return end
    if plan.known_read[address] then known_read = known_read + 1; return end
    if seen_read[address] or novel_read >= max_each then return end
    seen_read[address] = true
    novel_read = novel_read + 1
    emit("ROM_READ_NOVELTY", {address = address, value = value, flags = flags,
        registers = snapshot(), known_by_plan = false})
end

local function on_write(address, value, flags)
    if phase ~= "collect" or address < 0xFF0000 or address > 0xFFFFFF then return end
    if plan.known_write[address] then known_write = known_write + 1; return end
    if seen_write[address] or novel_write >= max_each then return end
    seen_write[address] = true
    novel_write = novel_write + 1
    emit("RAM_WRITE_NOVELTY", {address = address, value = value, flags = flags,
        registers = snapshot(), known_by_plan = false})
end

event.on_bus_exec_any(on_exec, "M12-AUTO62-EXEC", "M68K BUS")
event.on_bus_read(on_read, nil, "M12-AUTO62-ROM-READ", "M68K BUS")
event.on_bus_write(on_write, nil, "M12-AUTO62-RAM-WRITE", "M68K BUS")

local function apply_input(frame)
    joypad.set({}, 1)
    if frame <= 30 then joypad.set({Right = true}, 1)
    elseif frame >= 61 and frame <= 90 then joypad.set({Left = true}, 1) end
end

local function run()
    assert(emu.getsystemid() == "GEN", "Genesis not ready")
    raw:write(json({kind = "RAW_HEADER", schema = "oasis.m68k.m12-live-discovery.raw.v1",
        capture_id = assert(os.getenv("TEE_CAPTURE_ID")), rom_sha256 = rom_sha,
        state_sha256 = state_sha, watch_plan_sha256 = assert(os.getenv("TEE_WATCH_PLAN_SHA256")),
        scenario = "quicksave1-directional-window-v1", requested_frames = 120}), "\n")
    assert(savestate.load(assert(os.getenv("BH_TEST_STATE")), true), "state load failed")
    assert(emu.framecount() == 2117, "restore frame drift")
    emit("STATE_LOADED", {frame = 2117, state_sha256 = state_sha})
    for _ = 1, 3 do joypad.set({}); emu.frameadvance() end
    assert(emu.framecount() == 2120, "settle frame drift")
    phase = "collect"
    for frame = 1, 120 do
        apply_input(frame)
        emu.frameadvance()
    end
    phase, done = "finished", true
    emit("SUMMARY", {known_exec = known_exec, known_read = known_read, known_write = known_write,
        novel_exec = novel_exec, novel_read = novel_read, novel_write = novel_write,
        end_frame = emu.framecount()})
    raw:write(json({kind = "RAW_END", schema = "oasis.m68k.m12-live-discovery.raw.v1",
        complete = true, events = seq}), "\n")
    log:write("result=PASS\n")
end

local ok, err = xpcall(run, debug.traceback)
if not ok then log:write("error=", tostring(err), "\nresult=FAIL\n") end
raw:close()
log:close()
client.exitCode(ok and 0 or 1)
