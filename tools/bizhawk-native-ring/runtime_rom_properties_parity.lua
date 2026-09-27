local output_path = assert(os.getenv("THOR_ROM_PROPERTIES_PARITY_OUTPUT"),
  "parity output path missing")
local frame_count = assert(tonumber(os.getenv("THOR_ROM_PROPERTIES_PARITY_FRAMES")),
  "parity frame count missing")
local scenario = os.getenv("THOR_ROM_PROPERTIES_PARITY_SCENARIO") or "neutral"
local out = assert(io.open(output_path, "wb"))
local marker_addresses = { 0xFF0001, 0xFF0002, 0xFF0003, 0xFF0012,
  0xFF0B44, 0xFF0B45, 0xFF0B60, 0xFF0B61 }
local z80_registers = { "pc", "sp", "af", "bc", "de", "hl", "ix", "iy" }

local function register(name)
  local ok, value = pcall(emu.getregister, name)
  if not ok then return "NA" end
  return string.format("%08x", value & 0xFFFFFFFF)
end

local function marker_bytes()
  local values = {}
  for _, address in ipairs(marker_addresses) do
    values[#values + 1] = string.format("%02x", memory.read_u8(address, "M68K BUS"))
  end
  return table.concat(values)
end

local function input_for(frame)
  if scenario == "neutral" then return {} end
  assert(scenario == "menu-start-directional-v1", "unknown parity input scenario")
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

local function write_row(frame)
  out:write(table.concat({ tostring(frame), register("M68K PC"),
    register("M68K SR"), register("Z80 pc") }, ","), "\n")
end

local function fnv1a(domain, address, count)
  local bytes = memory.read_bytes_as_array(address, count, domain)
  assert(#bytes == count, "memory-domain read failed: " .. domain)
  local hash = 2166136261
  for _, byte in ipairs(bytes) do hash = ((hash ~ byte) * 16777619) & 0xFFFFFFFF end
  return string.format("%08x", hash)
end

local ok, failure = pcall(function()
  write_row(0)
  for frame = 1, frame_count do
    joypad.set(input_for(frame), 1)
    emu.frameadvance()
    write_row(frame)
    if frame % 100 == 0 then out:flush() end
  end
  local final_registers = {}
  for _, name in ipairs({ "M68K D0", "M68K D1", "M68K D2", "M68K A0",
      "M68K A1", "M68K A7" }) do
    final_registers[#final_registers + 1] = name .. "=" .. register(name)
  end
  for _, name in ipairs(z80_registers) do
    final_registers[#final_registers + 1] = "Z80 " .. name .. "=" .. register("Z80 " .. name)
  end
  out:write("FINAL_REGISTERS,", table.concat(final_registers, ","),
    ",markers=" .. marker_bytes(), "\n")
  out:write("STATE_MARKERS,m68k_ram_fnv1a=",
    fnv1a("M68K BUS", 0xFF0000, 0x10000), ",z80_bus_0000_1fff_fnv1a=",
    fnv1a("Z80 BUS", 0, 0x2000), "\n")
end)

out:close()
client.exitCode(ok and 0 or 1)
client.exit()
if not ok then error(failure) end
