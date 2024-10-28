#include <library/network.h>
#include <iostream>
#include <string_view>


//=============================================================================
auto select_network_interface
(
    bool loopback
) -> bcpp::network::network_interface_configuration
{
    for (auto const & networkInterfaceConfiguration : bcpp::network::get_available_network_interfaces())
    {
        if (!networkInterfaceConfiguration.ipAddress_.is_multicast())
        {
            if (networkInterfaceConfiguration.loopback_ == loopback)
                return networkInterfaceConfiguration;
        }
    }
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
    auto networkInterfaceConfiguration = select_network_interface(useLoopback);
    if (!networkInterfaceConfiguration.ipAddress_.is_valid())
    {
        std::cerr << "Failed to locate suitable network interface for test\n";
        return -1;
    }
    std::cout << "using network interface " << networkInterfaceConfiguration.name_ << "\n";

    if (bcpp::network::virtual_network_interface networkInterface({.networkInterfaceConfiguration_ = networkInterfaceConfiguration}); networkInterface.is_valid())
    {
        // set up threads
        bcpp::system::thread_pool threadPool
        ({
            {.function_ = [&](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface.poll();}},// one polling thread
            {.function_ = [&](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface.service_sockets(1s);}} // one socket reader thread
        });

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
        auto packet1 = bcpp::network::packet::create(64);
        packet1.set_content("guess what?");
        socket2.send(std::move(packet1));
        auto packet2 = bcpp::network::packet::create(64);
        packet2.set_content("chicken butt!!");
        socket1.send_to(socket2.get_socket_address(), std::move(packet2));

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
