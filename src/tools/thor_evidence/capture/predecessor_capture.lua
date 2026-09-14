-- AUTO67.6R2 one global execution history ring and bounded frozen slices.
local M = {}
local MAGIC, VERSION, RECORD_BYTES = "O67P", 2, 28
local HEADER_FORMAT = "<I4I4I4I4I4I4I4I4I4I4I4I4I4I4I4I4I4"
local RECORD_FORMAT = "<I4I4I4I4I4I4I4"
local RING_LIMIT = tonumber(os.getenv("OASIS_AUTO67_PREHISTORY_RING") or "4096") or 4096
local MAX_SLICES = 16

local function register(name)
    local ok, value = pcall(emu.getregister, "M68K " .. name)
    return ok and (tonumber(value) or 0) or nil
end

local function read_targets(path)
    local targets = {}
    if not path or path == "" then return targets end
    local file = io.open(path, "r")
    if not file then return targets end
    for line in file:lines() do
        local pc, mask = line:match("^([%x]+)|(%d+)$")
        if pc and mask then targets[tonumber(pc, 16)] = tonumber(mask) end
    end
    file:close()
    return targets
end

function M.create(frame_reader, target_path, hook_metrics)
    local state = {}
    local targets = read_targets(target_path)
    local target_pc_count = 0
    for _ in pairs(targets) do target_pc_count = target_pc_count + 1 end
    local hook = nil
    local frame, epoch = 0, 0
    local execution_sequence = 0
    local ring_start, ring_count, overwrites = 1, 0, 0
    local epochs, sequences, frames, pcs, opcodes = {}, {}, {}, {}, {}
    local latest_epoch, latest_sequence, latest_frame, latest_pc = nil, nil, nil, nil
    local slices, next_slice_id = {}, 1
    local pending_writes = {}
    local records_observed, consumer_joins, unjoined = 0, 0, 0

    local function append(address, opcode)
        execution_sequence = execution_sequence + 1
        local index
        if ring_count < RING_LIMIT then
            ring_count = ring_count + 1
            index = ((ring_start + ring_count - 2) % RING_LIMIT) + 1
        else
            index = ring_start
            ring_start = (ring_start % RING_LIMIT) + 1
            overwrites = overwrites + 1
        end
        epochs[index], sequences[index], frames[index] = epoch, execution_sequence, frame
        pcs[index], opcodes[index] = address or 0, opcode or 0
        latest_epoch, latest_sequence, latest_frame, latest_pc = epoch, execution_sequence, frame, address or 0
        records_observed = records_observed + 1
    end

    local function copy_slice(consumer_pc, mask, a4, a5, slice_id)
        if #slices >= MAX_SLICES then return nil end
        local first_index = ring_count > 0 and ring_start or 1
        local last_index = ring_count > 0 and
            ((ring_start + ring_count - 2) % RING_LIMIT) + 1 or first_index
        local slice = {
            id = slice_id or next_slice_id, epoch = latest_epoch, target_pc = consumer_pc,
            requested_mask = mask, first_sequence = ring_count > 0 and sequences[first_index] or 0,
            last_sequence = ring_count > 0 and sequences[last_index] or 0,
            record_count = ring_count, complete = true, truncated = false, gap = false,
            consumer_sequence = latest_sequence, consumer_frame = latest_frame,
            consumer_pc = consumer_pc, overwrites = overwrites,
            ring_capacity = RING_LIMIT, ring_wrapped = overwrites > 0,
            join_status = "EXACT", path = nil, flushed = false,
            epochs = {}, sequences = {}, frames = {}, pcs = {}, opcodes = {},
            a4 = {}, a5 = {}}
        for offset = 1, ring_count do
            local index = ((ring_start + offset - 2) % RING_LIMIT) + 1
            slice.epochs[offset], slice.sequences[offset] = epochs[index], sequences[index]
            slice.frames[offset], slice.pcs[offset] = frames[index], pcs[index]
            slice.opcodes[offset] = opcodes[index]
            slice.a4[offset], slice.a5[offset] = 0, 0
        end
        local consumer_offset = ring_count
        slice.a4[consumer_offset], slice.a5[consumer_offset] = a4 or 0, a5 or 0
        slices[#slices + 1] = slice
        return slice
    end

    local function finish_pending(address, opcode)
        for index = #pending_writes, 1, -1 do
            local pending = pending_writes[index]
            if pending.epoch == epoch and pending.pc == address then
                local a4, a5 = nil, nil
                if pending.mask & 1 ~= 0 then a4 = register("A4") end
                if pending.mask & 2 ~= 0 then a5 = register("A5") end
                local slice = copy_slice(address, pending.mask, a4, a5, pending.id)
                if slice then
                    local result = {id = slice.id, status = "EXACT",
                        exec_epoch = slice.epoch, exec_sequence = slice.consumer_sequence,
                        exec_frame = slice.consumer_frame, exec_pc = slice.consumer_pc,
                        target_pc = address}
                    consumer_joins = consumer_joins + 1
                    if pending.on_exact then pending.on_exact(result) end
                else
                    unjoined = unjoined + 1
                    if pending.on_exact then
                        pending.on_exact({status = "SLICE_CAPACITY", target_pc = address})
                    end
                end
                table.remove(pending_writes, index)
                return
            end
        end
    end

    local function callback(address, opcode)
        append(address, opcode)
        finish_pending(address, opcode)
    end

    local register_hook = function()
        return event.on_bus_exec_any(callback, "AUTO67.6R2 global prehistory", "M68K BUS")
    end
    hook = hook_metrics and hook_metrics.register("prehistory_global_exec", register_hook) or register_hook()

    function state.set_frame(value)
        frame = value or frame
        epoch = math.floor(frame / 600)
    end

    function state.join_write(pc, on_exact)
        local mask = targets[pc]
        if not mask then return nil end
        local result = {id = nil, status = "UNJOINED", exec_epoch = latest_epoch,
                        exec_sequence = latest_sequence, exec_frame = latest_frame,
                        exec_pc = latest_pc, target_pc = pc}
        if latest_sequence == nil or latest_epoch ~= epoch then
            unjoined = unjoined + 1
            return result
        end
        if latest_pc == pc then
            local a4, a5 = nil, nil
            if mask & 1 ~= 0 then a4 = register("A4") end
            if mask & 2 ~= 0 then a5 = register("A5") end
            local slice_id = next_slice_id
            next_slice_id = next_slice_id + 1
            local slice = copy_slice(pc, mask, a4, a5, slice_id)
            if not slice then
                result.status = "SLICE_CAPACITY"
                unjoined = unjoined + 1
                return result
            end
            result.id, result.status = slice.id, "EXACT"
            result.exec_epoch, result.exec_sequence = slice.epoch, slice.consumer_sequence
            result.exec_frame, result.exec_pc = slice.consumer_frame, slice.consumer_pc
            consumer_joins = consumer_joins + 1
            if on_exact then on_exact(result) end
            return result
        end
        if #pending_writes >= 32 then
            unjoined = unjoined + 1
            return result
        end
        result.status = "PENDING"
        result.id = next_slice_id
        next_slice_id = next_slice_id + 1
        pending_writes[#pending_writes + 1] = {id = result.id, pc = pc, mask = mask,
                                               epoch = epoch, on_exact = on_exact}
        return result
    end

    function state.lookup(id)
        for _, slice in ipairs(slices) do
            if slice.id == id then
                return {id = slice.id, path = slice.path, status = slice.join_status,
                        exec_epoch = slice.epoch, exec_sequence = slice.consumer_sequence,
                        exec_frame = slice.consumer_frame, exec_pc = slice.consumer_pc,
                        first_sequence = slice.first_sequence, last_sequence = slice.last_sequence,
                        record_count = slice.record_count, ring_capacity = slice.ring_capacity,
                        ring_wrapped = slice.ring_wrapped, overwrites = slice.overwrites}
            end
        end
        return nil
    end

    local function write_slice(slice, directory)
        if slice.flushed then return end
        local path = string.format("%s/prehistory-%08X.o67p", directory, slice.id)
        local file = io.open(path, "wb")
        if not file then return end
        file:write(MAGIC, string.pack(HEADER_FORMAT, VERSION,
            slice.epoch, slice.target_pc, slice.requested_mask, slice.first_sequence,
            slice.last_sequence, slice.record_count, slice.complete and 1 or 0,
            slice.truncated and 1 or 0, slice.gap and 1 or 0,
            slice.consumer_sequence or 0xFFFFFFFF, slice.consumer_frame or 0xFFFFFFFF,
            slice.overwrites, slice.ring_capacity, slice.ring_wrapped and 1 or 0,
            slice.consumer_pc or 0xFFFFFFFF, slice.join_status == "EXACT" and 1 or 0))
        for i = 1, slice.record_count do
            file:write(string.pack(RECORD_FORMAT, slice.epochs[i], slice.sequences[i],
                slice.frames[i], slice.pcs[i], slice.opcodes[i], slice.a4[i], slice.a5[i]))
        end
        file:close()
        slice.path, slice.flushed = path, true
        slice.epochs, slice.sequences, slice.frames, slice.pcs, slice.opcodes = nil, nil, nil, nil, nil
        slice.a4, slice.a5 = nil, nil
    end

    function state.flush(directory)
        for _, slice in ipairs(slices) do write_slice(slice, directory) end
    end

    function state.lookup_count()
        local flushed = 0
        for _, slice in ipairs(slices) do flushed = flushed + (slice.flushed and 1 or 0) end
        return flushed
    end

    function state.snapshot()
        return {ring_capacity = RING_LIMIT, records_observed = records_observed,
                ring_overwrites = overwrites, ring_wrapped = overwrites > 0,
                frozen_slices = #slices, flushed_slices = state.lookup_count(),
                consumer_joins = consumer_joins, unjoined_consumers = unjoined,
                pending_consumers = #pending_writes,
                active_global_hook = hook ~= nil, target_pc_count = target_pc_count}
    end

    function state.close()
        if hook then
            if hook_metrics then hook_metrics.unregister("prehistory_global_exec", function()
                event.unregisterbyid(hook)
            end) else event.unregisterbyid(hook) end
            hook = nil
        end
    end

    return state
end

return M
