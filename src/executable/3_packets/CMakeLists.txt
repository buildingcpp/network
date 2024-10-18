add_executable(3_packets main.cpp)

target_link_directories(3_packets PUBLIC ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

target_link_libraries(3_packets 
PRIVATE
    network
    system
)