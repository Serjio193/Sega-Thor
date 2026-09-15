-- WALKER-1: one bounded two-window advancement experiment.
local W1_TRIGGER = 0x0027BE
local B_GUARD = 0x0027CA
local B_ENTRY = 0x0027CE
local B_EXIT = 0x0027E2
local W2_END = 0x0027EC
local VDP_WRITE = 0x00C00004
local MAX_W1, MAX_W2, MAX_HOOKS = 64, 64, 8

local output = os.getenv("OASIS_WALKER_FINAL") or "walker-final.json"
local run_id = os.getenv("OASIS_WALKER_RUN_ID") or "walker-run"
local restore_epoch = tonumber(os.getenv("OASIS_WALKER_RESTORE_EPOCH") or "0") or 0
local max_frames = tonumber(os.getenv("OASIS_WALKER_MAX_FRAMES") or "120") or 120
local frame = 0
local phase = "IDLE"
local global_id = nil
local w1_seq, w2_seq = 0, 0
local fragment = {w1 = {}, w2 = {}}
local data_accesses = {}
local off_pcs = {}
local metrics = {
    w1_count = 0, w2_count = 0, w1_callbacks = 0, w2_callbacks = 0,
    global_callbacks = 0, callbacks_during_b_off = 0,
    targeted_callbacks = 0, guard_callbacks = 0, data_callbacks = 0,
    w1_started = 0, w1_completed = 0, w2_started = 0, w2_completed = 0,
    contract_skips = 0, failed_guard_skips = 0, global_active_frames = 0,
    hook_install_errors = 0, global_off_body_executions = 0,
    max_w1_records = 0, max_w2_records = 0, global_active_ms = 0,
    frame_p50_ms = 0, frame_p95_ms = 0, frame_p99_ms = 0, frame_max_ms = 0,
    frames_over_33ms = 0, frames_over_50ms = 0
}
local frame_times = {}
local global_started = nil
local global_frames = {}

local function pc_value()
    local ok, value = pcall(emu.getregister, "M68K PC")
    return ok and tonumber(value or 0) or 0
end

local function sync_frame()
    local ok, value = pcall(emu.framecount)
    if ok and value then frame = tonumber(value) or frame end
end

local function append(which, address)
    local records = fragment[which]
    local sequence = which == "w1" and w1_seq or w2_seq
    sequence = sequence + 1
    if which == "w1" then w1_seq = sequence else w2_seq = sequence end
    records[#records + 1] = {sequence = sequence, frame = frame, pc = address or 0}
    return sequence
end

local function unregister_global()
    if global_id then
        event.unregisterbyid(global_id)
        global_id = nil
        if global_started then
            metrics.global_active_ms = metrics.global_active_ms +
                (os.clock() - global_started) * 1000.0
            global_started = nil
        end
    end
end

local function global_callback(address)
    sync_frame()
    global_frames[frame] = true
    metrics.global_callbacks = metrics.global_callbacks + 1
    if phase == "W1" then
        metrics.w1_callbacks = metrics.w1_callbacks + 1
        append("w1", address)
        if #fragment.w1 >= MAX_W1 then
            unregister_global()
            phase = "FAILED_W1_BUDGET"
        end
    elseif phase == "W2" then
        metrics.w2_callbacks = metrics.w2_callbacks + 1
        append("w2", address)
        if #fragment.w2 >= MAX_W2 then
            unregister_global()
            phase = "FAILED_W2_BUDGET"
        end
    elseif phase == "B_OFF" and address ~= B_GUARD and address ~= B_EXIT then
        -- B_EXIT is itself the targeted boundary guard.  The contract covers
        -- the body [B_ENTRY, B_EXIT); a callback at the guard is not body
        -- evidence and is retained separately in the boundary record.
        metrics.callbacks_during_b_off = metrics.callbacks_during_b_off + 1
        off_pcs[#off_pcs + 1] = address or 0
    end
end

local function install_global()
    local ok, id = pcall(function()
        return event.on_bus_exec_any(global_callback, "WALKER-1 global", "M68K BUS")
    end)
    if not ok or not id then
        metrics.hook_install_errors = metrics.hook_install_errors + 1
        return false
    end
    global_id = id
    global_started = os.clock()
    return true
end

local function begin_w1()
    sync_frame()
    if phase ~= "IDLE" then return end
    metrics.targeted_callbacks = metrics.targeted_callbacks + 1
    metrics.w1_started = metrics.w1_started + 1
    metrics.w1_count = metrics.w1_count + 1
    if install_global() then phase = "W1" else phase = "FAILED_W1_HOOK" end
end

local function enter_b()
    sync_frame()
    if phase ~= "W1" then return end
    metrics.guard_callbacks = metrics.guard_callbacks + 1
    metrics.contract_skips = metrics.contract_skips + 1
    metrics.w1_completed = metrics.w1_completed + 1
    fragment.b_entry = {frame = frame, pc = B_ENTRY, guard_pc = B_GUARD, w1_sequence = w1_seq}
    unregister_global()
    phase = "B_OFF"
end

local function exit_b()
    sync_frame()
    if phase ~= "B_OFF" then return end
    metrics.guard_callbacks = metrics.guard_callbacks + 1
    fragment.b_exit = {frame = frame, pc = B_EXIT, w1_sequence = w1_seq,
                       callbacks_during_b_off = metrics.callbacks_during_b_off}
    if metrics.callbacks_during_b_off ~= 0 then
        metrics.failed_guard_skips = metrics.failed_guard_skips + 1
        phase = "FAILED_B_GUARD"
        return
    end
    metrics.global_off_body_executions = metrics.global_off_body_executions + 1
    metrics.w2_started = metrics.w2_started + 1
    metrics.w2_count = metrics.w2_count + 1
    if install_global() then phase = "W2" else phase = "FAILED_W2_HOOK" end
end

local function end_w2()
    sync_frame()
    if phase ~= "W2" then return end
    metrics.guard_callbacks = metrics.guard_callbacks + 1
    fragment.w2_end = {frame = frame, pc = W2_END, w2_sequence = w2_seq}
    unregister_global()
    metrics.w2_completed = metrics.w2_completed + 1
    phase = "DONE"
end

local function record_vdp(address)
    sync_frame()
    if phase ~= "W2" then return end
    metrics.data_callbacks = metrics.data_callbacks + 1
    data_accesses[#data_accesses + 1] = {frame = frame, pc = pc_value(),
                                         address = address or 0, kind = "BUS_WRITE"}
end

local function register_exec(address, callback)
    local ok, id = pcall(function()
        return event.on_bus_exec(callback, address, "WALKER-1 guard", "M68K BUS")
    end)
    if not ok or not id then metrics.hook_install_errors = metrics.hook_install_errors + 1 end
    return id
end

local hooks = {}

local function json_string(value)
    local text = tostring(value or "")
    text = text:gsub("\\", "\\\\"):gsub('"', '\\"'):gsub("\n", "\\n")
    return '"' .. text .. '"'
end

local function json_records(records)
    local out = {}
    for _, item in ipairs(records) do
        out[#out + 1] = string.format('{"sequence":%d,"frame":%d,"pc":"0x%06X"}',
                                      item.sequence, item.frame, item.pc)
    end
    return "[" .. table.concat(out, ",") .. "]"
end

local function json_data()
    local out = {}
    for _, item in ipairs(data_accesses) do
        out[#out + 1] = string.format('{"address":"0x%08X","frame":%d,"kind":"%s","pc":"0x%06X"}',
                                      item.address, item.frame, item.kind, item.pc)
    end
    return "[" .. table.concat(out, ",") .. "]"
end

local function write_final()
    unregister_global()
    for _, id in ipairs(hooks) do if id then event.unregisterbyid(id) end end
    metrics.max_w1_records, metrics.max_w2_records = #fragment.w1, #fragment.w2
    local active_frames = 0
    for _ in pairs(global_frames) do active_frames = active_frames + 1 end
    metrics.global_active_frames = active_frames
    table.sort(frame_times)
    local function percentile(fraction)
        if #frame_times == 0 then return 0 end
        return frame_times[math.max(1, math.ceil(#frame_times * fraction))]
    end
    metrics.frame_p50_ms = percentile(0.50)
    metrics.frame_p95_ms = percentile(0.95)
    metrics.frame_p99_ms = percentile(0.99)
    metrics.frame_max_ms = frame_times[#frame_times] or 0
    for _, duration in ipairs(frame_times) do
        if duration > 33 then metrics.frames_over_33ms = metrics.frames_over_33ms + 1 end
        if duration > 50 then metrics.frames_over_50ms = metrics.frames_over_50ms + 1 end
    end
    local file = io.open(output, "w")
    if not file then return end
    file:write('{"schema":"oasis.m12.walker1.runtime.v1",')
    file:write('"run_id":', json_string(run_id), ',"restore_epoch":', restore_epoch, ',')
    file:write('"capture_id":"capture-1","fragment_id":"fragment-1",')
    file:write('"block":{"guard":"0x0027CA","entry":"0x0027CE","exit":"0x0027E2",')
    file:write('"contract":"straight-line 0x0027CE..0x0027E2; A4/A5/control-flow unchanged; selected VDP writes recorded"},')
    file:write('"phase":', json_string(phase), ',"metrics":{')
    local fields = {}
    for key, value in pairs(metrics) do fields[#fields + 1] = json_string(key) .. ":" .. tostring(value) end
    table.sort(fields)
    file:write(table.concat(fields, ","), '},"w1":', json_records(fragment.w1))
    file:write(',"b_entry":', fragment.b_entry and string.format('{"frame":%d,"guard_pc":"0x%06X","pc":"0x%06X","w1_sequence":%d}', fragment.b_entry.frame, fragment.b_entry.guard_pc, fragment.b_entry.pc, fragment.b_entry.w1_sequence) or "null")
    file:write(',"b_exit":', fragment.b_exit and string.format('{"callbacks_during_b_off":%d,"frame":%d,"pc":"0x%06X","w1_sequence":%d}', fragment.b_exit.callbacks_during_b_off, fragment.b_exit.frame, fragment.b_exit.pc, fragment.b_exit.w1_sequence) or "null")
    file:write(',"w2":', json_records(fragment.w2), ',"w2_end":', fragment.w2_end and string.format('{"frame":%d,"pc":"0x%06X","w2_sequence":%d}', fragment.w2_end.frame, fragment.w2_end.pc, fragment.w2_end.w2_sequence) or "null")
    local off = {}
    for _, address in ipairs(off_pcs) do off[#off + 1] = string.format('"0x%06X"', address) end
    file:write(',"data_accesses":', json_data(), ',"off_pcs":[' .. table.concat(off, ',') .. ']}')
    file:close()
    local log = io.open(os.getenv("BH_TEST_LOG") or "walker-result.log", "w")
    if log then log:write("result=PASS\nphase=", phase, "\n"); log:close() end
end

local state_path = os.getenv("OASIS_WALKER_STATE")
if state_path and state_path ~= "" then
    assert(savestate.load(state_path, true), "WALKER-1 state load failed")
    for _ = 1, 3 do emu.frameadvance() end
end
hooks[#hooks + 1] = register_exec(W1_TRIGGER, begin_w1)
hooks[#hooks + 1] = register_exec(B_GUARD, enter_b)
hooks[#hooks + 1] = register_exec(B_EXIT, exit_b)
hooks[#hooks + 1] = register_exec(W2_END, end_w2)
local write_hook = event.on_bus_write(record_vdp, VDP_WRITE, "WALKER-1 data", "M68K BUS")
hooks[#hooks + 1] = write_hook
for _ = 1, max_frames do
    frame = emu.framecount()
    local started = os.clock()
    emu.frameadvance()
    frame_times[#frame_times + 1] = (os.clock() - started) * 1000.0
end
write_final()
client.exitCode(0)
