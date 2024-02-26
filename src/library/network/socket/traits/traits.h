#pragma once

#include "./network_transport_protocol.h"
#include "./socket_type.h"

#include <concepts>
#include <type_traits>


namespace bcpp::network
{

    //=========================================================================
    // socket traits define two properties for the socket
    // 1: the protocol (udp or tcp)
    // 2: the type of socket (active or passive) 
    template <network_transport_protocol T0, socket_type T1>
    struct socket_traits
    {
        static auto constexpr protocol = T0;
        static auto constexpr type = T1;
    };


    //=========================================================================
    // aliases for the various valid socket traits
    using tcp_socket_traits = socket_traits<network_transport_protocol::tcp, socket_type::active>;
    using tcp_listener_socket_traits = socket_traits<network_transport_protocol::tcp, socket_type::passive>;
    using udp_socket_traits = socket_traits<network_transport_protocol::udp, socket_type::active>;

    //=========================================================================
    template <typename T>
    concept socket_traits_concept = std::is_same_v<T, network::socket_traits<T::protocol, T::type>>;

    //=========================================================================
    // concept and alias for 'active' socket types
    template <typename T>
    concept active_socket_traits_concept = std::is_same_v<T, network::socket_traits<T::protocol, socket_type::active>>;

    //=========================================================================
    // concept and alias for 'passive' socket type(s)
    template <typename T>
    concept passive_socket_traits_concept = std::is_same_v<T, network::socket_traits<T::protocol, socket_type::passive>>;



    template <network_transport_protocol T>
    using active_socket_traits = socket_traits<T, socket_type::active>;

    template <network_transport_protocol T>
    using passive_socket_traits = socket_traits<T, socket_type::passive>;


} // namespace bcpp::network
