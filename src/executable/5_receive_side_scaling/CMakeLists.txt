add_executable(5_receive_side_scaling main.cpp)

target_link_directories(5_receive_side_scaling PUBLIC ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

target_link_libraries(5_receive_side_scaling 
PRIVATE
    network
    system
)