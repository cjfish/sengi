function onload()
    print(__FILE__ .." on loaded!");

    g_Data = {};
    g_Data["test_sum"] = 0;
end

function onreload()
    print(__FILE__ .." on reloaded!");
    print(g_Data["test_sum"]);
end

local tablePrinted = {}
function printTableItem(k, v, level)
    for i = 1, level do
        io.write("    ")
    end

    io.write(tostring(k), " = ", tostring(v), "\n")
    if type(v) == "table" then
        if not tablePrinted[v] then
            tablePrinted[v] = true
            for k, v in pairs(v) do
                printTableItem(k, v, level + 1)
            end
        end
    end
end

function test_sum(a, b)
    g_Data["test_sum"] = g_Data["test_sum"] + 1;

    -- printTableItem("_G", _G, 0)
    return a, b, sum(a, b);
end

