cmake_minimum_required(VERSION 3.16)
project(xf_swarm_usage LANGUAGES C)
include(cmake/XFSwarmConfig.cmake)
add_executable(xf_example examples/basic.c)
target_link_libraries(xf_example PRIVATE XF::swarm XF::eots)
target_compile_features(xf_example PRIVATE c_std_99)
if(WIN32)
    add_custom_command(TARGET xf_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:XF::swarm> $<TARGET_FILE_DIR:xf_example>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:XF::eots> $<TARGET_FILE_DIR:xf_example>)
endif()
enable_testing()
add_test(NAME usage_example COMMAND xf_example)
