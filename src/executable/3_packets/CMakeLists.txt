add_executable(3_packets main.cpp)

target_link_libraries(3_packets 
PRIVATE
    network
    system
)