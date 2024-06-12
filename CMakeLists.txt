cmake_minimum_required(VERSION 3.14)

add_executable(dds_replay   main.cpp)
target_include_directories(dds_replay PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    ${CMAKE_CURRENT_SOURCE_DIR}/idl/v2
)

target_link_libraries(dds_replay lz4 zstd DdsRecorderCommand DdsRecorderStatus)

install(TARGETS dds_replay DESTINATION bin)


add_subdirectory(idl/v2/DdsRecorderCommand)
add_subdirectory(idl/v2/DdsRecorderStatus)