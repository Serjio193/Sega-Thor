-- V0 observation only. Fixed state, bounded watches, no guest writes/inference.
local output = assert(io.open(assert(os.getenv("BH_TEST_LOG")), "w"))
local raw = assert(io.open(assert(os.getenv("TEE_RAW")), "w"))
local mode = os.getenv("TEE_MODE") or "probe"
local reverse = os.getenv("TEE_REVERSE") == "1"
local state_sha = "7fde47833ce70a1df34e75d95c84ed87afc8470d228af87967c6bd9dd38b3970"
local seq, epoch, phase, peeking = 0, 0, "inactive", false
local hits, writes, polls, reentries = 0, 0, 0, 0
local active, installed = false, false
local hook_ids = {}

local function json(v)
    if type(v) == "string" then
        return '"' .. v:gsub('\\', '\\\\'):gsub('"', '\\"'):gsub('\n', '\\n'):gsub('\r', '\\r') .. '"'
    end
    if type(v) == "boolean" or type(v) == "number" then return tostring(v) end
    if type(v) == "table" then
        local keys, parts = {}, {}
        for k in pairs(v) do keys[#keys + 1] = k end
        table.sort(keys)
        for _, k in ipairs(keys) do parts[#parts + 1] = json(k) .. ':' .. json(v[k]) end
        return '{' .. table.concat(parts, ',') .. '}'
    end
    return "null"
end

local function emit(kind, data)
    assert(seq < 10000, "V0 event budget exceeded")
    local record = {kind = kind, data = data, epoch = epoch, seq = seq,
        frame = emu.framecount(), actor = "M68K", phase = "RAW"}
    raw:write(json(record), "\n")
    seq = seq + 1
end

local function reg(name) return emu.getregister("M68K " .. name) end
local function peek(address, width)
    peeking = true
    local bytes = memory.read_bytes_as_array(address, width, "M68K BUS")
    peeking = false
    local parts = {}
    for _, v in ipairs(bytes) do parts[#parts + 1] = string.format("%02X", v) end
    return table.concat(parts)
end

local function snapshot()
    return {PC = reg("PC"), D2 = reg("D2"), D5 = reg("D5"), A0 = reg("A0"),
        A5 = reg("A5"), SR = reg("SR")}
end

local function callback(kind, label)
    return function(address, value, flags)
        if not active then return end
        if kind == "EXEC" and address == 0xA372 and phase == "test" then hits = hits + 1 end
        if kind == "WRITE" and address == 0xFF13CC and label == "w0" and phase == "test" then
            writes = writes + 1
        end
        local data = {hook = label, address = address, value = value, flags = flags,
            pc = reg("PC"), window = phase, during_peek = peeking,
            right = joypad.getwithmovie()["P1 Right"] == true}
        if peeking then
            reentries = reentries + 1
        elseif mode == "probe" then
            data.registers = snapshot()
            data.peek_hex = peek(address, 4)
        end
        emit(kind, data)
        -- NO return value: newer APIs may interpret a value as a bus override.
    end
end

local function install()
    if installed or mode == "uninstrumented" then return end
    local specs = {{"EXEC", 0xA372, "exec-A372"}, {"WRITE", 0xFF13CC, "w0"}}
    if mode == "probe" then
        for _, pc in ipairs({0xA36C, 0xA36E, 0xA370, 0xA374, 0xA376, 0xA378,
                0xA37A, 0xA37C, 0x1FCA, 0x1FD0, 0xA42A, 0xA430, 0xA436, 0xA358, 0xA35E}) do
            specs[#specs + 1] = {"EXEC", pc, string.format("exec-%X", pc)}
        end
        for i = 1, 7 do specs[#specs + 1] = {"WRITE", 0xFF13CC + i, "sat+" .. i} end
        for i = 0, 3 do specs[#specs + 1] = {"WRITE", 0xFF188A + i, "field+" .. i} end
        specs[#specs + 1] = {"WRITE", 0xFF13CC, "duplicate-start"}
        for _, addr in ipairs({0xA438, 0xA439, 0xA43A, 0xA43C, 0xFF188A, 0xFF188C, 0xFF1858}) do
            specs[#specs + 1] = {"READ", addr, string.format("read-%X", addr)}
        end
    end
    if reverse then
        local reordered = {}
        for i = #specs, 1, -1 do reordered[#reordered + 1] = specs[i] end
        specs = reordered
    end
    for _, spec in ipairs(specs) do
        local api = ({EXEC = event.on_bus_exec, WRITE = event.on_bus_write, READ = event.on_bus_read})[spec[1]]
        local id = api(callback(spec[1], spec[3]), spec[2], "TEE-V0-" .. spec[3], "M68K BUS")
        assert(id and id ~= "" and id ~= "00000000-0000-0000-0000-000000000000", "hook unavailable")
        hook_ids[#hook_ids + 1] = id
    end
    event.oninputpoll(function()
        if active then
            if phase == "test" then polls = polls + 1 end
            emit("INPUT", {poll = true, window = phase})
        end
    end, "TEE-V0-poll")
    installed = true
end

local function run()
    assert(mode == "probe" or mode == "minimal" or mode == "uninstrumented", "bad mode")
    assert(emu.getsystemid() == "GEN", "Genesis not ready")
    for iteration = 1, 2 do
        active = false
        assert(savestate.load(assert(os.getenv("BH_TEST_STATE")), true), "state load failed")
        assert(emu.framecount() == 2117, "restore frame drift")
        epoch, phase = iteration, "settle"
        hits, writes, polls = 0, 0, 0
        emit("EPOCH_BEGIN", {state_sha256 = state_sha, hooks_already_installed = installed})
        install()
        local neutral = {}
        for key, value in pairs(joypad.get()) do
            if type(value) == "boolean" then neutral[key] = false end
        end
        emit("SNAPSHOT", {point = "loaded", sat = peek(0xFF13CC, 8), registers = snapshot()})
        active = true
        for _ = 1, 3 do joypad.set(neutral); emu.frameadvance() end
        assert(emu.framecount() == 2120, "settle frame drift")
        emit("SNAPSHOT", {point = "before", sat = peek(0xFF13CC, 8), registers = snapshot()})
        if mode == "probe" then
            local saved_seq = seq
            local a = peek(0xFF188A, 4)
            local b = peek(0xA438, 4)
            emit("PEEK", {ram = a, rom = b, callback_events = seq - saved_seq})
        end
        phase = "test"
        joypad.set(neutral)
        joypad.set({ Right = true }, 1)
        emit("INPUT", {set_right = true, window = phase})
        emu.frameadvance()
        active, phase = false, "finished"
        joypad.set(neutral)
        assert(emu.framecount() == 2121, "end frame drift")
        local sat = peek(0xFF13CC, 8)
        assert(sat:sub(1, 8) == "00880901", "controlled SAT oracle changed")
        if mode ~= "uninstrumented" then assert(hits == 6 and writes == 1, "hook oracle changed") end
        emit("ORACLE", {sat = sat, fields = peek(0xFF188A, 4), registers = snapshot(),
            lagged = emu.islagged(), exec_hits = hits, write_hits = writes, input_polls = polls})
        emit("EPOCH_END", {reason = "COMPLETE", reentries = reentries})
    end
    raw:write(json({kind = "RAW_END", complete = true, events = seq}), "\n")
    output:write("result=PASS\n")
end
local ok, err = xpcall(run, debug.traceback)
if not ok then output:write("error=", tostring(err), "\nresult=FAIL\n") end
raw:close()
output:close()
client.exitCode(ok and 0 or 1)
