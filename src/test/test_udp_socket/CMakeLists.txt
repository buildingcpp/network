add_executable(test_udp_socket main.cpp)

target_link_libraries(test_udp_socket 
PRIVATE
    network
    system
)