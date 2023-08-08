add_executable(tcp_demo main.cpp)

target_link_libraries(tcp_demo 
PRIVATE
    network
    system
)