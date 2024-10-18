add_executable(tcp_demo main.cpp)

target_link_directories(tcp_demo PUBLIC ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

target_link_libraries(tcp_demo 
PRIVATE
    network
    system
)