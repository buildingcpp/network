add_executable(echo_server_demo main.cpp)

target_link_libraries(echo_server_demo 
PRIVATE
    network
    system
)