#include "./server.h"
#include "./client.h"

#include <iostream>
#include <thread>
#include <chrono>


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
    using namespace bcpp::network::literals;
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

    server myServer(networkInterfaceName, 3000_port);
    client myClient(networkInterfaceName, {myServer.get_ip_address(), 3000_port});

    myClient.send("this is the message");

    std::this_thread::sleep_for(100ms); // demo is async so give it a moment to complete
    return 0;
}
