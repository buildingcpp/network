add_executable(udp_demo main.cpp)

target_link_libraries(udp_demo 
PRIVATE
    network
    system
)