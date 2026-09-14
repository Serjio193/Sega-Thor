-- AUTO67.6R3B bounded producer-triggered execution bursts.
local M = {}
local MAGIC, VERSION, RECORD_BYTES = "O67P", 2, 28
local HEADER_FORMAT = "<I4I4I4I4I4I4I4I4I4I4I4I4I4I4I4I4I4"
local RECORD_FORMAT = "<I4I4I4I4I4I4I4"
local PRODUCERS = {[0x002234] = 2, [0x0027BE] = 1}
local CONSUMER = 0x0027EC
local DEFAULT_BUDGET, MAX_SLICES = 64, 16

local function register(name)
    local ok, value = pcall(emu.getregister, "M68K " .. name)
    return ok and (tonumber(value) or 0) or nil
end

local function unregister(id, metrics, name)
    if not id then return end
    local function remove() event.unregisterbyid(id) end
    if metrics then metrics.unregister(name, remove) else remove() end
end

function M.create(frame_reader, _, hook_metrics)
    local state = {}
    local frame, epoch, next_id = 0, 0, 1
    local budget = tonumber(os.getenv("OASIS_AUTO67_BURST_BUDGET") or DEFAULT_BUDGET)
        or DEFAULT_BUDGET
    budget = math.max(1, math.min(DEFAULT_BUDGET, budget))
    local allow_burst = os.getenv("OASIS_AUTO67_PREHISTORY_MODE") ~= "targeted_idle"
    local producer_hooks, global_hook, active = {}, nil, nil
    local slices, completed_by_pc = {}, {}
    local hits = {[0x002234] = 0, [0x0027BE] = 0}
    local first_pcs, max_records, completed_records = {}, 0, 0
    local metrics = {mode = allow_burst and "targeted_burst" or "targeted_idle", burst_budget = budget,
        targeted_callback_count = 0, global_exec_callbacks = 0,
        producer_hits = hits, bursts_started = 0, bursts_completed = 0,
        bursts_budget_exhausted = 0, bursts_overlapping = 0,
        consumer_hits = 0, max_callbacks_in_burst = 0,
        mean_callbacks_per_completed_burst = 0, global_active_us = 0,
        global_duty_cycle = 0, boundary_gap = false, active_global_hook = false}

    local function append(burst, pc)
        burst.records[#burst.records + 1] = {epoch, burst.next_sequence,
            frame, pc or 0, 0, 0, 0}
        burst.next_sequence = burst.next_sequence + 1
    end

    local function write_slice(slice, directory)
        if slice.flushed then return end
        local path = string.format("%s/prehistory-%08X.o67p", directory, slice.id)
        local file = io.open(path, "wb")
        if not file then return end
        file:write(MAGIC, string.pack(HEADER_FORMAT, VERSION, slice.epoch,
            slice.target_pc, slice.requested_mask, 1, #slice.records,
            #slice.records, slice.complete and 1 or 0, slice.truncated and 1 or 0,
            0, slice.consumer_sequence or 0xFFFFFFFF,
            slice.consumer_frame or 0xFFFFFFFF, 0, budget, 0,
            CONSUMER, slice.complete and 1 or 0))
        for _, record in ipairs(slice.records) do
            file:write(string.pack(RECORD_FORMAT, table.unpack(record)))
        end
        file:close()
        slice.path, slice.flushed = path, true
    end

    local function finish(reason)
        local burst = active
        if not burst then return end
        if global_hook then
            unregister(global_hook, hook_metrics, "burst_global_exec")
            global_hook = nil
        end
        metrics.active_global_hook = false
        metrics.global_active_us = metrics.global_active_us +
            (os.clock() - burst.started_clock) * 1000000.0
        metrics.max_callbacks_in_burst = math.max(metrics.max_callbacks_in_burst,
                                                  burst.callbacks)
        max_records = math.max(max_records, #burst.records)
        local exact = reason == "CONSUMER"
        local slice = {id = burst.id, epoch = burst.epoch, target_pc = CONSUMER,
            requested_mask = burst.mask, records = burst.records, complete = exact,
            truncated = not exact, join_status = exact and "EXACT" or reason,
            consumer_sequence = exact and (#burst.records) or nil,
            consumer_frame = exact and frame or nil, path = nil, flushed = false}
        if #slices < MAX_SLICES then slices[#slices + 1] = slice end
        if exact then
            metrics.bursts_completed = metrics.bursts_completed + 1
            completed_records = completed_records + burst.callbacks
            metrics.mean_callbacks_per_completed_burst = completed_records /
                metrics.bursts_completed
            completed_by_pc[CONSUMER] = slice
        else
            metrics.bursts_budget_exhausted = metrics.bursts_budget_exhausted + 1
        end
        active = nil
    end

    local function global_callback(address)
        if not active then return end
        local burst = active
        metrics.global_exec_callbacks = metrics.global_exec_callbacks + 1
        burst.callbacks = burst.callbacks + 1
        if burst.callbacks == 1 then
            first_pcs[#first_pcs + 1] = address or 0
            if address == burst.producer_pc then metrics.boundary_gap = true end
        end
        append(burst, address)
        if address == CONSUMER then
            metrics.consumer_hits = metrics.consumer_hits + 1
            local a4, a5 = register("A4"), register("A5")
            burst.records[#burst.records][6], burst.records[#burst.records][7] = a4 or 0, a5 or 0
            finish("CONSUMER")
        elseif #burst.records >= budget then
            finish("BUDGET")
        end
    end

    local function start(producer_pc)
        if not allow_burst then return end
        if active then
            metrics.bursts_overlapping = metrics.bursts_overlapping + 1
            active.mask = active.mask | PRODUCERS[producer_pc]
            return
        end
        local burst = {id = next_id, epoch = epoch, producer_pc = producer_pc,
            mask = PRODUCERS[producer_pc], callbacks = 0, next_sequence = 1,
            records = {}, started_clock = os.clock()}
        next_id = next_id + 1
        append(burst, producer_pc)
        active = burst
        metrics.bursts_started = metrics.bursts_started + 1
        local function install() return event.on_bus_exec_any(global_callback,
            "AUTO67.6R3B burst", "M68K BUS") end
        global_hook = hook_metrics and hook_metrics.register("burst_global_exec", install) or install()
        metrics.active_global_hook = true
    end

    local function install_producer(pc)
        local function callback()
            metrics.targeted_callback_count = metrics.targeted_callback_count + 1
            hits[pc] = hits[pc] + 1
            start(pc)
        end
        local function install() return event.on_bus_exec(callback, pc,
            "AUTO67.6R3B producer", "M68K BUS") end
        producer_hooks[pc] = hook_metrics and hook_metrics.register("burst_targeted_exec", install) or install()
    end
    install_producer(0x002234)
    install_producer(0x0027BE)

    function state.set_frame(value)
        frame = value or frame
        epoch = math.floor(frame / 600)
    end

    function state.join_write(pc, on_exact)
        if pc ~= CONSUMER then return {status = "UNJOINED", target_pc = pc, exec_epoch = epoch} end
        local slice = completed_by_pc[pc]
        if not slice then return {status = "UNJOINED", target_pc = pc, exec_epoch = epoch} end
        completed_by_pc[pc] = nil
        local result = {id = slice.id, status = "EXACT", exec_epoch = slice.epoch,
            exec_sequence = slice.consumer_sequence, exec_frame = slice.consumer_frame,
            exec_pc = CONSUMER, target_pc = pc}
        if on_exact then on_exact(result) end
        return result
    end

    function state.lookup(id)
        for _, slice in ipairs(slices) do
            if slice.id == id then
                return {id = id, path = slice.path, status = slice.join_status,
                    exec_epoch = slice.epoch, exec_sequence = slice.consumer_sequence,
                    exec_frame = slice.consumer_frame, exec_pc = CONSUMER,
                    first_sequence = 1, last_sequence = #slice.records,
                    record_count = #slice.records, ring_capacity = budget,
                    ring_wrapped = false, overwrites = 0}
            end
        end
        return nil
    end

    function state.flush(directory)
        for _, slice in ipairs(slices) do write_slice(slice, directory) end
    end

    function state.lookup_count()
        local count = 0
        for _, slice in ipairs(slices) do count = count + (slice.flushed and 1 or 0) end
        return count
    end

    function state.snapshot()
        local first = first_pcs[#first_pcs] or 0
        local duty = frame > 0 and metrics.global_active_us / (frame * 16666.667) or 0
        metrics.global_duty_cycle = duty
        return {mode = metrics.mode, ring_capacity = budget,
            records_observed = metrics.global_exec_callbacks, ring_overwrites = 0,
            ring_wrapped = false, frozen_slices = #slices, flushed_slices = state.lookup_count(),
            consumer_joins = metrics.bursts_completed, unjoined_consumers = metrics.bursts_budget_exhausted,
            pending_consumers = 0, active_global_hook = metrics.active_global_hook,
            target_pc_count = 2, burst_budget = budget,
            targeted_callback_count = metrics.targeted_callback_count, producer_hits = hits,
            bursts_started = metrics.bursts_started, bursts_completed = metrics.bursts_completed,
            bursts_budget_exhausted = metrics.bursts_budget_exhausted,
            bursts_overlapping = metrics.bursts_overlapping, consumer_hits = metrics.consumer_hits,
            global_exec_callbacks = metrics.global_exec_callbacks,
            max_callbacks_in_burst = metrics.max_callbacks_in_burst,
            mean_callbacks_per_completed_burst = metrics.mean_callbacks_per_completed_burst,
            global_active_us = metrics.global_active_us, global_duty_cycle = duty,
            first_burst_pc = first, boundary_gap = metrics.boundary_gap,
            first_burst_pcs = first_pcs}
    end

    function state.close()
        finish("CLOSE")
        for pc, id in pairs(producer_hooks) do
            unregister(id, hook_metrics, "burst_targeted_exec")
            producer_hooks[pc] = nil
        end
    end
    return state
end

return M
