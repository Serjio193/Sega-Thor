-- AUTO67 bounded status serializer kept outside the capture callback module.
local M = {}

local function json_string(value)
    local text = tostring(value or "")
    return '"' .. text:gsub("\\", "\\\\"):gsub('"', '\\"'):gsub("\n", "\\n") .. '"'
end

local function capsule_json(c, s)
    local path = c.lease and string.format("%s/capsule-%02d-%s.bin", s.capsule_dir,
                                            c.id, c.lease) or nil
    return '{"capsule_id":' .. c.id .. ',"state":' .. json_string(c.state) ..
        ',"worker_id":' .. (c.worker or "null") ..
        ',"investigation_id":' .. (c.investigation and json_string(c.investigation) or "null") ..
        ',"lease_id":' .. (c.lease and json_string(c.lease) or "null") ..
        ',"filter_type":' .. (c.filter_type and json_string(c.filter_type) or "null") ..
        ',"filter_value":' .. c.filter_value .. ',"start_frame":' .. c.start_frame ..
        ',"last_frame":' .. c.last_frame .. ',"bytes_used":' .. c.bytes_used ..
        ',"logical_header_bytes":' .. s.logical_header_bytes ..
        ',"physical_header_bytes":' .. s.physical_header_bytes ..
        ',"record_size":' .. s.record_size .. ',"format_version":' .. s.format_version ..
        ',"capsule_path":' .. (path and json_string(path) or "null") ..
        ',"capacity":' .. s.capsule_capacity .. ',"event_count":' .. c.event_count ..
        ',"capture_duration":' .. string.format("%.6f", c.capture_duration) ..
        ',"frames_covered":' .. c.frames_covered ..
        ',"freeze_reason":' .. (c.freeze_reason and json_string(c.freeze_reason) or "null") ..
        ',"full":' .. tostring(c.full) .. ',"truncated":' .. tostring(c.truncated) ..
        ',"predecessor_enabled":' .. tostring(c.predecessor_enabled) ..
        ',"predecessor_id":' .. (c.predecessor_id or "null") ..
        ',"predecessor_path":' .. (c.predecessor_path and json_string(c.predecessor_path) or "null") ..
        ',"predecessor_record_count":' .. (c.predecessor_record_count or 0) ..
        ',"predecessor_complete":' .. tostring(c.predecessor_complete or false) ..
        ',"predecessor_truncated":' .. tostring(c.predecessor_truncated or false) ..
        ',"predecessor_gap":' .. tostring(c.predecessor_gap or false) .. '}'
end

local function events_json(s)
    local values = {}
    for i = 0, s.discovery_count - 1 do
        local item = s.discovery[((s.discovery_start + i - 1) % s.discovery_capacity) + 1]
        local prehistory = item.prehistory_id and s.prehistory.lookup(item.prehistory_id) or nil
        local prehistory_path = prehistory and prehistory.path or
            (item.prehistory_id and string.format("%s/prehistory-%08X.o67p",
                                                   s.capsule_dir, item.prehistory_id) or nil)
        values[#values + 1] = '{"seq":' .. item.seq .. ',"frame":' .. item.frame ..
            ',"epoch":' .. item.epoch .. ',"kind":' .. json_string(item.kind) ..
            ',"pc":' .. json_string(s.hex(item.pc)) ..
            ',"address":' .. (item.address and json_string(s.hex(item.address)) or "null") ..
            ',"exec_epoch":' .. (item.exec_epoch or "null") ..
            ',"exec_sequence":' .. (item.exec_sequence or "null") ..
            ',"exec_frame":' .. (item.exec_frame or "null") ..
            ',"exec_pc":' .. (item.exec_pc and json_string(s.hex(item.exec_pc)) or "null") ..
            ',"consumer_join":' .. (item.consumer_join and json_string(item.consumer_join) or "null") ..
            ',"prehistory_id":' .. (item.prehistory_id or "null") ..
            ',"prehistory_path":' .. (prehistory_path and json_string(prehistory_path) or "null") ..
            ',"prehistory_first_sequence":' .. (prehistory and prehistory.first_sequence or "null") ..
            ',"prehistory_last_sequence":' .. (prehistory and prehistory.last_sequence or "null") ..
            ',"prehistory_record_count":' .. (prehistory and prehistory.record_count or "null") .. '}'
    end
    return "[" .. table.concat(values, ",") .. "]"
end

local function prehistory_json(item)
    item = item or {}
    return '{"ring_capacity":' .. (item.ring_capacity or 0) ..
        ',"records_observed":' .. (item.records_observed or 0) ..
        ',"ring_overwrites":' .. (item.ring_overwrites or 0) ..
        ',"ring_wrapped":' .. tostring(item.ring_wrapped or false) ..
        ',"frozen_slices":' .. (item.frozen_slices or 0) ..
        ',"flushed_slices":' .. (item.flushed_slices or 0) ..
        ',"consumer_joins":' .. (item.consumer_joins or 0) ..
        ',"unjoined_consumers":' .. (item.unjoined_consumers or 0) ..
        ',"pending_consumers":' .. (item.pending_consumers or 0) ..
        ',"active_global_hook":' .. tostring(item.active_global_hook or false) ..
        ',"target_pc_count":' .. (item.target_pc_count or 0) .. '}'
end

local function leases_json(items)
    local values = {}
    for _, item in ipairs(items or {}) do
        values[#values + 1] = '{"lease_id":' .. json_string(item.lease_id) ..
            ',"investigation_id":' .. json_string(item.investigation_id) .. '}'
    end
    return "[" .. table.concat(values, ",") .. "]"
end

local function spikes_json(s)
    local values = {}
    for i = 0, s.frame_spike_count - 1 do
        local item = s.frame_spikes[((s.frame_spike_start + i - 1) % s.frame_time_capacity) + 1]
        values[#values + 1] = '{"frame":' .. item.frame .. ',"duration_ms":' ..
            string.format("%.6f", item.duration_ms) .. ',"leases":' .. leases_json(item.leases) .. '}'
    end
    return "[" .. table.concat(values, ",") .. "]"
end

local function filter_json(s)
    local values = {}
    local first = math.max(1, s.filter_install_count - s.frame_time_capacity + 1)
    for number = first, s.filter_install_count do
        local item = s.filter_install_samples[((number - 1) % s.frame_time_capacity) + 1]
        values[#values + 1] = '{"frame":' .. item.frame .. ',"duration_ms":' ..
            string.format("%.6f", item.duration_ms) .. ',"lease_id":' ..
            json_string(item.lease_id) .. ',"investigation_id":' .. json_string(item.investigation_id) .. '}'
    end
    return "[" .. table.concat(values, ",") .. "]"
end

function M.write(path, final, s)
    local capsules = {}
    for i = 0, s.capsule_count - 1 do capsules[#capsules + 1] = capsule_json(s.capsules[i], s) end
    local timing = '{"sample_count":' .. s.frame_time_count ..
        ',"over_16ms":' .. s.frame_spike_counts.over_16ms ..
        ',"over_33ms":' .. s.frame_spike_counts.over_33ms ..
        ',"over_50ms":' .. s.frame_spike_counts.over_50ms ..
        ',"largest":{"frame":' .. s.largest_frame_spike.frame ..
        ',"duration_ms":' .. string.format("%.6f", s.largest_frame_spike.duration_ms) ..
        ',"leases":' .. leases_json(s.largest_frame_spike.leases) ..
        '},"spikes":' .. spikes_json(s) .. '}'
    local file = io.open(path, "w")
    if not file then return end
    file:write('{"schema":"oasis.m12.auto67.1.capsule.v2","frame":' .. s.frame ..
        ',"epoch":1,"events_observed":' .. s.sequence .. ',"events_overwritten":' ..
        s.discovery_overwrites .. ',"callback_count":' .. s.callbacks ..
        ',"sampling_policy":"AUTO67.1_FIXED_16_CAPSULES_TARGETED_BUDGETED"' ..
        ',"discovery_burst_period_frames":' .. s.discovery_burst_period ..
        ',"discovery_burst_callback_budget":' .. s.discovery_burst_budget ..
        ',"discovery_burst_count":' .. s.discovery_burst_count ..
        ',"discovery_capacity":' .. s.discovery_capacity .. ',"discovery_utilization":' ..
        s.discovery_count .. ',"capsules":[' .. table.concat(capsules, ",") ..
        '],"discovery":' .. events_json(s) .. ',"frame_timing":' .. timing ..
        ',"filter_install_samples":' .. filter_json(s) ..
        ',"prehistory":' .. prehistory_json(s.prehistory_metrics) ..
        (s.hook_metrics and ',"hook_metrics":' .. s.hook_metrics.json(final) or '') .. '}')
    file:close()
end

return M
