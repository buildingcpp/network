add_executable(4_async_poll_send_and_recv main.cpp)

target_link_libraries(4_async_poll_send_and_recv 
PRIVATE
    network
    system
)