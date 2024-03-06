#include "./server.h"
#include "./client.h"

#include <iostream>
#include <thread>
#include <chrono>


//=============================================================================
auto select_network_interface
(
    bool loopback
) -> bcpp::network::network_interface_configuration
{
    for (auto const & networkInterfaceConfiguration : bcpp::network::get_available_network_interfaces())
        if (networkInterfaceConfiguration.loopback_ == loopback)
            return networkInterfaceConfiguration;
    return {};
}


//=============================================================================
int main
(
    int,
    char **
)
{
    using namespace bcpp::network::literals;
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

    server myServer(networkInterfaceConfiguration, 3000_port);
    client myClient(networkInterfaceConfiguration, {myServer.get_ip_address(), 3000_port});

    myClient.send("this is the message");

    std::this_thread::sleep_for(100ms); // demo is async so give it a moment to complete
    return 0;
}
