add_executable(test_send_completion main.cpp)

target_link_libraries(test_send_completion 
PRIVATE
    network
    system
)