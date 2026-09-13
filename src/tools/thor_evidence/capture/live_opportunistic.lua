-- AUTO67 developer-only sampler; burst mode trades coverage for responsiveness.

local status_path = os.getenv("OASIS_LIVE_STATUS")
local final_path = os.getenv("OASIS_LIVE_FINAL")
local stop_path = os.getenv("OASIS_LIVE_STOP")
local max_frames = tonumber(os.getenv("OASIS_LIVE_MAX_FRAMES") or "0") or 0
local capacity = tonumber(os.getenv("OASIS_LIVE_WINDOW") or "256") or 256
local disabled = os.getenv("OASIS_LIVE_CAPTURE_DISABLED") == "1"
local demo_inputs = os.getenv("OASIS_LIVE_DEMO_INPUTS") or ""
local frame = 0
local sequence = 0
local callbacks = 0
local observed = 0
local overwritten = 0
local capture_ticks = 0
local capture_seconds = 0
local capture_mode = os.getenv("OASIS_LIVE_CAPTURE_MODE") or "continuous"
assert(capture_mode == "continuous" or capture_mode == "burst", "invalid capture mode")
local default_stride = capture_mode == "burst" and 1 or 16
local write_stride = math.max(1, tonumber(os.getenv("OASIS_LIVE_WRITE_STRIDE")) or default_stride)
local burst_period = math.max(1, tonumber(os.getenv("OASIS_LIVE_BURST_PERIOD")) or 30)
local burst_budget = math.max(1, tonumber(os.getenv("OASIS_LIVE_BURST_BUDGET")) or 64)
local burst_calls = 0
local burst_count = 0
local write_hook = nil
local ring = {}
local ring_start = 1
local ring_count = 0

local function json_string(value)
    local text = tostring(value or "")
    text = text:gsub("\\", "\\\\"):gsub('"', '\\"'):gsub("\n", "\\n")
    return '"' .. text .. '"'
end

local function hex(value)
    return string.format("0x%06X", tonumber(value or 0) & 0xFFFFFF)
end

local function read_pc()
    local ok, value = pcall(emu.getregister, "M68K PC")
    return ok and tonumber(value or 0) or 0
end

local function append_event(kind, pc, address)
    if disabled then return end
    local event = {seq = sequence, frame = frame, kind = kind, pc = pc, address = address}
    sequence = sequence + 1
    observed = observed + 1
    if ring_count < capacity then
        ring_count = ring_count + 1
        ring[((ring_start + ring_count - 2) % capacity) + 1] = event
    else
        ring[ring_start] = event
        ring_start = (ring_start % capacity) + 1
        overwritten = overwritten + 1
    end
end

local function event_json(event)
    return '{"seq":' .. event.seq .. ',"frame":' .. event.frame ..
        ',"kind":' .. json_string(event.kind) .. ',"pc":' .. json_string(hex(event.pc)) ..
        ',"address":' .. (event.address and json_string(hex(event.address)) or "null") .. '}'
end

local function input_table()
    local result = {}
    for item in demo_inputs:gmatch("([^,]+)") do
        local frame_text, buttons = item:match("^(%d+):(.+)$")
        if frame_text and buttons then
            local values = {}
            for button in buttons:gmatch("[^+]+") do values["P1 " .. button] = true end
            result[tonumber(frame_text)] = values
        end
    end
    return result
end

local inputs = input_table()

local function events_json()
    local values = {}
    for index = 0, ring_count - 1 do
        local event = ring[((ring_start + index - 1) % capacity) + 1]
        values[#values + 1] = event_json(event)
    end
    return "[" .. table.concat(values, ",") .. "]"
end

local function write_status(final)
    if disabled and not final then return end
    local path = final and final_path or status_path
    local file = io.open(path, "w")
    if not file then return end
    file:write('{"schema":"oasis.m12.auto67.live-capture.v1","frame":' .. frame .. ',"epoch":1' ..
        ',"events_observed":' .. observed .. ',"events_overwritten":' .. overwritten ..
        ',"callback_count":' .. callbacks .. ',"capture_enabled":' .. tostring(not disabled) ..
        ',"sampling_policy":' .. json_string(capture_mode) ..
        ',"causal_chain_complete":false,"burst_period_frames":' .. burst_period ..
        ',"burst_callback_budget":' .. burst_budget .. ',"burst_count":' .. burst_count ..
        ',"write_stride":' .. write_stride ..
        ',"rolling_window_capacity":' .. capacity .. ',"rolling_window_utilization":' .. ring_count ..
        ',"capture_seconds":' .. string.format("%.9f", capture_seconds) ..
        ',"capture_timing_scope":"POST_FRAME_ONLY_EXCLUDES_BUS_CALLBACKS"' ..
        ',"capture_samples":' .. capture_ticks .. ',"events":' .. events_json() .. '}')
    file:close()
end

local function capture_work(start)
    if disabled then return end
    capture_ticks = capture_ticks + 1
    capture_seconds = capture_seconds + (os.clock() - start)
end

local function remove_write_hook()
    if write_hook then
        assert(event.unregisterbyid(write_hook), "AUTO67 could not remove write hook")
        write_hook = nil
    end
end

local function on_write(address)
    callbacks = callbacks + 1
    burst_calls = burst_calls + 1
    if burst_calls % write_stride == 0 then
        append_event("RAM_WRITE_SAMPLE", read_pc(), address)
    end
    if capture_mode == "burst" and burst_calls >= burst_budget then remove_write_hook() end
end

while max_frames == 0 or frame < max_frames do
    if stop_path then
        local stop_file = io.open(stop_path, "r")
        if stop_file then stop_file:close(); break end
    end
    local arm = capture_mode == "continuous" and not write_hook
        or capture_mode == "burst" and frame % burst_period == 0
    if not disabled and arm then
        burst_calls = 0
        burst_count = burst_count + 1
        write_hook = event.on_bus_write(on_write, nil, "AUTO67 live writes", "M68K BUS")
        assert(write_hook and write_hook ~= "00000000-0000-0000-0000-000000000000",
            "AUTO67 write callbacks unavailable")
    end
    local buttons = inputs[frame]
    if buttons then joypad.set(buttons, 1) end
    emu.frameadvance()
    if capture_mode == "burst" then remove_write_hook() end
    local start = os.clock()
    frame = frame + 1
    if not disabled then append_event("FRAME_PC", read_pc(), nil) end
    if frame % 15 == 0 then write_status(false) end
    capture_work(start)
end

remove_write_hook()
write_status(true)
client.exitCode(0)
