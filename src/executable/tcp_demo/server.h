#pragma once

#include <library/network.h>

#include <include/non_movable.h>
#include <include/non_copyable.h>

#include <iostream>
#include <memory>
#include <map>


//=============================================================================
class server : bcpp::non_movable, bcpp::non_copyable
{
public:

    struct session;

    server
    (        
        bcpp::network::network_interface_configuration networkInterfaceConfiguration,
        bcpp::network::port_id portId
    ):
        networkInterface_({.networkInterfaceConfiguration_ = networkInterfaceConfiguration})
    {
        // we will use two threads for our example.  one will be responsible for polling the network interface
        // the other will be responsible for all of the async socket recv and tcp accepts
        // NOTE: in a non-demo environment we wouldn't want to create threads in this fashion for the purposes of servicing
        // a network interface.  It's done this way here for simplicity.
        // However, because sockets are destroyed asynchronously, and these threads will likely terminate very soon after 'this' is
        // destroyed, it is likely that the threads will not have time to complete the socket async delete prior to exiting.
        // But this is just a demo so ...
        pollerThread_ = std::jthread([this](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface_.poll();});
        workerThread_ = std::jthread([this](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface_.service_sockets();});
        // create a tcp listener socket
        socket_ = networkInterface_.create_tcp_socket(     // create a tcp listener socket
                {                    
                    .portId_ = portId,               
                    .backlog_ = 8,                   // configuration of this socket (or use the defaults)
                },
                {
                    .acceptHandler_ = [this]        // set event handlers
                            (
                                auto socketId,      // id of the socket making the callback (the listener socket)
                                auto fileDescriptor // the file descriptor of the newly accepted socket
                            )
                            {
                                on_accept_socket(std::move(fileDescriptor));
                            }
                });
    }

    auto get_ip_address() const{return socket_.get_ip_address();}

    void on_accept_socket
    (
        auto fileDescriptor // the file descriptor of the newly accepted socket
    )
    {
        // create a new session object.  it will create the tcp socket using the newly accepted file descriptor
        auto s = std::make_unique<session>(networkInterface_, std::move(fileDescriptor), [this](auto const & s){on_session_close(s);});
        std::lock_guard lockGuard(mutex_);
        std::cout << "server: accepted client connection from " << s->get_peer_socket_address() << "\n";
        std::cout << "server: creating session " << s->get_id() << "\n";
        sessions_[s->get_id()] = std::move(s);
    }


    void on_session_close
    (
        session const & s
    )
    {
        std::lock_guard lockGuard(mutex_);
        if (auto iter = sessions_.find(s.get_id()); iter != sessions_.end())
        {
            std::cout << "session " << s.get_id() << " removed\n";
            sessions_.erase(iter);
        }
    }


    struct session : bcpp::non_movable, bcpp::non_copyable
    {
        session
        (
            bcpp::network::virtual_network_interface & networkInterface,
            bcpp::system::file_descriptor fileDescriptor,
            std::function<void(session const &)> endSessionHandler 
        ) : endSessionHandler_(endSessionHandler)
        {
            tcpSocket_ = networkInterface.accept_tcp_socket(   // create a new tcp socket instance
                    std::move(fileDescriptor),          // move the newly accepted file descriptor into it
                    bcpp::network::tcp_socket::configuration{}, // configure this new tcp socket
                    bcpp::network::tcp_socket::event_handlers{
                            .closeHandler_ = [this]          // configure the close handler
                                    (
                                        auto                // id of this socket
                                    )
                                    {
                                        std::cout << "server: socket closed\n";
                                        if (endSessionHandler_)
                                            std::exchange(endSessionHandler_, nullptr)(*this);
                                    },
                            .receiveHandler_ = [this]               // configure the packet receive handler
                                    (
                                        auto,                       // id of this new socket
                                        auto receivedPacket,        // the received packet
                                        auto sendersSocketAddress   // the socket address of the partner
                                    )
                                    {
                                        on_receive_packet(std::move(receivedPacket), sendersSocketAddress);
                                    }
                    });
        }

        ~session(){std::cout << "server: deleting session " << get_id() << "\n";}

        void on_receive_packet
        (
            auto receivedPacket,
            auto sendersSocketAddress
        )
        {
            std::cout << "server: received message from " << sendersSocketAddress <<
                    ", content of message is \"" << std::string_view((char const *)receivedPacket.data(), receivedPacket.size()) << "\"\n";
        }

        bcpp::network::socket_id get_id() const{return tcpSocket_.get_id();}
        bcpp::network::socket_address get_peer_socket_address() const{return tcpSocket_.get_peer_socket_address();}

        bcpp::network::tcp_socket               tcpSocket_;
        std::function<void(session const &)>    endSessionHandler_;
    };

    std::function<void(session const &)>                closeHandler_;
    bcpp::network::virtual_network_interface            networkInterface_;
    bcpp::network::tcp_listener_socket                  socket_;
    std::mutex                                          mutex_;
    std::map<bcpp::network::socket_id, std::unique_ptr<session>>       sessions_;
    std::jthread                                        pollerThread_;
    std::jthread                                        workerThread_;
};
