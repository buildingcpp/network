add_executable(test_virtual_network_interface main.cpp)

target_link_libraries(test_virtual_network_interface 
PRIVATE
    network
    system
)