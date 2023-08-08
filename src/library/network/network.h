#pragma once

#include "./network_interface/network_interface.h"

#include <string>


namespace bcpp::network
{

    ip_address get_ip_address_from_hostname
    (
        std::string
    );

}