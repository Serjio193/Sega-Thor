-- AUTO67 developer-only live sampler.  The callback path only updates a fixed ring.

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
local write_stride = tonumber(os.getenv("OASIS_LIVE_WRITE_STRIDE") or "16") or 16
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
        ',"rolling_window_capacity":' .. capacity .. ',"rolling_window_utilization":' .. ring_count ..
        ',"capture_seconds":' .. string.format("%.9f", capture_seconds) ..
        ',"capture_samples":' .. capture_ticks .. ',"events":' .. events_json() .. '}')
    file:close()
end

local function capture_work(start)
    if disabled then return end
    capture_ticks = capture_ticks + 1
    capture_seconds = capture_seconds + (os.clock() - start)
end

if not disabled then
    event.on_bus_write(function(address)
        callbacks = callbacks + 1
        if callbacks % write_stride == 0 then
            append_event("RAM_WRITE_SAMPLE", read_pc(), address)
        end
    end, "AUTO67 live writes", "M68K BUS")
end

event.onframeend(function()
    local start = os.clock()
    frame = frame + 1
    if not disabled then append_event("FRAME_PC", read_pc(), nil) end
    if frame % 15 == 0 then write_status(false) end
    capture_work(start)
end)

while max_frames == 0 or frame < max_frames do
    if stop_path and io.open(stop_path, "r") then break end
    local buttons = inputs[frame]
    if buttons then joypad.set(buttons, 1) end
    emu.frameadvance()
end

write_status(true)
local file = io.open(final_path, "w")
if file then
    file:write('{"schema":"oasis.m12.auto67.live-capture.v1","frame":' .. frame .. ',"epoch":1' ..
        ',"events_observed":' .. observed .. ',"events_overwritten":' .. overwritten ..
        ',"callback_count":' .. callbacks .. ',"capture_enabled":' .. tostring(not disabled) ..
        ',"rolling_window_capacity":' .. capacity .. ',"rolling_window_utilization":' .. ring_count ..
        ',"capture_seconds":' .. string.format("%.9f", capture_seconds) ..
        ',"capture_samples":' .. capture_ticks .. ',"events":' .. events_json() .. '}')
    file:close()
end
client.exitCode(0)
