add_executable(oasis_runtime_rom_properties_core_test
    tests/runtime_rom_properties_core_test.c
    tools/bizhawk-native-ring/rom_properties_core.c)
target_include_directories(oasis_runtime_rom_properties_core_test PRIVATE
    tools/bizhawk-native-ring)
add_test(NAME oasis_runtime_rom_properties_core
         COMMAND oasis_runtime_rom_properties_core_test)
