#pragma once

#include "./network_interface_name.h"
#include <library/network/ip/ip_address.h>


namespace bcpp::network
{

    struct network_interface_configuration
    {
        network_interface_name      name_;
        ip_address                  ipAddress_;
        ip_address                  netmask_;
        bool                        up_;
        bool                        loopback_;
        bool                        broadcast_;
        bool                        multicast_;
        bool                        running_;                 
    };

}