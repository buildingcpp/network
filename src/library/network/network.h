#pragma once

#include "./network_interface/virtual_network_interface.h"

#include <string>
#include <vector>


namespace bcpp::network
{

    ip_address get_ip_address_from_hostname
    (
        std::string hostname
    );

    std::vector<network_interface_configuration> get_available_network_interfaces();

}