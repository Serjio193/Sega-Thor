-- Bounded AUTO67.2 hook/callback profiler. Loaded by live_capsule.lua.
local M = {}
local capacity = 512
local classes, registrations, unregistrations, frames = {}, {}, {}, {}
local frame_count, frame_start = 0, 1
local current = {}
local current_calls, current_callback_us, current_host_us, current_budget = 0, 0, 0, 0
local active_hooks = 0

local function series()
    return {values = {}, count = 0, start = 1}
end

local function add(target, value)
    target.count = math.min(capacity, target.count + 1)
    local index = ((target.start + target.count - 2) % capacity) + 1
    if target.count == capacity then target.start = (target.start % capacity) + 1 end
    target.values[index] = value
end

local function class(name, kind)
    if not classes[name] then
        classes[name] = {kind = kind, active = 0, registrations = 0,
            unregistrations = 0, calls = 0, total_us = 0, samples = series(),
            frame_calls = series(), frame_self = series()}
    end
    return classes[name]
end

local function summary(target)
    local values = {}
    for i = 0, target.count - 1 do
        values[#values + 1] = target.values[((target.start + i - 1) % capacity) + 1]
    end
    table.sort(values)
    local function pick(ratio)
        if #values == 0 then return 0 end
        return values[math.max(1, math.ceil(#values * ratio))]
    end
    return {count = #values, min = values[1] or 0, p50 = pick(.50),
        p90 = pick(.90), p95 = pick(.95), p99 = pick(.99), max = values[#values] or 0}
end

local function quote(value)
    local text = tostring(value or "")
    return '"' .. text:gsub("\\", "\\\\"):gsub('"', '\\"') .. '"'
end

local function summary_json(target)
    local value = summary(target)
    return '{"count":' .. value.count .. ',"min":' .. value.min ..
        ',"p50":' .. value.p50 .. ',"p90":' .. value.p90 ..
        ',"p95":' .. value.p95 .. ',"p99":' .. value.p99 ..
        ',"max":' .. value.max .. '}'
end

local function now_us(started)
    return (os.clock() - started) * 1000000.0
end

function M.callback_start(name)
    class(name, "callback")
    return os.clock()
end

function M.callback_end(name, started)
    local item = class(name, "callback")
    local elapsed = now_us(started)
    item.calls, item.total_us = item.calls + 1, item.total_us + elapsed
    add(item.samples, elapsed)
    current[name] = current[name] or {calls = 0, self_us = 0}
    current[name].calls = current[name].calls + 1
    current[name].self_us = current[name].self_us + elapsed
    current_calls, current_callback_us = current_calls + 1, current_callback_us + elapsed
end

function M.register(name, register_fn)
    local item, started = class(name, "callback"), os.clock()
    local result = register_fn()
    item.active, item.registrations = item.active + 1, item.registrations + 1
    active_hooks = active_hooks + 1
    registrations[name] = registrations[name] or series()
    add(registrations[name], now_us(started))
    return result
end

function M.unregister(name, unregister_fn)
    local item, started = class(name, "callback"), os.clock()
    unregister_fn()
    item.active = math.max(0, item.active - 1)
    item.unregistrations = item.unregistrations + 1
    active_hooks = math.max(0, active_hooks - 1)
    unregistrations[name] = unregistrations[name] or series()
    add(unregistrations[name], now_us(started))
end

function M.host_duration(name, duration_us)
    local item = class(name, "host_operation")
    item.calls, item.total_us = item.calls + 1, item.total_us + duration_us
    add(item.samples, duration_us)
    current[name] = current[name] or {calls = 0, self_us = 0}
    current[name].calls = current[name].calls + 1
    current[name].self_us = current[name].self_us + duration_us
    current_host_us = current_host_us + duration_us
end

function M.begin_frame()
    current, current_calls = {}, 0
    current_callback_us, current_host_us, current_budget = 0, 0, 0
end

function M.set_budget(value)
    current_budget = value or 0
end

function M.end_frame(frame, leases)
    frame_count = math.min(capacity, frame_count + 1)
    local index = ((frame_start + frame_count - 2) % capacity) + 1
    if frame_count == capacity then frame_start = (frame_start % capacity) + 1 end
    frames[index] = {frame = frame, calls = current_calls,
        callback_us = current_callback_us, host_us = current_host_us,
        budget = current_budget, active_hooks = active_hooks,
        leases = leases or {}, classes = current}
    for name, item in pairs(classes) do
        local observed = current[name] or {calls = 0, self_us = 0}
        add(item.frame_calls, observed.calls)
        add(item.frame_self, observed.self_us)
    end
end

local function frame_series(field)
    local target = series()
    for i = 0, frame_count - 1 do
        add(target, frames[((frame_start + i - 1) % capacity) + 1][field] or 0)
    end
    return target
end

local function classes_json(item)
    local values = {}
    for name, value in pairs(item.classes or {}) do
        values[#values + 1] = quote(name) .. ':{"calls":' .. value.calls ..
            ',"self_us":' .. string.format("%.3f", value.self_us) .. '}'
    end
    table.sort(values)
    return '{' .. table.concat(values, ',') .. '}'
end

local function frame_json(item)
    local leases = {}
    for _, lease in ipairs(item.leases or {}) do
        leases[#leases + 1] = '{"lease_id":' .. quote(lease.lease_id) ..
            ',"investigation_id":' .. quote(lease.investigation_id) .. '}'
    end
    return '{"frame":' .. item.frame .. ',"callbacks":' .. item.calls ..
        ',"callback_self_us":' .. string.format("%.3f", item.callback_us) ..
        ',"host_us":' .. string.format("%.3f", item.host_us) ..
        ',"budget":' .. item.budget .. ',"active_hooks":' .. item.active_hooks ..
        ',"leases":[' .. table.concat(leases, ',') .. '],"classes":' ..
        classes_json(item) .. '}'
end

local function timing_json(source, names)
    local values = {}
    for _, name in ipairs(names) do
        values[#values + 1] = quote(name) .. ':' .. summary_json(source[name] or series())
    end
    return '{' .. table.concat(values, ',') .. '}'
end

function M.json(detailed)
    local names = {}
    for name in pairs(classes) do names[#names + 1] = name end
    table.sort(names)
    local values = {}
    for _, name in ipairs(names) do
        local item = classes[name]
        values[#values + 1] = quote(name) .. ':{"kind":' .. quote(item.kind) ..
            ',"active":' .. item.active .. ',"registrations":' .. item.registrations ..
            ',"unregistrations":' .. item.unregistrations .. ',"calls":' .. item.calls ..
            ',"total_self_us":' .. string.format("%.3f", item.total_us) ..
            ',"avg_self_us":' .. string.format("%.3f", item.calls > 0 and item.total_us / item.calls or 0) ..
            ',"self_time_us":' .. summary_json(item.samples) ..
            ',"calls_per_frame":' .. summary_json(item.frame_calls) ..
            ',"self_time_per_frame_us":' .. summary_json(item.frame_self) .. '}'
    end
    local worst = {}
    if detailed then
        local copy = {}
        for i = 0, frame_count - 1 do
            copy[#copy + 1] = frames[((frame_start + i - 1) % capacity) + 1]
        end
        table.sort(copy, function(a, b) return a.callback_us > b.callback_us end)
        for i = 1, math.min(8, #copy) do worst[#worst + 1] = frame_json(copy[i]) end
    end
    return '{"filter_location":"native BizHawk registration argument; discovery hook is global Lua capture",' ..
        '"active_hooks":' .. active_hooks .. ',"classes":{' .. table.concat(values, ',') ..
        '},"registration_us":' .. timing_json(registrations, names) ..
        ',"unregistration_us":' .. timing_json(unregistrations, names) ..
        ',"frames":{"sample_count":' .. frame_count ..
        ',"callbacks_per_frame":' .. summary_json(frame_series("calls")) ..
        ',"callback_self_time_us_per_frame":' .. summary_json(frame_series("callback_us")) ..
        ',"host_time_us_per_frame":' .. summary_json(frame_series("host_us")) ..
        ',"callback_budget_per_frame":' .. summary_json(frame_series("budget")) ..
        ',"worst_callback_frames":[' .. table.concat(worst, ',') .. ']}}'
end

return M
