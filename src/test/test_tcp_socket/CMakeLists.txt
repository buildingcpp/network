add_executable(test_tcp_socket main.cpp)

target_link_libraries(test_tcp_socket 
PRIVATE
    network
    system
)