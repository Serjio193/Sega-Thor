-- AUTO67.6R bounded per-investigation register predecessor capture.
local M = {}
local MAGIC, VERSION, RECORD_BYTES = "O67P", 1, 28
local RING_LIMIT = 256

local function register(name)
    local ok, value = pcall(emu.getregister, "M68K " .. name)
    return ok and (tonumber(value) or 0) or nil
end

local function mask(registers)
    local result = 0
    for _, name in ipairs(registers or {}) do
        if name == "A4" then result = result | 1 end
        if name == "A5" then result = result | 2 end
    end
    return result
end

function M.create(frame_reader)
    local execution_hook = nil
    local state = {}

    local function active(capsules)
        for _, capsule in pairs(capsules) do
            if capsule.state == "CAPTURING" and capsule.predecessor_enabled then
                return true
            end
        end
        return false
    end

    local function callback(capsules, address, opcode)
        local current_frame = frame_reader()
        local epoch = math.floor(current_frame / 600)
        local matched = false
        for _, capsule in pairs(capsules) do
            matched = matched or (capsule.state == "CAPTURING" and
                capsule.predecessor_enabled and capsule.predecessor_target_pc == address)
        end
        local current_a4, current_a5 = nil, nil
        if matched then
            current_a4, current_a5 = register("A4"), register("A5")
        end
        for _, capsule in pairs(capsules) do
            if capsule.state == "CAPTURING" and capsule.predecessor_enabled then
                local ring = capsule.predecessor_ring
                capsule.predecessor_sequence = capsule.predecessor_sequence + 1
                local index
                if capsule.predecessor_ring_count < RING_LIMIT then
                    capsule.predecessor_ring_count = capsule.predecessor_ring_count + 1
                    index = ((capsule.predecessor_ring_start +
                        capsule.predecessor_ring_count - 2) % RING_LIMIT) + 1
                else
                    index = capsule.predecessor_ring_start
                    capsule.predecessor_ring_start = (capsule.predecessor_ring_start % RING_LIMIT) + 1
                    capsule.predecessor_overwrites = capsule.predecessor_overwrites + 1
                end
                local values = capsule.predecessor_target_pc == address and
                    {A4 = current_a4, A5 = current_a5} or {A4 = nil, A5 = nil}
                ring[index] = {
                    epoch = epoch, sequence = capsule.predecessor_sequence,
                    frame = current_frame, pc = address,
                    opcode = opcode or 0,
                    A4 = values.A4, A5 = values.A5}
            end
        end
    end

    function state.start(capsule, target_pc, registers)
        capsule.predecessor_enabled = true
        capsule.predecessor_registers = registers or {"A4", "A5"}
        capsule.predecessor_target_pc = target_pc or 0
        capsule.predecessor_ring = {}
        capsule.predecessor_ring_start = 1
        capsule.predecessor_ring_count = 0
        capsule.predecessor_sequence = 0
        capsule.predecessor_overwrites = 0
        capsule.predecessor_consumer = nil
    end

    function state.on_write(capsule, pc)
        if not capsule.predecessor_enabled or pc ~= capsule.predecessor_target_pc then
            return
        end
        local ring = capsule.predecessor_ring
        for offset = capsule.predecessor_ring_count, 1, -1 do
            local index = ((capsule.predecessor_ring_start + offset - 2) % RING_LIMIT) + 1
            if ring[index].pc == pc then
                capsule.predecessor_consumer = ring[index]
                capsule.predecessor_freeze_requested = true
                return
            end
        end
    end

    function state.refresh(capsules)
        local needed = active(capsules)
        if needed and not execution_hook then
            execution_hook = event.on_bus_exec_any(function(address, opcode)
                callback(capsules, address, opcode)
            end, "AUTO67.6R targeted predecessor", "M68K BUS")
        elseif not needed and execution_hook then
            event.unregisterbyid(execution_hook)
            execution_hook = nil
        end
    end

    function state.write(capsule, path)
        if not capsule.predecessor_enabled then return nil end
        local ring = capsule.predecessor_ring
        local count = capsule.predecessor_ring_count
        local first_item = count > 0 and ring[capsule.predecessor_ring_start] or nil
        local last_index = count > 0 and
            ((capsule.predecessor_ring_start + count - 2) % RING_LIMIT) + 1 or nil
        local last_item = last_index and ring[last_index] or nil
        local first = first_item and first_item.sequence or 0
        local last = last_item and last_item.sequence or 0
        local consumer = capsule.predecessor_consumer
        local complete = consumer ~= nil
        local truncated = not complete or capsule.predecessor_overwrites > 0
        local gap = false
        local file = io.open(path, "wb")
        if not file then return nil end
        file:write(MAGIC, string.pack("<I4I4I4I4I4I4I4I4I4I4I4I4I4", VERSION,
            consumer and consumer.epoch or math.floor(frame_reader() / 600),
            capsule.predecessor_target_pc, mask(capsule.predecessor_registers),
            first, last, count, complete and 1 or 0, truncated and 1 or 0,
            gap and 1 or 0, consumer and consumer.sequence or 0xFFFFFFFF,
            consumer and consumer.frame or 0xFFFFFFFF, capsule.predecessor_overwrites))
        for offset = 1, count do
            local index = ((capsule.predecessor_ring_start + offset - 2) % RING_LIMIT) + 1
            local item = ring[index]
            file:write(string.pack("<I4I4I4I4I4I4I4", item.epoch, item.sequence,
                item.frame, item.pc, item.opcode, item.A4 or 0, item.A5 or 0))
        end
        file:close()
        return {path = path, record_count = count, first_sequence = first,
                last_sequence = last, complete = complete, truncated = truncated,
                gap = gap, overwrites = capsule.predecessor_overwrites,
                consumer_sequence = consumer and consumer.sequence or nil,
                consumer_frame = consumer and consumer.frame or nil}
    end

    function state.close(capsules)
        if execution_hook then
            event.unregisterbyid(execution_hook)
            execution_hook = nil
        end
        for _, capsule in pairs(capsules) do
            capsule.predecessor_enabled = false
        end
    end

    return state
end

return M
