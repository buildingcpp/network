#include <library/network.h>
#include <iostream>
#include <string_view>


//=============================================================================
auto select_network_interface
(
    bool loopback
) -> bcpp::network::network_interface_name
{
    std::string networkInterfaceName;
    for (auto && [name, ipAddress, netmask] : bcpp::network::get_available_network_interfaces())
        if (ipAddress.is_valid() && (ipAddress.is_loop_back() == loopback))
            return name;
    return {};
}


//=============================================================================
int main
(
    int,
    char **
)
{
    using namespace std::chrono;

    // find a suitable network interface (a non loop back interface for this example)
    auto useLoopback = false;
    auto networkInterfaceName = select_network_interface(useLoopback);
    if (networkInterfaceName.empty())
    {
        std::cerr << "Failed to locate suitable network interface for test\n";
        return -1;
    }
    std::cout << "using network interface " << networkInterfaceName << "\n";

    if (bcpp::network::virtual_network_interface networkInterface({.physicalNetworkInterfaceName_ = networkInterfaceName}); networkInterface.is_valid())
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
        auto socket1 = networkInterface.create_udp_socket({/*default configuration for this demo*/}, 
                {.receiveHandler_ = [](auto socketId, auto packet, auto senderSocketAddress){std::cout << "udp socket [id = " << socketId << "] received message from " << senderSocketAddress <<
                ".  Message is \"" << std::string_view((char const *)packet.data(), packet.size()) << "\"\n";}});

        auto socket2 = networkInterface.create_udp_socket( 
                {},
                {
                    .receiveHandler_ = []
                            (
                                auto socketId, 
                                auto packet,
                                auto senderSocketAddress
                            )
                            {
                                std::cout << "udp socket [id = " << socketId << "] received message from " << senderSocketAddress << ".  Message is \"" << std::string_view((char const *)packet.data(), packet.size()) << "\"\n";
                            }
                });
        socket2.connect_to(socket1.get_socket_address());
        //auto packet1 = create_packet("guess what!");
        socket2.send(create_packet("guess what!"));
        //auto packet2 = create_packet("chicken butt!!");
        socket1.send_to(socket2.get_socket_address(), create_packet("chicken butt!!"));

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
