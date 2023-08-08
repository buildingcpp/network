add_executable(multicast_demo main.cpp)

target_link_libraries(multicast_demo 
PRIVATE
    network
    system
)