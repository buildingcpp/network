#pragma once

#include <cstdint>
#include <concepts>
#include <type_traits>


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
    concept udp_concept = (T == network_transport_protocol::udp);


    //=========================================================================
    // alias for tcp based sockets
    template <network_transport_protocol T>
    concept tcp_concept = (T == network_transport_protocol::tcp);

} // namespace bcpp::network
