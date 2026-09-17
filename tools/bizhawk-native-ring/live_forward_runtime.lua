local raw_path = assert(os.getenv("LF_OUTPUT"), "LF_OUTPUT missing")
local ack_path = assert(os.getenv("LF_ACK"), "LF_ACK missing")
local run_id = assert(tonumber(os.getenv("LF_RUN_ID")), "LF_RUN_ID missing")
local max_frames = tonumber(os.getenv("LF_MAX_FRAMES")) or 1800
local raw = assert(io.open(raw_path, "w"))
local log = assert(io.open(assert(os.getenv("BH_TEST_LOG")), "w"))
local frame_count = 0

local function output(key, value)
    raw:write(key, "=", tostring(value), "\n")
    raw:flush()
end

local function advance(count)
    for _ = 1, count do
        emu.frameadvance()
        frame_count = frame_count + 1
    end
end

local function fields(value)
    local result = {}
    for item in value:gmatch("[^,]+") do result[#result + 1] = item end
    return result
end

local function records(generation, count)
    local pages = {}
    for offset = 0, count - 1, 512 do
        local capacity = math.min(512, count - offset)
        local page = genesis.live_forward_result_records(0, generation, offset, capacity)
        if page == "" then error("empty record page at offset " .. offset) end
        pages[#pages + 1] = page
    end
    return table.concat(pages, ";")
end

local function snapshot(prefix, expected_capture, expected_generation)
    local copy_start = os.clock()
    local meta = genesis.live_forward_result_info(0)
    if meta == "" then error(prefix .. " result metadata missing") end
    local m = fields(meta)
    local count = assert(tonumber(m[17]), "result record count missing")
    if tonumber(m[1]) ~= expected_capture or tonumber(m[2]) ~= expected_generation then
        error(prefix .. " result identity mismatch")
    end
    local entry = genesis.live_forward_result_state(0, false)
    local exit = genesis.live_forward_result_state(0, true)
    local flow = records(expected_generation, count)
    local copy_ms = (os.clock() - copy_start) * 1000
    local payload = table.concat({meta, entry, exit, flow}, "\n")
    local export_start = os.clock()
    output(prefix .. "_META", meta)
    output(prefix .. "_ENTRY", entry)
    output(prefix .. "_EXIT", exit)
    output(prefix .. "_RECORDS", flow)
    output(prefix .. "_COPY_MS", string.format("%.6f", copy_ms))
    output(prefix .. "_EXPORT_MS", string.format("%.6f", (os.clock() - export_start) * 1000))
    return payload, m
end

local function benchmark(name, frames, recycle)
    local durations = {}
    local captures = 0
    for i = 1, frames do
        local start = os.clock()
        advance(1)
        durations[i] = (os.clock() - start) * 1000
        if recycle and genesis.live_forward_status(0) == 3 then
            local meta = fields(genesis.live_forward_result_info(0))
            if #meta ~= 20 then error("performance result metadata malformed") end
            if genesis.live_forward_ack(0, tonumber(meta[1]), tonumber(meta[2]),
                tonumber(meta[3]), tonumber(meta[4])) then
                captures = captures + 1
                local next_id = 1000 + captures
                local epoch = genesis.live_forward_epoch()
                if not genesis.live_forward_request(0, next_id, next_id, run_id, epoch) then
                    error("performance Worker reconnect failed")
                end
            end
        end
    end
    table.sort(durations)
    output("PERF_" .. name, string.format("frames=%d,p50_ms=%.6f,max_ms=%.6f,captures=%d",
        frames, durations[math.floor((frames + 1) / 2)], durations[frames], captures))
end

local function wait_complete(name)
    for _ = 1, max_frames do
        local state = genesis.live_forward_status(0)
        if state == 3 then return end
        if state == 0 then error(name .. " Worker became FREE before completion") end
        advance(1)
    end
    error(name .. " timed out after " .. max_frames .. " frames")
end

local function wait_host_ack(name, meta)
    local identity = table.concat({meta[1], meta[2], meta[3], meta[4]}, "|")
    for _ = 1, max_frames do
        local ack = io.open(ack_path, "r")
        if ack then
            local value = ack:read("*a")
            ack:close()
            if value and value:sub(1, #identity) == identity then return end
        end
        advance(1)
    end
    error(name .. " host analysis ACK timed out")
end

local function acknowledge(name, meta)
    wait_host_ack(name, meta)
    if not genesis.live_forward_ack(0, tonumber(meta[1]), tonumber(meta[2]),
        tonumber(meta[3]), tonumber(meta[4])) then
        error(name .. " exact native ACK rejected")
    end
    output(name .. "_ACK", "accepted")
end

local function run()
    if not genesis.live_forward_enable(false) then error("cannot disable recorder for baseline") end
    benchmark("BASELINE", 120, false)
    if not genesis.live_forward_enable(true) then error("cannot enable native recorder") end
    benchmark("RECORDER", 120, false)

    if not genesis.live_forward_configure(1, 20, 65536) then error("Worker config rejected") end
    local epoch = genesis.live_forward_epoch()
    if not genesis.live_forward_request(0, 1000, 1000, run_id, epoch) then
        error("performance Worker request rejected")
    end
    benchmark("WORKER", 120, true)
    for _ = 1, 20 do
        if genesis.live_forward_status(0) == 3 then break end
        advance(1)
    end
    if genesis.live_forward_status(0) == 3 then
        local meta = fields(genesis.live_forward_result_info(0))
        if not genesis.live_forward_ack(0, tonumber(meta[1]), tonumber(meta[2]),
            tonumber(meta[3]), tonumber(meta[4])) then error("performance cleanup ACK failed") end
    else
        error("performance Worker did not complete")
    end

    if not genesis.live_forward_configure(1, 20, 65536) then error("proof Worker config rejected") end
    epoch = genesis.live_forward_epoch()
    if not genesis.live_forward_request(0, 1, 1, run_id, epoch) then error("capture 1 request rejected") end
    wait_complete("FIRST")
    local first_before, first_meta = snapshot("FIRST_BEFORE", 1, 1)
    local first_stream = genesis.live_forward_stream_sequence()
    advance(60)
    local first_after, _ = snapshot("FIRST_AFTER", 1, 1)
    local later_stream = genesis.live_forward_stream_sequence()
    output("FIRST_STREAM_BEFORE", first_stream)
    output("FIRST_STREAM_AFTER", later_stream)
    output("FIRST_RING_WRAPPED", later_stream - tonumber(first_meta[6]) >= 4096)
    output("FIRST_IMMUTABLE_EQUAL", first_before == first_after)
    output("FIRST_FRAMES_ADVANCED", frame_count)
    output("FIRST_READY", "true")
    acknowledge("FIRST", first_meta)

    advance(5)
    epoch = genesis.live_forward_epoch()
    if not genesis.live_forward_request(0, 2, 2, run_id, epoch) then error("capture 2 reconnect rejected") end
    wait_complete("SECOND")
    local second_before, second_meta = snapshot("SECOND_BEFORE", 2, 2)
    advance(10)
    local second_after = snapshot("SECOND_AFTER", 2, 2)
    output("SECOND_IMMUTABLE_EQUAL", second_before == second_after)
    output("SECOND_READY", "true")
    acknowledge("SECOND", second_meta)

    if not genesis.live_forward_configure(1, 100000, 512) then error("memory Worker config rejected") end
    epoch = genesis.live_forward_epoch()
    if not genesis.live_forward_request(0, 3, 3, run_id, epoch) then error("memory capture request rejected") end
    wait_complete("MEMORY")
    local memory_before, memory_meta = snapshot("MEMORY_BEFORE", 3, 3)
    advance(10)
    local memory_after = snapshot("MEMORY_AFTER", 3, 3)
    output("MEMORY_IMMUTABLE_EQUAL", memory_before == memory_after)
    output("MEMORY_STREAM_AFTER", genesis.live_forward_stream_sequence())
    output("MEMORY_READY", "true")
    acknowledge("MEMORY", memory_meta)
    output("FINAL_FRAMES", frame_count)
    output("RESULT", "PASS")
    raw:close()
    log:write("result=PASS\n")
    log:close()
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
