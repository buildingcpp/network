#pragma once

#include <library/network.h>
#include <library/system.h>

#include <include/non_movable.h>
#include <include/non_copyable.h>

#include <thread>
#include <iostream>


//=============================================================================
struct echo_client : bcpp::non_movable, bcpp::non_copyable
{
    echo_client
    (        
        bcpp::network::network_interface_name networkInterfaceName,
        bcpp::network::socket_address serverAddress
    ):
        networkInterface_({.physicalNetworkInterfaceName_ = networkInterfaceName}),
        socket_(networkInterface_.tcp_connect(serverAddress, {}, 
            {.receiveHandler_ = [&](auto, auto packet, auto){std::cout << std::string(packet.data(), packet.size()) << std::flush;}})),
        pollerThread_([this](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface_.poll();}),
        workerThread_([this](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface_.service_sockets();}){}

    void send(std::span<char const> data){socket_.send(data);}


    bcpp::network::virtual_network_interface    networkInterface_;
    bcpp::network::tcp_socket                   socket_;
    std::jthread                                pollerThread_;
    std::jthread                                workerThread_;
};
