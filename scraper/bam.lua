function intermediate_output(settings, input)
    return PathJoin("objs", PathBase(input) .. settings.config_ext)
end

settings = NewSettings()
settings.cc.flags:Add("-Wall")
settings.cc.flags_cxx:Add("-std=c++11")
settings.optimize = 1
settings.cc.Output = intermediate_output

main = Collect("*.cpp")
source = Collect("src/*.cpp")
util = Collect("src/util/*.cpp")

util_obj = Compile(settings, util)
source_obj = Compile(settings, source)
main_obj = Compile(settings, main)

exe = Link(settings, "scraper", util_obj, source_obj, main_obj)
