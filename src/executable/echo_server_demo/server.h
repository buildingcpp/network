#pragma once

#include <library/network.h>
#include <library/system.h>

#include <include/non_movable.h>
#include <include/non_copyable.h>

#include <memory>
#include <vector>
#include <thread>


//=============================================================================
struct echo_server : bcpp::non_movable, bcpp::non_copyable
{
    echo_server
    (
        bcpp::network::network_interface_configuration networkInterfaceConfiguration,
        bcpp::network::port_id portId
    ):
        networkInterface_({.networkInterfaceConfiguration_ = networkInterfaceConfiguration}),
        socket_(networkInterface_.create_tcp_socket({.portId_ = portId},
                {.acceptHandler_ = [this](auto, auto fileDescriptor){sessions.push_back(std::make_unique<session>(networkInterface_, std::move(fileDescriptor)));}})),
        pollerThread_([this](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface_.poll();}),
        workerThread_([this](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface_.service_sockets();}){}

    auto get_ip_address() const{return socket_.get_ip_address();}

    struct session : non_movable, non_copyable
    {
        session(bcpp::network::virtual_network_interface & networkInterface, bcpp::system::file_descriptor fileDescriptor):
            tcpSocket_(networkInterface.accept_tcp_socket(std::move(fileDescriptor), {}, 
                    {.receiveHandler_ = [this](auto, auto packet, auto){tcpSocket_.send(std::move(packet));}})){}
        bcpp::network::tcp_socket tcpSocket_;
    };

    bcpp::network::virtual_network_interface    networkInterface_;
    bcpp::network::tcp_listener_socket          socket_;
    std::vector<std::unique_ptr<session>>       sessions;
    std::jthread                                pollerThread_;
    std::jthread                                workerThread_;
};