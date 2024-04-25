#include "./network.h"

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>


//=============================================================================
auto bcpp::network::get_ip_address_from_hostname
(
    std::string hostname
) -> ip_address
{
    struct ::addrinfo * result = nullptr;
    ::getaddrinfo(hostname.c_str(), 0, 0, &result);
    for (struct addrinfo * cur = result; cur != nullptr; cur = cur->ai_next)
        if (cur->ai_addr->sa_family == AF_INET) 
            return ip_address(((struct sockaddr_in *)(cur->ai_addr))->sin_addr);
    return {};
}


//=============================================================================
auto bcpp::network::get_available_network_interfaces
(
) -> std::vector<network_interface_configuration>
{
    std::vector<network_interface_configuration> interfaces;

    bcpp::network::ip_address ipAddress;
    ::ifaddrs * interfaceAddress = nullptr;

    if (auto result = ::getifaddrs(&interfaceAddress); result == 0)
    {
        auto cur = interfaceAddress;
        while (cur != nullptr)
        {
            if (cur->ifa_addr->sa_family == AF_INET)
            {
                interfaces.push_back(
                        {
                            .name_ = cur->ifa_name,
                            .ipAddress_ = {((struct sockaddr_in *)(cur->ifa_addr))->sin_addr},
                            .netmask_ = {((struct sockaddr_in *)(cur->ifa_netmask))->sin_addr},
                            .up_ = (((cur->ifa_flags) & IFF_UP) == IFF_UP),
                            .loopback_ = (((cur->ifa_flags) & IFF_LOOPBACK) == IFF_LOOPBACK),
                            .broadcast_ = (((cur->ifa_flags) & IFF_BROADCAST) == IFF_BROADCAST),
                            .multicast_ = (((cur->ifa_flags) & IFF_MULTICAST) == IFF_MULTICAST),
                            .running_ = (((cur->ifa_flags) & IFF_RUNNING) == IFF_RUNNING)
                        });
            }
            cur = cur->ifa_next;
        }
    }

    if (interfaceAddress != nullptr)
        ::freeifaddrs(interfaceAddress);

    return interfaces;
}


//=============================================================================
auto bcpp::network::get_network_interface_configuration
(
    // locate the configuration for the physical network interface associated with
    // the specified network interface name.
    network_interface_name physicalNetworkInterfaceName
) -> network_interface_configuration
{
    for (auto const & networkInterfaceConfiguration : get_available_network_interfaces())
        if (networkInterfaceConfiguration.name_ == physicalNetworkInterfaceName)
            return networkInterfaceConfiguration;
    return {};
}
