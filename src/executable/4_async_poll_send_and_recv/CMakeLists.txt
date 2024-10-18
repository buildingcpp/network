add_executable(4_async_poll_send_and_recv main.cpp)

target_link_directories(4_async_poll_send_and_recv PUBLIC ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

target_link_libraries(4_async_poll_send_and_recv 
PRIVATE
    network
    system
)