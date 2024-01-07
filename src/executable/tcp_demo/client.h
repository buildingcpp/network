#pragma once

#include <library/network.h>

#include <include/non_movable.h>
#include <include/non_copyable.h>

#include <memory>
#include <thread>
#include <iostream>



//=============================================================================
struct client : 
    bcpp::non_movable, 
    bcpp::non_copyable
{

    client
    (
        bcpp::network::network_interface_name interfaceName,
        bcpp::network::socket_address serverSocketAddress
    ):
        networkInterface_({.physicalNetworkInterfaceName_ = interfaceName}),
        socket_(networkInterface_.tcp_connect(
                serverSocketAddress, {}, 
                {
                    .receiveHandler_ = [&](auto, auto packet, auto){std::cout << std::string(packet.data(), packet.size()) << std::flush;
                }})),
        pollerThread_([this](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface_.poll();}),
        workerThread_([this](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface_.service_sockets();})
    {
        if (socket_.is_valid())
            std::cout << "client: established connection with server at " << socket_.get_peer_socket_address() << "\n";
    }

    void send(std::span<char const> data){socket_.send(data);}

    bcpp::network::virtual_network_interface    networkInterface_;
    bcpp::network::tcp_socket                   socket_;
    std::jthread                                pollerThread_;
    std::jthread                                workerThread_;
};
