add_executable(1_virtual_network_interface main.cpp)

target_link_libraries(1_virtual_network_interface 
PRIVATE
    network
    system
)