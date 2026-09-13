-- Developer-only dense interval capture for the selected FF13CC A372 instance.
-- Captures every EXEC callback from A370 through the A374 fetch boundary.
local raw = assert(io.open(assert(os.getenv("TEE_RAW")), "w"))
local log = assert(io.open(assert(os.getenv("BH_TEST_LOG")), "w"))
local schema = "thor.evidence.raw.v0.1"
local state_sha = "7fde47833ce70a1df34e75d95c84ed87afc8470d228af87967c6bd9dd38b3970"
local seq, epoch, frame = 0, 0, 0
local phase, active, target_seen, done = "inactive", false, false, false
local current_exec_seq = nil
local exec_count, write_count = 0, 0
local target = 0xFF13CC

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
    local result = {D = {}, A = {}, PC = reg("PC"), SR = reg("SR")}
    for index = 0, 7 do result.D[index + 1] = reg("D" .. index) end
    for index = 0, 7 do result.A[index + 1] = reg("A" .. index) end
    return result
end

local function emit(kind, data)
    local event = {kind = kind, data = data, epoch = epoch, seq = seq,
        frame = frame, actor = "M68K", phase = "RAW"}
    raw:write(json(event), "\n")
    seq = seq + 1
    return event.seq
end

raw:write(json({kind = "RAW_HEADER", schema = schema,
    receipt_sha256 = assert(os.getenv("TEE_RECEIPT_SHA256")),
    capture_id = assert(os.getenv("TEE_CAPTURE_ID")),
    rom_sha256 = assert(os.getenv("TEE_ROM_SHA256")), state_sha256 = state_sha,
    mode = "probe", reverse = false,
    watch_plan_sha256 = assert(os.getenv("TEE_WATCH_PLAN_SHA256"))}), "\n")

local function on_exec(address, opcode, flags)
    if not active then
        if phase == "test" and address == 0xA370 and reg("A5") == target then
            active = true
        else
            return
        end
    end
    local registers = snapshot()
    local event_seq = emit("EXEC", {address = address, pc = registers.PC,
        opcode = opcode, flags = flags, registers = registers,
        dense = true, boundary = address == 0xA374 and "POST_FETCH" or nil})
    current_exec_seq = event_seq
    exec_count = exec_count + 1
    if address == 0xA372 and registers.A[6] == target then target_seen = true end
    if target_seen and address == 0xA374 then
        active, done = false, true
    end
end

local function on_write(address, value, flags)
    if not active or current_exec_seq == nil then return end
    write_count = write_count + 1
    emit("WRITE", {address = address, value = value, flags = flags,
        pc = reg("PC"), exec_seq = current_exec_seq, dense = true})
end

event.on_bus_exec_any(on_exec, "THOR-V1-DENSE-EXEC", "M68K BUS")
event.on_bus_write(on_write, nil, "THOR-V1-DENSE-WRITE", "M68K BUS")

local function run_epoch(number)
    epoch, phase, active, target_seen, done = number, "settle", false, false, false
    current_exec_seq, exec_count, write_count = nil, 0, 0
    assert(savestate.load(assert(os.getenv("BH_TEST_STATE")), true), "state load failed")
    assert(emu.framecount() == 2117, "restore frame drift")
    emit("EPOCH_BEGIN", {state_sha256 = state_sha, dense = true})
    for _ = 1, 3 do emu.frameadvance() end
    assert(emu.framecount() == 2120, "settle frame drift")
    phase = "test"
    joypad.set({}, 1)
    joypad.set({Right = true}, 1)
    emu.frameadvance()
    phase = "finished"
    assert(done, "dense A372 interval not found")
    emit("EPOCH_END", {reason = "COMPLETE", dense = true,
        exec_count = exec_count, write_count = write_count})
end

local ok, err = xpcall(function()
    run_epoch(1)
    run_epoch(2)
    raw:write(json({kind = "RAW_END", schema = schema, complete = true,
        events = seq}), "\n")
    log:write("result=PASS\n")
end, debug.traceback)
if not ok then log:write("error=", tostring(err), "\nresult=FAIL\n") end
raw:close()
log:close()
client.exitCode(ok and 0 or 1)
