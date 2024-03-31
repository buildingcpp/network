add_executable(5_receive_side_scaling main.cpp)

target_link_libraries(5_receive_side_scaling 
PRIVATE
    network
    system
)