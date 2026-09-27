local frame_count = tonumber(os.getenv("THOR_ROM_PROPERTIES_FRAMES")) or 3600
local scenario_id = assert(os.getenv("THOR_ROM_PROPERTIES_SCENARIO_ID"),
  "scenario ID missing")
local output_path = assert(os.getenv("THOR_ROM_PROPERTIES_HEX"),
  "property hex output path missing")
local log_path = assert(os.getenv("THOR_ROM_PROPERTIES_LOG"),
  "scenario log path missing")
local log = assert(io.open(log_path, "wb"))

local function note(value)
  log:write(tostring(value), "\n")
  log:flush()
end

local function input_for(frame)
  if frame < 1200 then
    if frame % 120 == 0 then return { Start = true } end
    if frame % 60 == 0 then return { A = true } end
    if frame % 60 == 30 then return { C = true } end
    return {}
  end
  local phase = math.floor((frame - 1200) / 30) % 8
  if phase == 0 then return { Right = true } end
  if phase == 1 then return { Right = true, B = frame % 8 < 2 } end
  if phase == 2 then return { Down = true } end
  if phase == 3 then return { Left = true } end
  if phase == 4 then return { Left = true, B = frame % 8 < 2 } end
  if phase == 5 then return { Up = true } end
  if phase == 6 then return { Right = true, C = frame % 15 == 0 } end
  return { A = frame % 20 == 0 }
end

local ok, failure = pcall(function()
  note("scenario_id=" .. scenario_id)
  note("clear=" .. tostring(genesis.rom_properties_clear()))
  note("enable=" .. tostring(genesis.rom_properties_enable(true)))
  for frame = 1, frame_count do
    joypad.set(input_for(frame), 1)
    emu.frameadvance()
    if frame % 300 == 0 then
      note(string.format("frame=%d changed_bytes=%d change_operations=%d",
        frame, genesis.rom_properties_changed_bytes(),
        genesis.rom_properties_change_operations()))
    end
  end

  local rom_size = genesis.rom_properties_size()
  assert(rom_size > 0, "ROM property map has no loaded ROM")
  local output = assert(io.open(output_path, "wb"))
  for offset = 0, rom_size - 1, 4096 do
    local count = math.min(4096, rom_size - offset)
    local chunk = genesis.rom_properties_copy_hex(offset, count)
    assert(#chunk == count * 4, "property map copy failed at " .. offset)
    output:write(chunk, "\n")
  end
  output:close()
  note(string.format("capture=PASS frames=%d rom_size=%d changed_bytes=%d change_operations=%d",
    frame_count, rom_size, genesis.rom_properties_changed_bytes(),
    genesis.rom_properties_change_operations()))
end)

log:close()
client.exitCode(ok and 0 or 1)
client.exit()
if not ok then error(failure) end
