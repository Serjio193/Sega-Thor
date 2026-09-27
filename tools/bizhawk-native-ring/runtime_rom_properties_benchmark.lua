local mode = assert(os.getenv("THOR_ROM_PROPERTIES_MODE"), "benchmark mode missing")
local output_path = assert(os.getenv("THOR_ROM_PROPERTIES_BENCH_OUTPUT"),
  "benchmark output path missing")
local ram_dump_path = os.getenv("THOR_ROM_PROPERTIES_RAM_DUMP")
local frame_count = tonumber(os.getenv("THOR_ROM_PROPERTIES_BENCH_FRAMES")) or 120
local out = assert(io.open(output_path, "wb"))

local function finish(code, message)
  out:write(message, "\n")
  out:close()
  client.exitCode(code)
  client.exit()
end

local ok, result = pcall(function()
  if mode ~= "BASELINE" then
    assert(genesis.rom_properties_clear(), "property map clear failed")
    assert(genesis.rom_properties_enable(mode == "ON_COLD"),
      "property map enable failed")
  end
  local times = {}
  local started = os.clock()
  local cold_elapsed = 0
  for frame = 1, frame_count do
    joypad.set({}, 1)
    local before = os.clock()
    emu.frameadvance()
    times[#times + 1] = (os.clock() - before) * 1000
    if frame == 120 then cold_elapsed = os.clock() - started end
  end
  local elapsed = os.clock() - started
  table.sort(times)
  local bytes = memory.read_bytes_as_array(0xFF0000, 0x10000, "M68K BUS")
  assert(#bytes == 0x10000, "could not read full M68K RAM for parity hash")
  local hash = 2166136261
  for _, byte in ipairs(bytes) do hash = ((hash ~ byte) * 16777619) & 0xFFFFFFFF end
  if ram_dump_path then
    local dump = assert(io.open(ram_dump_path, "wb"))
    for _, byte in ipairs(bytes) do dump:write(string.format("%02x", byte)) end
    dump:close()
  end
  local properties, operations = 0, 0
  if mode ~= "BASELINE" then
    properties = genesis.rom_properties_changed_bytes()
    operations = genesis.rom_properties_change_operations()
  end
  local warm_frames = math.max(0, frame_count - 120)
  local warm_elapsed = math.max(0, elapsed - cold_elapsed)
  return string.format(
    "mode=%s frames=%d cpu_seconds=%.6f fps=%.3f cold120_fps=%.3f warm_fps=%.3f p95_ms=%.4f p99_ms=%.4f ram_fnv1a=%08x pc=%08x sr=%08x d0=%08x a7=%08x changed_bytes=%d change_operations=%d changes_per_second=%.1f map_storage_bytes=%d",
    mode, frame_count, elapsed, frame_count / elapsed,
    cold_elapsed > 0 and 120 / cold_elapsed or 0,
    warm_frames > 0 and warm_elapsed > 0 and warm_frames / warm_elapsed or 0,
    times[math.ceil(frame_count * 0.95)], times[math.ceil(frame_count * 0.99)],
    hash, emu.getregister("M68K PC") & 0xFFFFFFFF,
    emu.getregister("M68K SR") & 0xFFFFFFFF,
    emu.getregister("M68K D0") & 0xFFFFFFFF,
    emu.getregister("M68K A7") & 0xFFFFFFFF,
    properties, operations, elapsed > 0 and operations / elapsed or 0,
    mode == "BASELINE" and 0 or (genesis.rom_properties_size() * 2 +
      math.floor(math.ceil(genesis.rom_properties_size() / 4096) / 8)))
end)

if ok then finish(0, result) else finish(1, "ERROR=" .. tostring(result)) end
