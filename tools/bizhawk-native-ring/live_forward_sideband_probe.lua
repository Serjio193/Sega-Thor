local raw_path = assert(os.getenv("LF_OUTPUT"), "LF_OUTPUT missing")
local record_path = os.getenv("LF_PROBE_RECORDS") or
    assert(os.getenv("LF_RECORD_DIR"), "LF_RECORD_DIR missing") .. "\\probe-records.bin"
local log_path = assert(os.getenv("BH_TEST_LOG"), "BH_TEST_LOG missing")
local raw = assert(io.open(raw_path, "w"))
local log = assert(io.open(log_path, "w"))
local depth = 100
local memory = 1024 * 1024
local frame_count = 0

local function output(key, value)
    raw:write(key, "=", tostring(value), "\n")
    raw:flush()
end

local function fields(value)
    local result = {}
    for item in value:gmatch("[^,]+") do result[#result + 1] = item end
    return result
end

local function advance()
    emu.frameadvance()
    frame_count = frame_count + 1
end

local function run()
    if not genesis.live_forward_enable(false) then error("cannot disable recorder") end
    local plan = genesis.live_forward_memory_plan(1, depth, memory)
    if plan == "" then error("probe plan rejected") end
    if not genesis.live_forward_configure(1, depth, memory) then
        error("probe configure rejected")
    end
    for _ = 1, 10 do advance() end
    local frame_before = genesis.live_forward_latest_frame_boundary_record()
    local epoch = genesis.live_forward_epoch()
    if not genesis.live_forward_request(0, 1, 1, 1, epoch) then
        error("probe request rejected")
    end
    for _ = 1, 120 do
        if genesis.live_forward_status(0) == 3 then break end
        advance()
    end
    if genesis.live_forward_status(0) ~= 3 then error("probe capture timed out") end
    local meta = genesis.live_forward_result_info(0)
    local values = fields(meta)
    local count = assert(tonumber(values[17]), "probe record count missing")
    if not genesis.live_forward_export_records(record_path, 0, 1, count, false) then
        error("probe record export failed")
    end
    output("META", meta)
    output("PLAN", plan)
    output("PREROLL_FRAMES", 10)
    output("FRAME_BEFORE", frame_before)
    output("FRAME_AFTER", genesis.live_forward_latest_frame_boundary_record())
    output("FRAMES", frame_count)
    output("STREAM", genesis.live_forward_stream_sequence())
    output("INSTRUCTION", genesis.live_forward_instruction_sequence())
    output("METRICS", genesis.live_forward_metrics())
    output("RESULT", "PASS")
    log:write("result=PASS\n")
    log:close()
    raw:close()
    client.exitCode(0)
end

local ok, err = pcall(run)
if not ok then
    output("RESULT", "FAIL")
    output("ERROR", err)
    log:write("result=FAIL\nerror=", tostring(err), "\n")
    log:close()
    raw:close()
    client.exitCode(1)
end
