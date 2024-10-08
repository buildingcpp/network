add_library(network
    ./network.cpp
    ./socket/socket_id.cpp
    ./ip/socket_address.cpp
    ./ip/port_id.cpp
    ./ip/ip_address.cpp
    ./ip/host_name.cpp
    ./socket/active_socket.cpp
    ./socket/passive_socket.cpp
    ./poller/epoller.cpp
    ./poller/kpoller.cpp
    ./network_interface/virtual_network_interface.cpp
    ./network_interface/network_interface_name.cpp
    ./socket/private/socket_base_impl.cpp
    ./socket/private/passive_socket_impl.cpp
    ./socket/private/active_socket_impl.cpp
)


target_link_libraries(network 
PUBLIC
    include
    system
    work_contract
    )

if (USE_KQUEUE)
    target_link_libraries(network PUBLIC kqueue)
endif()

target_include_directories(network
    PUBLIC
        ${_include_dir}/src
        ${_system_dir}/src
        ${_network_dir}/src
        ${_work_contract_dir}/src
        ${_fmt_src_path}/include
        /usr/include/kqueue/
)
