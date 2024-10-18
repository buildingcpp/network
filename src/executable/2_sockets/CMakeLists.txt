add_executable(2_sockets main.cpp)

target_link_directories(2_sockets PUBLIC ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

target_link_libraries(2_sockets 
PRIVATE
    network
    system
)