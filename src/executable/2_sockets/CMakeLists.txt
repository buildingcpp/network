add_executable(2_sockets main.cpp)

target_link_libraries(2_sockets 
PRIVATE
    network
    system
)