-- Developer-only FF13CC canary capture. Arms at the first test A342 entry and
-- stops at its A436 return boundary; no guest state is written.
local raw = assert(io.open(assert(os.getenv("TEE_RAW")), "w"))
local log = assert(io.open(assert(os.getenv("BH_TEST_LOG")), "w"))
local schema = "thor.evidence.raw.v0.1"
local state_sha = "7fde47833ce70a1df34e75d95c84ed87afc8470d228af87967c6bd9dd38b3970"
local seq, epoch, frame = 0, 0, 0
local phase, active, target_count, done = "inactive", false, 0, false
local current_exec_seq = nil

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
    capture_id = assert(os.getenv("TEE_CAPTURE_ID")), rom_sha256 = assert(os.getenv("TEE_ROM_SHA256")),
    state_sha256 = state_sha, mode = "probe", reverse = false,
    watch_plan_sha256 = assert(os.getenv("TEE_WATCH_PLAN_SHA256"))}), "\n")

local function capture_exec(address, opcode, flags)
    if not active then
        if phase ~= "test" or address ~= 0xA342 then return end
        active = true
    end
    local registers = snapshot()
    local event_seq = emit("EXEC", {address = address, pc = address, callback_pc = registers.PC,
        opcode = opcode, flags = flags, registers = registers, dense = true})
    current_exec_seq = event_seq
    if address == 0xA372 then target_count = target_count + 1 end
    if address == 0xA436 and target_count >= 6 then active, done = false, true end
end

local function capture_bus(kind, address, value, flags)
    if not active or current_exec_seq == nil then return end
    emit(kind, {address = address, value = value, flags = flags,
        pc = reg("PC"), exec_seq = current_exec_seq, dense = true})
end

event.on_bus_exec_any(capture_exec, "THOR-V1-CANARY-EXEC", "M68K BUS")
event.on_bus_read(function(address, value, flags) capture_bus("READ", address, value, flags) end,
    nil, "THOR-V1-CANARY-READ", "M68K BUS")
event.on_bus_write(function(address, value, flags) capture_bus("WRITE", address, value, flags) end,
    nil, "THOR-V1-CANARY-WRITE", "M68K BUS")

local function run_epoch(number)
    epoch, phase, active, target_count, done = number, "settle", false, 0, false
    current_exec_seq = nil
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
    assert(done and target_count == 6, "A342/A372/A436 canary window incomplete")
    emit("EPOCH_END", {reason = "COMPLETE", dense = true, target_count = target_count})
end

local ok, err = xpcall(function()
    run_epoch(1)
    run_epoch(2)
    raw:write(json({kind = "RAW_END", schema = schema, complete = true, events = seq}), "\n")
    log:write("result=PASS\n")
end, debug.traceback)
if not ok then log:write("error=", tostring(err), "\nresult=FAIL\n") end
raw:close()
log:close()
client.exitCode(ok and 0 or 1)
