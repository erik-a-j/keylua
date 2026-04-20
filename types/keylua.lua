---@meta keylua

---@alias keylua.IfaceType "keyboard" | "mouse"
---@alias keylua.KeyName

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
---@param trigger keylua.KeyName  Key name that fires the mapping
---@param action keylua.KeyName|keylua.EventJob  Either a key name (shorthand for press+release) or a job built with key()/keydown()/keyup()
function Device:map(trigger, action) end
