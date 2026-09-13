-- AUTO67.1 benchmark control: advance BizHawk without RE callbacks or event capture.
local max_frames = tonumber(os.getenv("OASIS_LIVE_MAX_FRAMES") or "0") or 0
local output = os.getenv("OASIS_FRAME_CLOCK_OUTPUT")
local frame = 0

while max_frames == 0 or frame < max_frames do
    emu.frameadvance()
    frame = frame + 1
end

if output then
    local file = io.open(output, "w")
    if file then
        file:write('{"frame":' .. frame .. '}\n')
        file:close()
    end
end
client.exitCode(0)
