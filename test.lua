
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
      
    printTableItem("_G", _G, 0)

    return a, b, sum(a, b);
end

