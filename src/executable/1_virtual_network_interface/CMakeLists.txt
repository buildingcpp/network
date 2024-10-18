add_executable(1_virtual_network_interface main.cpp)


target_link_directories(1_virtual_network_interface PUBLIC ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

target_link_libraries(1_virtual_network_interface 
PRIVATE
    network
    system
)