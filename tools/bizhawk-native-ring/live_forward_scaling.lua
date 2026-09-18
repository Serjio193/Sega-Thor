local raw_path = assert(os.getenv("LF_OUTPUT"), "LF_OUTPUT missing")
local ack_path = assert(os.getenv("LF_ACK"), "LF_ACK missing")
local record_path = assert(os.getenv("LF_RECORD_DIR"), "LF_RECORD_DIR missing")
local run_id = assert(tonumber(os.getenv("LF_RUN_ID")), "LF_RUN_ID missing")
local worker_count = assert(tonumber(os.getenv("LF_COUNT")), "LF_COUNT missing")
local depth = assert(tonumber(os.getenv("LF_DEPTH")), "LF_DEPTH missing")
local memory_bytes = assert(tonumber(os.getenv("LF_MEMORY")), "LF_MEMORY missing")
local allocation_budget = assert(tonumber(os.getenv("LF_BUDGET_BYTES")), "LF_BUDGET_BYTES missing")
local free_disk_bytes = assert(tonumber(os.getenv("LF_FREE_DISK_BYTES")), "LF_FREE_DISK_BYTES missing")
local rounds = tonumber(os.getenv("LF_ROUNDS")) or 100
local max_frames = tonumber(os.getenv("LF_MAX_FRAMES")) or 1800
local control_enabled = os.getenv("LF_CONTROL_ENABLE") == "1"
local control_preview_path = os.getenv("LF_CONTROL_PREVIEW")
local saved_next_workers = tonumber(os.getenv("LF_NEXT_WORKERS")) or worker_count
local saved_next_depth = tonumber(os.getenv("LF_NEXT_DEPTH")) or depth
local raw = assert(io.open(raw_path, "w"))
local log = assert(io.open(assert(os.getenv("BH_TEST_LOG")), "w"))
local frame_count, ack_offset = 0, 0
local completion_wait_frames, ack_wait_frames = 0, 0
local worker_frame_times, worker_frame_enabled = {}, false
local publish_control

local function output(key, value)
    raw:write(key, "=", tostring(value), "\n")
    raw:flush()
end

local function advance(count, measure_worker)
    for _ = 1, count do
        local start = measure_worker and os.clock() or nil
        emu.frameadvance()
        frame_count = frame_count + 1
        if start then worker_frame_times[#worker_frame_times + 1] = (os.clock() - start) * 1000 end
        if control_enabled and frame_count % 15 == 0 and publish_control then
            publish_control()
        end
    end
    if worker_frame_enabled and #worker_frame_times >= 120 then
        worker_frame_enabled = false
        table.sort(worker_frame_times)
        output("PERF_WORKER", string.format(
            "frames=120,p50_ms=%.6f,max_ms=%.6f",
            worker_frame_times[60], worker_frame_times[120]))
    end
end

local function fields(value)
    local result = {}
    for item in value:gmatch("[^,]+") do result[#result + 1] = item end
    return result
end

local function pipe_fields(value)
    local result = {}
    for item in value:gmatch("[^|]+") do result[#result + 1] = item end
    return result
end

local function finish(code, result, preflight_reason)
    if control_enabled and publish_control then pcall(publish_control) end
    if preflight_reason then output("PREFLIGHT_REASON", preflight_reason) end
    output("RESULT", result)
    raw:close()
    log:write("result=", result, "\n")
    log:close()
    client.exitCode(code)
end

local function metric_snapshot()
    local value = genesis.live_forward_metrics()
    if value == "" then error("native lifecycle metrics missing") end
    return value
end

local function control_preview()
    local next_workers, next_depth, worker_offset = saved_next_workers, saved_next_depth, 0
    if control_preview_path then
        local file = io.open(control_preview_path, "r")
        if file then
            local body = file:read("*a") or ""
            file:close()
            local status = body:match("status=(%u+)")
            worker_offset = tonumber(body:match("worker_offset=(%d+)")) or 0
            if status == "UNREPRESENTABLE" then
                return 0, 0, "UNREPRESENTABLE", worker_offset
            end
            next_workers = tonumber(body:match("worker_count=(%d+)")) or next_workers
            next_depth = tonumber(body:match("chain_depth=(%d+)")) or next_depth
        end
    end
    if worker_offset >= worker_count then worker_offset = math.max(0, worker_count - 1) end
    local plan = genesis.live_forward_memory_plan(next_workers, next_depth, memory_bytes)
    if plan == "" then plan = "REJECTED" end
    return next_workers, next_depth, plan, worker_offset
end

publish_control = function()
    local metrics = fields(metric_snapshot())
    if #metrics ~= 30 then error("native control metrics width mismatch") end
    local pool_active = tonumber(metrics[1]) == worker_count and 1 or 0
    local next_workers, next_depth, next_plan, worker_offset = control_preview()
    local header = {frame_count, worker_count, depth, pool_active}
    for _, value in ipairs(metrics) do header[#header + 1] = value end
    local rows = {}
    local stop = math.min(worker_count, worker_offset + 64)
    for worker = worker_offset, stop - 1 do
        local values
        if pool_active == 1 then
            values = fields(genesis.live_forward_worker_status(worker))
        else
            values = {}
        end
        if #values == 7 then
            rows[#rows + 1] = table.concat({worker, values[1], values[2],
                tonumber(values[3]) > 0 and values[3] or depth, values[4],
                values[5], values[6], values[7]}, ",")
        else
            rows[#rows + 1] = table.concat({worker, 5, 0, depth, 0, 0, 0, 0}, ",")
        end
    end
    output("LIVE_CONTROL", table.concat(header, ",") .. "|" ..
        table.concat({next_workers, next_depth, next_plan}, ",") .. "|" ..
        worker_offset .. "|" .. table.concat(rows, ";"))
end

local function benchmark(name, frames)
    local durations = {}
    for index = 1, frames do
        local start = os.clock()
        advance(1, false)
        durations[index] = (os.clock() - start) * 1000
    end
    table.sort(durations)
    output("PERF_" .. name, string.format(
        "frames=%d,p50_ms=%.6f,max_ms=%.6f", frames,
        durations[math.floor((frames + 1) / 2)], durations[frames]))
end

local function wait_all_complete(round)
    for _ = 1, max_frames do
        local complete = 0
        for worker = 0, worker_count - 1 do
            local state = genesis.live_forward_status(worker)
            if state == 3 or state == 4 then complete = complete + 1 end
            if state == 0 then error("Worker " .. worker .. " became FREE before round " .. round .. " completed") end
        end
        if complete == worker_count then return end
        advance(1, true)
        completion_wait_frames = completion_wait_frames + 1
    end
    error("round " .. round .. " completion timed out")
end

local function wait_round_acks(round, expected)
    local received = {}
    local stream_before = genesis.live_forward_stream_sequence()
    for _ = 1, max_frames do
        advance(1, true)
        ack_wait_frames = ack_wait_frames + 1
        local ack = io.open(ack_path, "r")
        if ack then
            ack:seek("set", ack_offset)
            while true do
                local line = ack:read("*l")
                if not line then break end
                local parts = pipe_fields(line)
                if #parts == 7 then
                    local values = {}
                    for index, item in ipairs(parts) do values[index] = tonumber(item) end
                    if values[1] == round and expected[values[2]] then
                        local worker = values[2]
                        if values[3] ~= expected[worker].capture or
                           values[4] ~= expected[worker].generation or
                           values[5] ~= run_id or values[6] ~= expected[worker].epoch then
                            ack:close()
                            error("host ACK identity mismatch in round " .. round)
                        end
                        received[worker] = values[7]
                    end
                end
            end
            ack_offset = ack:seek()
            ack:close()
        end
        local received_count = 0
        for _ in pairs(received) do received_count = received_count + 1 end
        if received_count == worker_count then return received, stream_before end
    end
    error("round " .. round .. " host audit ACK timed out")
end

local function run()
    if not genesis.live_forward_enable(false) then error("cannot disable recorder for baseline") end
    benchmark("BASELINE", 120)
    local plan = genesis.live_forward_memory_plan(worker_count, depth, memory_bytes)
    if plan == "" then
        finish(0, "RESOURCE_PREFLIGHT_REJECTED", "NATIVE_PLANNER_REJECTED")
        return
    end
    output("PLAN", plan)
    local plan_values = fields(plan)
    local required_bytes = assert(tonumber(plan_values[16]), "native total allocation missing")
    if required_bytes > allocation_budget then
        output("REQUIRED_NATIVE_BYTES", required_bytes)
        output("ALLOCATION_BUDGET_BYTES", allocation_budget)
        finish(0, "RESOURCE_PREFLIGHT_REJECTED", "NATIVE_BUDGET_SHORTFALL")
        return
    end
    local required_disk_bytes = worker_count * (memory_bytes * 2 + rounds * 1536 + 256) + 4 * 1024 * 1024
    output("HOST_DISK_TRANSPORT_REQUIRED_BYTES", required_disk_bytes)
    output("HOST_DISK_AVAILABLE_BYTES", free_disk_bytes)
    if required_disk_bytes > free_disk_bytes then
        finish(0, "HOST_TRANSPORT_PREFLIGHT_REJECTED", "HOST_DISK_BUDGET_SHORTFALL")
        return
    end
    if not genesis.live_forward_configure_bounded(worker_count, depth,
        memory_bytes, allocation_budget) then
        finish(0, "NATIVE_ALLOCATION_REJECTED", "NATIVE_ALLOCATION_FAILED")
        return
    end
    if not genesis.live_forward_enable(true) then error("cannot enable recorder after bounded configure") end
    benchmark("RECORDER", 120)
    output("RECORDER_METRICS", metric_snapshot())
    worker_frame_enabled = true
    local performance_start = os.clock()
    local initial_epoch = genesis.live_forward_epoch()
    local record_file_1 = record_path .. "/live-forward-wave-records-pass1.bin"
    local record_file_2 = record_path .. "/live-forward-wave-records-pass2.bin"
    local expected_total = worker_count * rounds
    local first_round_metrics

    for round = 1, rounds do
        local expected = {}
        local epoch = genesis.live_forward_epoch()
        if epoch ~= initial_epoch then error("native execution epoch changed during lifecycle run") end
        for worker = 0, worker_count - 1 do
            local capture = run_id * 1000000 + (round - 1) * worker_count + worker + 1
            local generation = round
            if not genesis.live_forward_request(worker, capture, generation, run_id, epoch) then
                error("Worker " .. worker .. " request rejected in round " .. round)
            end
            expected[worker] = {capture = capture, generation = generation, epoch = epoch}
        end
        wait_all_complete(round)
        local snapshots, offset_1 = {}, 0
        for worker = 0, worker_count - 1 do
            local identity = expected[worker]
            local meta = genesis.live_forward_result_info(worker)
            if meta == "" then error("Worker " .. worker .. " result metadata missing") end
            local m = fields(meta)
            if tonumber(m[1]) ~= identity.capture or tonumber(m[2]) ~= identity.generation or
               tonumber(m[3]) ~= run_id or tonumber(m[4]) ~= epoch or
               tonumber(m[11]) ~= worker then
                error("Worker " .. worker .. " result identity mismatch")
            end
            local entry = genesis.live_forward_result_state(worker, false)
            local exit_state = genesis.live_forward_result_state(worker, true)
            local count = assert(tonumber(m[17]), "result record count missing")
            local export_start = os.clock()
            if not genesis.live_forward_export_records(record_file_1, worker,
                identity.generation, count, worker ~= 0) then
                error("Worker " .. worker .. " immutable result export failed")
            end
            local export_ms = (os.clock() - export_start) * 1000
            snapshots[worker] = {meta = meta, entry = entry, exit_state = exit_state,
                count = count, offset = offset_1, export_ms = export_ms}
            offset_1 = offset_1 + count
        end
        local immutable_stream_before = genesis.live_forward_stream_sequence()
        advance(1, true)
        local immutable_stream_after = genesis.live_forward_stream_sequence()
        if immutable_stream_after <= immutable_stream_before then
            error("CPU execution stream stopped before immutable result reread")
        end
        local offset_2 = 0
        for worker = 0, worker_count - 1 do
            local identity, first = expected[worker], snapshots[worker]
            local second_meta = genesis.live_forward_result_info(worker)
            local second_entry = genesis.live_forward_result_state(worker, false)
            local second_exit = genesis.live_forward_result_state(worker, true)
            local m = fields(second_meta)
            local second_count = assert(tonumber(m[17]), "reread record count missing")
            local export_start = os.clock()
            if not genesis.live_forward_export_records(record_file_2, worker,
                identity.generation, second_count, worker ~= 0) then
                error("Worker " .. worker .. " immutable reread export failed")
            end
            local export_ms = (os.clock() - export_start) * 1000
            output(string.format("SEG_%06d_%06d", round, worker), table.concat({
                round, first.meta, first.entry, first.exit_state, first.offset,
                second_meta, second_entry, second_exit, offset_2,
                string.format("%.6f", first.export_ms), string.format("%.6f", export_ms),
                immutable_stream_before, immutable_stream_after}, "|"))
            offset_2 = offset_2 + second_count
        end
        local received, stream_before = wait_round_acks(round, expected)
        local stream_after = genesis.live_forward_stream_sequence()
        if stream_after <= stream_before then
            error("CPU execution stream did not advance during host audit in round " .. round)
        end
        output(string.format("ROUND_%06d_AUDIT_STREAM_DELTA", round),
            stream_after - stream_before)
        for worker = 0, worker_count - 1 do
            local identity = expected[worker]
            local accepted = received[worker] == 1
            if not genesis.live_forward_mark_audited(worker, identity.generation, accepted) then
                error("Worker " .. worker .. " host audit mark rejected in round " .. round)
            end
            if not genesis.live_forward_ack(worker, identity.capture,
                identity.generation, run_id, identity.epoch) then
                error("Worker " .. worker .. " exact lifecycle ACK rejected in round " .. round)
            end
            if genesis.live_forward_status(worker) ~= 0 then
                error("Worker " .. worker .. " did not return to FREE after ACK")
            end
            local lifecycle = fields(genesis.live_forward_worker_lifecycle(worker))
            if #lifecycle ~= 4 then error("Worker lifecycle counters missing") end
            for _, value in ipairs(lifecycle) do
                if tonumber(value) ~= round then
                    error("Worker " .. worker .. " lifecycle transition count differs from generation")
                end
            end
            output(string.format("LIFECYCLE_%06d_%06d", round, worker),
                table.concat(lifecycle, ","))
            if not accepted then error("host segment audit rejected Worker " .. worker .. " round " .. round) end
        end
        output(string.format("ROUND_%06d_ACKED", round), worker_count)
        if round == 1 then first_round_metrics = metric_snapshot() end
        output(string.format("ROUND_%06d_METRICS", round), metric_snapshot())
    end

    if worker_frame_enabled then
        while #worker_frame_times < 120 do advance(1, true) end
    end
    local final_metrics = metric_snapshot()
    output("CAPTURE_METRICS", first_round_metrics or "")
    output("FINAL_METRICS", final_metrics)
    output("CAPTURE_TOTAL_REQUIRED", expected_total)
    output("WORKER_CYCLES_REQUIRED", rounds)
    output("WORKER_RUN_WALL_SECONDS", string.format("%.6f", os.clock() - performance_start))
    output("CAPTURE_COMPLETION_WAIT_FRAMES", completion_wait_frames)
    output("ACK_WAIT_FRAMES", ack_wait_frames)
    output("TOTAL_FRAMES", frame_count)
    finish(0, "PASS")
end

local ok, err = pcall(run)
if not ok then
    output("ERROR", err)
    log:write("error=", tostring(err), "\n")
    finish(1, "FAIL")
end
