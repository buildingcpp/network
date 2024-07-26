add_executable(test_tcp_sender main.cpp)

target_link_libraries(test_tcp_sender 
PRIVATE
    network
    system
)