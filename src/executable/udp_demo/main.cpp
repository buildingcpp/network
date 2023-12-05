#include <library/network.h>
#include <iostream>
#include <string_view>


//=============================================================================
int main
(
    int,
    char **
)
{
    using namespace std::chrono;

    if (bcpp::network::virtual_network_interface networkInterface({.physicalNetworkInterfaceName_ = "lo"}); networkInterface.is_valid())
    {
        // set up threads
        bcpp::system::thread_pool threadPool
        ({
            {.function_ = [&](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface.poll();}},// one polling thread
            {.function_ = [&](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface.service_sockets(1s);}} // one socket reader thread
        });

        auto create_packet = []
                (
                    std::string_view message
                )
                {
                    bcpp::network::packet packet({[](auto const & p){delete [] p.data();}}, std::span(new char[message.size()], message.size()));
                    std::copy_n(message.data(), message.size(), packet.data());
                    return packet; 
                };

        // create a standard (connectionless) UDP socket
        auto socket1 = networkInterface.udp_connectionless({/*default configuration for this demo*/}, 
                {.receiveHandler_ = [](auto socketId, auto packet, auto senderSocketAddress){std::cout << "udp socket [id = " << socketId << "] received message from " << senderSocketAddress <<
                ".  Message is \"" << std::string_view((char const *)packet.data(), packet.size()) << "\"\n";}});

        auto udpStream = networkInterface.open_stream<bcpp::network::udp_socket>(socket1.get_socket_address(), 
                {},
                {
                    .receiveHandler_ = []
                            (
                                auto const & udpStream, 
                                auto packet
                            )
                            {
                                std::cout << "udp stream " << udpStream.get_socket_address() << " received message from " << udpStream.get_peer_socket_address() << ".  Message is \"" << std::string_view((char const *)packet.data(), packet.size()) << "\"\n";
                            }
                });
        udpStream.send(create_packet("guess what!"));
        socket1.send_to(udpStream.get_socket_address(), create_packet("chicken butt!!"));

        // since demo is async we wait for the messages to be processed
        std::this_thread::sleep_for(100ms);
    }
    else
    {
        std::cout << "network interface not found\n";
        return -1;
    }
    return 0;
}
