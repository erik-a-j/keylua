---@meta keylua

---@alias keylua.IfaceType "keyboard" | "mouse"

---@class keylua.DeviceConfig
---@field vid integer
---@field pid integer
---@field iface keylua.IfaceType
---@field name? string

---@class keylua.Device
local Device = {}

---Opaque handle to a sequence of input events. Produced by key(), keydown(),
---keyup(), and consumed by map().
---@class keylua.EventJob

---@param config keylua.DeviceConfig
---@return keylua.Device
function device(config) end

---Bind a trigger key to an action.
---@param trigger string  Key name that fires the mapping
---@param action string|keylua.EventJob  Either a key name (shorthand for press+release) or a job built with key()/keydown()/keyup()
function Device:map(trigger, action) end

---Bind a trigger key to an action.
---@param trigger string  Key name that fires the mapping
---@param action string|keylua.EventJob  Either a key name (shorthand for press+release) or a job built with key()/keydown()/keyup()
function map(trigger, action) end

---Create a key-down-only job.
---@param name string
---@return keylua.EventJob
function keydown(name) end

---Create a key-up-only job.
---@param name string
---@return keylua.EventJob
function keyup(name) end

---Create a press+release job for the given key.
---@param name string  Key name, e.g. "KEY_A" or "a"
---@return keylua.EventJob
function key(name) end
