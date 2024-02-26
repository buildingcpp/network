#include <library/network.h>

#include <iostream>


//=============================================================================
int main
(
    int,
    char **
)
{
    std::cout << "create virtual network interface\n";
    bcpp::network::virtual_network_interface virtualNetworkInterface;
    if (!virtualNetworkInterface.is_valid())
    {
        std::cerr << "Failed to create virtual network interface\n";
        return -1;
    }
    std::cout << "\tcreate udp socket\n";
    auto udpSocket = virtualNetworkInterface.create_udp_socket({}, {});
    if (!udpSocket.is_valid())
    {
        std::cerr << "Failed to create connectionless udp socket\n";
        return -1;
    }
    std::cout << "success\n";
    return 0;
}
