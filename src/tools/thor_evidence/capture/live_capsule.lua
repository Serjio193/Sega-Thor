-- AUTO67.1 developer-only discovery ring and targeted frozen capsules.
local status_path = os.getenv("OASIS_LIVE_STATUS")
local final_path = os.getenv("OASIS_LIVE_FINAL")
local command_path = os.getenv("OASIS_CAPSULE_COMMANDS")
local capsule_dir = os.getenv("OASIS_CAPSULE_DIR") or "."
local max_frames = tonumber(os.getenv("OASIS_LIVE_MAX_FRAMES") or "0") or 0
local state_path = os.getenv("OASIS_LIVE_STATE")
local hook_metrics_path = os.getenv("OASIS_AUTO67_HOOK_METRICS")
local hook_metrics = nil
if hook_metrics_path then
    local loader, load_error = loadfile(hook_metrics_path)
    assert(loader, load_error)
    hook_metrics = loader()
end
local source_path = debug.getinfo(1, "S").source:sub(2)
local source_dir = source_path:match("^(.*[\\/])") or ""
local predecessor_module = dofile(source_dir .. "predecessor_capture.lua")
local discovery_capacity = 256
local capsule_capacity = 131072
local capsule_magic, capsule_format_version = "O67V", 2
local logical_header_bytes, physical_header_bytes, record_size = 64, 24, 20
local callback_budget = 64
local capsule_count = 16
local frame = 0
local sequence = 0
local callbacks = 0
local discovery_overwrites = 0
local discovery_count = 0
local discovery_start = 1
local discovery = {}
local capsules = {}
local command_sequence = 0
local command_poll_interval = 4
local discovery_burst_period = 30
local discovery_burst_budget = 64
local discovery_burst_calls = 0
local discovery_burst_count = 0
local frame_time_capacity = 512
local frame_time_count = 0
local frame_time_start = 1
local frame_times = {}
local frame_spike_count = 0
local frame_spike_start = 1
local frame_spikes = {}
local frame_spike_counts = {over_16ms = 0, over_33ms = 0, over_50ms = 0}
local largest_frame_spike = {frame = 0, duration_ms = 0, leases = {}}
local filter_install_count = 0
local filter_install_samples = {}
local predecessor_capture = predecessor_module.create(function() return frame end)
local discovery_active = false
local discovery_write_hook = nil
local target_write_hooks = {}
local exec_hooks = {}
if state_path and state_path ~= "" then
    assert(savestate.load(state_path, true), "AUTO67 state load failed")
    for _ = 1, 3 do emu.frameadvance() end
end

for i = 1, discovery_capacity do discovery[i] = {frame = 0, pc = 0} end
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

local function append_discovery(kind, pc, address)
    local index
    if discovery_count < discovery_capacity then
        discovery_count = discovery_count + 1
        index = ((discovery_start + discovery_count - 2) % discovery_capacity) + 1
    else
        index = discovery_start
        discovery_start = (discovery_start % discovery_capacity) + 1
        discovery_overwrites = discovery_overwrites + 1
    end
    discovery[index] = {seq = sequence, frame = frame, epoch = math.floor(frame / 600), kind = kind,
                        pc = pc, address = address}
    sequence = sequence + 1
end

local function new_capsule(id)
    return {id = id, state = "FREE", worker = nil, investigation = nil,
            lease = nil, filter_type = nil, filter_value = 0, start_frame = 0,
            last_frame = 0, bytes_used = 0, event_count = 0,
            capture_start = 0, capture_duration = 0, frames_covered = 0,
            freeze_reason = nil, full = false, truncated = false, records = {},
            frame_records = 0, hook_paused = false, predecessor_enabled = false,
            predecessor_registers = {}, predecessor_target_pc = 0,
            predecessor_ring = {}, predecessor_freeze_requested = false}
end
for i = 0, capsule_count - 1 do capsules[i] = new_capsule(i) end

local function capsule_bytes(capsule)
    return logical_header_bytes + capsule.event_count * record_size
end

local function append_capsule(capsule, kind, address, pc)
    if capsule.state ~= "CAPTURING" then return end
    if capsule.frame_records >= callback_budget then
        capsule.truncated = true
        capsule.freeze_reason = "CALLBACK_BUDGET"
        capsule.hook_paused = true
        return
    end
    local bytes = capsule_bytes(capsule)
    if bytes + record_size > capsule_capacity then
        capsule.full = true
        capsule.truncated = true
        capsule.freeze_reason = "CAPACITY"
        capsule.hook_paused = true
        return
    end
    capsule.event_count = capsule.event_count + 1
    capsule.bytes_used = bytes + record_size
    sequence = sequence + 1
    capsule.records[capsule.event_count] = {sequence, frame, address or 0, pc or 0, kind}
    capsule.frame_records = capsule.frame_records + 1
    capsule.last_frame = frame
end

local function freeze(capsule, reason)
    if capsule.state ~= "CAPTURING" then return end
    capsule.state = "FROZEN"
    capsule.freeze_reason = reason or capsule.freeze_reason or "FRAME_BUDGET"
    capsule.capture_duration = (frame - capsule.capture_start) / 60.0
    capsule.frames_covered = math.max(0, frame - capsule.start_frame)
    local path = string.format("%s/capsule-%02d-%s.bin", capsule_dir,
                               capsule.id, capsule.lease or "unknown")
    if capsule.predecessor_enabled then
        local metadata = predecessor_capture.write(capsule, path .. ".pred")
        capsule.predecessor_path = path .. ".pred"
        if metadata then
            capsule.predecessor_record_count = metadata.record_count
            capsule.predecessor_complete = metadata.complete
            capsule.predecessor_truncated = metadata.truncated
            capsule.predecessor_gap = metadata.gap
        end
    end
    local file = io.open(path, "wb")
    if file then
        file:write(capsule_magic, string.pack("<I4I4I4I4I4", capsule_format_version,
                    capsule.id, capsule.start_frame, capsule.bytes_used, capsule.event_count))
        for i = 1, capsule.event_count do
            local item = capsule.records[i]
            file:write(string.pack("<I4I4I4I4I4", item[1], item[2], item[3], item[4], item[5]))
        end
        file:close()
    end
end

local function unregister_exec(id)
    if exec_hooks[id] then
        if hook_metrics then
            hook_metrics.unregister("targeted_bus_exec", function()
                event.unregisterbyid(exec_hooks[id])
            end)
        else
            event.unregisterbyid(exec_hooks[id])
        end
        exec_hooks[id] = nil
    end
end

local function remove_write_hook()
    if discovery_write_hook then
        if hook_metrics then
            hook_metrics.unregister("discovery_bus_write", function()
                event.unregisterbyid(discovery_write_hook)
            end)
        else
            event.unregisterbyid(discovery_write_hook)
        end
        discovery_write_hook = nil
    end
    for id, hook in pairs(target_write_hooks) do
        if hook_metrics then
            hook_metrics.unregister("targeted_bus_write", function()
                event.unregisterbyid(hook)
            end)
        else
            event.unregisterbyid(hook)
        end
        target_write_hooks[id] = nil
    end
end

local function refresh_write_hook()
    if discovery_active and not discovery_write_hook then
        local register = function()
            return event.on_bus_write(function(address)
            local started = hook_metrics and hook_metrics.callback_start("discovery_bus_write")
            callbacks = callbacks + 1
            discovery_burst_calls = discovery_burst_calls + 1
            append_discovery("BUS_WRITE_PC", read_pc(), address)
            if discovery_burst_calls >= discovery_burst_budget then
                discovery_active = false
                if discovery_write_hook then
                    if hook_metrics then
                        hook_metrics.unregister("discovery_bus_write", function()
                            event.unregisterbyid(discovery_write_hook)
                        end)
                    else
                        event.unregisterbyid(discovery_write_hook)
                    end
                    discovery_write_hook = nil
                end
            end
            if hook_metrics then hook_metrics.callback_end("discovery_bus_write", started) end
        end, nil, "AUTO67.1 discovery writes", "M68K BUS")
        end
        discovery_write_hook = hook_metrics and hook_metrics.register("discovery_bus_write", register) or register()
    elseif not discovery_active and discovery_write_hook then
            if hook_metrics then
                hook_metrics.unregister("discovery_bus_write", function()
                    event.unregisterbyid(discovery_write_hook)
                end)
            else
                event.unregisterbyid(discovery_write_hook)
            end
            discovery_write_hook = nil
    end
    for id, capsule in pairs(capsules) do
        local needs_hook = capsule.state == "CAPTURING" and capsule.filter_type == "address"
            and not capsule.hook_paused
        if needs_hook and not target_write_hooks[id] then
            local capsule_id = id
            local register = function()
                return event.on_bus_write(function(address)
                local started = hook_metrics and hook_metrics.callback_start("targeted_bus_write")
                callbacks = callbacks + 1
                local active = capsules[capsule_id]
                if active.state == "CAPTURING" and not active.hook_paused then
                    append_capsule(active, 1, address, read_pc())
                    predecessor_capture.on_write(active, read_pc())
                end
                if hook_metrics then hook_metrics.callback_end("targeted_bus_write", started) end
            end, capsule.filter_value, "AUTO67.1 targeted writes", "M68K BUS")
            end
            target_write_hooks[id] = hook_metrics and hook_metrics.register("targeted_bus_write", register) or register()
        elseif not needs_hook and target_write_hooks[id] then
            if hook_metrics then
                hook_metrics.unregister("targeted_bus_write", function()
                    event.unregisterbyid(target_write_hooks[id])
                end)
            else
                event.unregisterbyid(target_write_hooks[id])
            end
            target_write_hooks[id] = nil
        end
    end
end

local function active_lease_snapshot()
    local result = {}
    for _, capsule in pairs(capsules) do
        if capsule.state == "CAPTURING" and capsule.lease then
            result[#result + 1] = {lease_id = capsule.lease,
                                   investigation_id = capsule.investigation}
        end
    end
    return result
end

local function record_filter_install(capsule, duration_ms)
    filter_install_count = filter_install_count + 1
    filter_install_samples[((filter_install_count - 1) % frame_time_capacity) + 1] = {
        frame = frame, lease_id = capsule.lease, investigation_id = capsule.investigation,
        duration_ms = duration_ms}
end

local function record_frame_timing(frame_number, duration_ms, leases)
    frame_time_count = math.min(frame_time_capacity, frame_time_count + 1)
    local index = ((frame_time_start + frame_time_count - 2) % frame_time_capacity) + 1
    if frame_time_count == frame_time_capacity then
        frame_time_start = (frame_time_start % frame_time_capacity) + 1
    end
    frame_times[index] = duration_ms
    if duration_ms >= 16 then frame_spike_counts.over_16ms = frame_spike_counts.over_16ms + 1 end
    if duration_ms >= 33 then frame_spike_counts.over_33ms = frame_spike_counts.over_33ms + 1 end
    if duration_ms >= 50 then frame_spike_counts.over_50ms = frame_spike_counts.over_50ms + 1 end
    if duration_ms < 16 then return end
    frame_spike_count = math.min(frame_time_capacity, frame_spike_count + 1)
    local spike_index = ((frame_spike_start + frame_spike_count - 2) % frame_time_capacity) + 1
    if frame_spike_count == frame_time_capacity then
        frame_spike_start = (frame_spike_start % frame_time_capacity) + 1
    end
    frame_spikes[spike_index] = {frame = frame_number, duration_ms = duration_ms, leases = leases}
    if duration_ms > largest_frame_spike.duration_ms then
        largest_frame_spike = {frame = frame_number, duration_ms = duration_ms, leases = leases}
    end
end

local function install_exec(capsule)
    if capsule.filter_type ~= "pc" or capsule.hook_paused or exec_hooks[capsule.id] then
        return
    end
    local register = function()
        return event.on_bus_exec(function(address)
        local started = hook_metrics and hook_metrics.callback_start("targeted_bus_exec")
        callbacks = callbacks + 1
        if capsule.state ~= "CAPTURING" or capsule.hook_paused then
            if hook_metrics then hook_metrics.callback_end("targeted_bus_exec", started) end
            return
        end
        append_capsule(capsule, 2, address, capsule.filter_value)
        if capsule.hook_paused then unregister_exec(capsule.id) end
        if hook_metrics then hook_metrics.callback_end("targeted_bus_exec", started) end
    end, capsule.filter_value, "AUTO67.1 targeted exec", "M68K BUS")
    end
    exec_hooks[capsule.id] = hook_metrics and hook_metrics.register("targeted_bus_exec", register) or register()
end

local function start_capsule(parts)
    local id = tonumber(parts[3])
    local capsule = capsules[id]
    if not capsule or capsule.state ~= "FREE" then return end
    capsule.state = "CLAIMED"
    capsule.lease, capsule.worker, capsule.investigation = parts[4], tonumber(parts[5]), parts[6]
    capsule.filter_type, capsule.filter_value = parts[7], tonumber(parts[8]) or 0
    local requested = {}
    for item in (parts[12] or ""):gmatch("[^,]+") do requested[#requested + 1] = item end
    if parts[10] == "1" then
        predecessor_capture.start(capsule, tonumber(parts[11]) or 0, requested)
    end
    capsule.start_frame, capsule.last_frame, capsule.capture_start = frame, frame, frame
    capsule.bytes_used, capsule.event_count, capsule.freeze_reason = 64, 0, nil
    capsule.capture_duration, capsule.frames_covered = 0, 0
    capsule.full, capsule.truncated = false, false
    capsule.frame_records, capsule.hook_paused = 0, false
    capsule.records = {}
    capsule.predecessor_freeze_requested = false
    capsule.state = "CAPTURING"
    local install_started = os.clock()
    install_exec(capsule)
    refresh_write_hook()
    record_filter_install(capsule, (os.clock() - install_started) * 1000.0)
end

local function release_capsule(parts)
    local id = tonumber(parts[3])
    local capsule = capsules[id]
    if not capsule or capsule.lease ~= parts[4] then return end
    unregister_exec(id)
    if capsule.state == "CAPTURING" then freeze(capsule, "RELEASE") end
    capsule.state = "FREE"
    capsule.records = {}
    capsule.lease, capsule.worker, capsule.investigation = nil, nil, nil
    capsule.bytes_used, capsule.event_count = 0, 0
    capsule.frame_records, capsule.hook_paused = 0, false
    capsule.predecessor_enabled = false
    refresh_write_hook()
end

local function read_commands()
    if not command_path then return end
    local file = io.open(command_path, "r")
    if not file then return end
    for line in file:lines() do
        local parts = {}
        for item in line:gmatch("[^|]+") do parts[#parts + 1] = item end
        local seq = tonumber(parts[2]) or 0
        if seq > command_sequence then
            command_sequence = seq
            if parts[1] == "START" then start_capsule(parts)
            elseif parts[1] == "RELEASE" then release_capsule(parts) end
        end
    end
    file:close()
end

local status_writer = dofile(source_dir .. "live_capsule_status.lua")
local function write_status(path)
    status_writer.write(path, path == final_path, {
        capsules = capsules, capsule_count = capsule_count, capsule_dir = capsule_dir,
        logical_header_bytes = logical_header_bytes, physical_header_bytes = physical_header_bytes,
        record_size = record_size, format_version = capsule_format_version,
        capsule_capacity = capsule_capacity, frame = frame, sequence = sequence,
        discovery_overwrites = discovery_overwrites, callbacks = callbacks,
        discovery_burst_period = discovery_burst_period,
        discovery_burst_budget = discovery_burst_budget,
        discovery_burst_count = discovery_burst_count, discovery_capacity = discovery_capacity,
        discovery_count = discovery_count, discovery = discovery, discovery_start = discovery_start,
        hex = hex, frame_time_count = frame_time_count,
        frame_spike_counts = frame_spike_counts, largest_frame_spike = largest_frame_spike,
        frame_spike_count = frame_spike_count, frame_spikes = frame_spikes,
        frame_spike_start = frame_spike_start, frame_time_capacity = frame_time_capacity,
        filter_install_count = filter_install_count, filter_install_samples = filter_install_samples,
        hook_metrics = hook_metrics})
end

while max_frames == 0 or frame < max_frames do
    if frame % command_poll_interval == 0 then read_commands() end
    if frame % discovery_burst_period == 0 then
        discovery_active = true
        discovery_burst_calls = 0
        discovery_burst_count = discovery_burst_count + 1
    end
    for i = 0, capsule_count - 1 do
        local capsule = capsules[i]
        if capsule.state == "CAPTURING" then
            capsule.frame_records, capsule.hook_paused = 0, false
            install_exec(capsule)
        end
    end
    refresh_write_hook()
    predecessor_capture.refresh(capsules)
    for i = 0, capsule_count - 1 do
        local capsule = capsules[i]
        if capsule.state == "CAPTURING" and (frame - capsule.start_frame >= 30 or
                capsule.predecessor_freeze_requested) then
            freeze(capsule, capsule.freeze_reason or
                   (capsule.full and "CAPACITY" or "FRAME_BUDGET"))
            unregister_exec(i)
        end
    end
    refresh_write_hook()
    local frame_leases = active_lease_snapshot()
    if hook_metrics then hook_metrics.begin_frame() end
    local frame_started = os.clock()
    emu.frameadvance()
    local frame_duration_us = (os.clock() - frame_started) * 1000000.0
    if hook_metrics then hook_metrics.host_duration("frameadvance", frame_duration_us) end
    local budget_used = 0
    for i = 0, capsule_count - 1 do budget_used = budget_used + capsules[i].frame_records end
    if hook_metrics then
        hook_metrics.set_budget(budget_used)
        hook_metrics.end_frame(frame + 1, frame_leases)
    end
    record_frame_timing(frame + 1, frame_duration_us / 1000.0, frame_leases)
    frame = frame + 1
    append_discovery("FRAME_PC", read_pc(), nil)
    if frame % 4 == 0 then write_status(status_path) end
end

for i = 0, capsule_count - 1 do
    local capsule = capsules[i]
    unregister_exec(i)
    if capsule.state == "CAPTURING" then freeze(capsule, "SESSION_STOP") end
end
remove_write_hook()
predecessor_capture.close(capsules)
write_status(final_path)
client.exitCode(0)
