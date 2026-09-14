-- AUTO67.6R3B baseline: no producer or global execution hook.
local M = {}

function M.create()
    local state = {}
    function state.set_frame() end
    function state.join_write(pc)
        return {status = "UNJOINED", target_pc = pc}
    end
    function state.lookup() return nil end
    function state.flush() end
    function state.lookup_count() return 0 end
    function state.snapshot()
        return {mode = "disabled", ring_capacity = 0, records_observed = 0,
            ring_overwrites = 0, ring_wrapped = false, frozen_slices = 0,
            flushed_slices = 0, consumer_joins = 0, unjoined_consumers = 0,
            pending_consumers = 0, active_global_hook = false, target_pc_count = 0}
    end
    function state.close() end
    return state
end

return M
