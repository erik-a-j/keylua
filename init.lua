local mouse = device({ vid = 0x1532, pid = 0x0015, iface = "keyboard", name = "Nagga mus" })
local kb = device({ vid = 0x046d, pid = 0xc339, iface = "keyboard", name = "logit" })

--mouse:map("1", { on_press = function(value) print(string.format("key=a, value=%d", value)) end })
mouse:map("KEY_1",
    { on_press = keydown("KEY_LEFTSHIFT") .. keydown("KEY_1"), on_release = keyup("KEY_1") .. keyup("KEY_LEFTSHIFT") })
mouse:map("KEY_2", { on_press = "KEY_B", on_repeat = "" })
mouse:map("KEY_3", {
    on_repeat = function(_)
        print("REPEAT KEY_3")
    end
})
mouse:map("KEY_4", "KEY_1")
