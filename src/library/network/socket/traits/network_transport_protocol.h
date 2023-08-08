#pragma once

#include <cstdint>


namespace bcpp::network
{

    //=========================================================================
    // the protocol type (udp or tcp)
    enum class network_transport_protocol : std::uint32_t
    {
        undefined                       = 0,
        transmission_control_protocol   = 1,
        tcp                             = transmission_control_protocol,
        user_datagram_protocol          = 2,
        udp                             = user_datagram_protocol
    };


    //=========================================================================
    // alias for udp based sockets
    template <network_transport_protocol T>
    concept udp_protocol_concept = (T == network_transport_protocol::udp);


    //=========================================================================
    // alias for tcp based sockets
    template <network_transport_protocol T>
    concept tcp_protocol_concept = (T == network_transport_protocol::tcp);
    
} // namespace bcpp::network
