#pragma once

#include "./network_interface_name.h"
#include <library/network/ip/ip_address.h>


namespace bcpp::network
{

    struct network_interface_info
    {
        network_interface_name name_;
        ip_address             ipAddress_;
        ip_address             netmask_;
    };

}