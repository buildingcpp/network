add_executable(udp_demo main.cpp)

target_link_directories(udp_demo PUBLIC ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

target_link_libraries(udp_demo 
PRIVATE
    network
    system
)