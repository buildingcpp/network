add_executable(echo_server_demo main.cpp)

target_link_directories(echo_server_demo PUBLIC ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

target_link_libraries(echo_server_demo 
PRIVATE
    network
    system
)