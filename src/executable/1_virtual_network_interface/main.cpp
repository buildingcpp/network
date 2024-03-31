//=============================================================================
//
// These examples demonstrate:
//
// 1. attaining information on available physical network interfaces
// 2. creating a basic virtual network interface (not bound to any specific physical interface)
// 3. creating a virtual network interface associated with a specific physical network interface
//
//=============================================================================

#include <library/network.h>

#include <iostream>


//============================================================================
namespace example_1
{

    void list_physical_network_interfaces
    (
        // gather and list the physical network interfaces that are available on the host
        // bcpp::network::get_available_network_interfaces() can be used to gather information about each
        // available interface.
    )
    {
        auto availableNetworkInterfaces = bcpp::network::get_available_network_interfaces();
        std::cout << "There are " << availableNetworkInterfaces.size() << " available network interfaces:\n";
        for (auto const & availableNetworkInterface : availableNetworkInterfaces)
            std::cout << availableNetworkInterface.name_ << '\n';
    }

} // namespace example_1


//=============================================================================
namespace example_2
{

    void create_virtual_network_interface
    (
    )
    {
        bcpp::network::virtual_network_interface virtualNetworkInterface;
        if (virtualNetworkInterface.is_valid())
            std::cout << "Successfully created virtual network interface\n";
        else
            std::cout << "Failed to create virtual network interface\n";
    }

} // namespace example_2


//=============================================================================
namespace example_3
{

    void create_virtual_network_interface_from_name
    (
        bcpp::network::network_interface_name physicalNetworkInterfaceName
    )
    {
        for (auto const & networkInterfaceConfiguration : bcpp::network::get_available_network_interfaces())
        {
            if (networkInterfaceConfiguration.name_ == physicalNetworkInterfaceName)
            {
                bcpp::network::virtual_network_interface virtualNetworkInterface({.networkInterfaceConfiguration_ = networkInterfaceConfiguration});
                if (virtualNetworkInterface.is_valid())
                {
                    std::cout << "Successfully created virtual network interface from physical network interface \"" << physicalNetworkInterfaceName << "\"\n";
                    return;
                }
            }
        }
        std::cout << "Failed to create virtual network interface from physical network interface \"" << physicalNetworkInterfaceName << "\"\n";
    }

} // namespace example_3


//=============================================================================
int main
(
    int,
    char **
)
{
    // list the available physical network interfaces by name
    example_1::list_physical_network_interfaces();

    // create a virtual network interface mapping without specifying a physical network interface 
    example_2::create_virtual_network_interface();

    // create a virtual network interface mapping to the physical network interface (loopback) 
    example_3::create_virtual_network_interface_from_name("lo");

    return 0;
}
