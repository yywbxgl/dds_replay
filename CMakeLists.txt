cmake_minimum_required(VERSION 3.14)

add_executable(dds_replay   dds_replay.cpp)
target_include_directories(dds_replay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS dds_replay DESTINATION bin)


