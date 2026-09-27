local frame_count = tonumber(os.getenv("THOR_ROM_PROPERTIES_FRAMES")) or 120
local chunk_capacity = 4096
local output_path = os.getenv("THOR_ROM_PROPERTIES_HEX") or
  "build/runtime-rom-properties/live-map.hex"
local log_path = os.getenv("THOR_ROM_PROPERTIES_LOG") or
  "build/runtime-rom-properties/capture.log"

local log = assert(io.open(log_path, "wb"))
local function note(value)
  log:write(tostring(value), "\n")
  log:flush()
end

local ok, failure = pcall(function()
  note("started")
  note("clear=" .. tostring(genesis.rom_properties_clear()))
  note("enable=" .. tostring(genesis.rom_properties_enable(true)))
  for frame = 1, frame_count do
    emu.frameadvance()
    if frame % 30 == 0 then
      note(string.format("frame=%d changed_bytes=%d change_operations=%d",
        frame, genesis.rom_properties_changed_bytes(),
        genesis.rom_properties_change_operations()))
    end
  end
  note("capturing")
  local rom_size = genesis.rom_properties_size()
  note("rom_size=" .. tostring(rom_size))
  if rom_size <= 0 then error("ROM property map has no loaded ROM") end
  local output = assert(io.open(output_path, "wb"))
  for offset = 0, rom_size - 1, chunk_capacity do
    local count = math.min(chunk_capacity, rom_size - offset)
    local chunk = genesis.rom_properties_copy_hex(offset, count)
    if #chunk ~= count * 4 then
      output:close()
      error("property map chunk copy failed at " .. offset)
    end
    output:write(chunk, "\n")
  end
  output:close()
  note(string.format("ROM_PROPERTIES_CAPTURE frames=%d rom_size=%d changed_bytes=%d change_operations=%d output=%s",
    frame_count, rom_size, genesis.rom_properties_changed_bytes(),
    genesis.rom_properties_change_operations(), output_path))
end)

if not ok then note("ERROR=" .. tostring(failure)) end
log:close()
client.exit()
if not ok then error(failure) end
