#include <library/network.h>

#include <iostream>


//=============================================================================
int main
(
    int,
    char **
)
{
    // iterate over all available network interfaces and create a virtual network interface mapped to each
    auto availableNetworkInterfaces = bcpp::network::get_available_network_interfaces();
    if (availableNetworkInterfaces.empty())
    {
        std::cerr << "no network interfaces detected\n";
        return -1;
    }
    for (auto const & networkInterfaceConfiguration : availableNetworkInterfaces)
    {
        bcpp::network::virtual_network_interface virtualNetworkInterface({.networkInterfaceConfiguration_ = networkInterfaceConfiguration});
        if (!virtualNetworkInterface.is_valid())
        {
            std::cerr << "Failed to create virtual network interface " << networkInterfaceConfiguration.name_ << "\n";
            return -1;
        }
    }
    std::cout << "success\n";
    return 0;
}
