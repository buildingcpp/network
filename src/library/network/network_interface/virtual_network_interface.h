#pragma once

#include "./network_interface_name.h"

#include <include/non_movable.h>
#include <include/non_copyable.h>

#include <library/network/polling/poller.h>
#include <library/network/socket/active_socket.h>
#include <library/network/socket/passive_socket.h>
#include <library/network/stream/stream.h>

#include <library/system.h>

#include <mutex>
#include <memory>
#include <chrono>


namespace bcpp::network
{

    class virtual_network_interface :
        non_copyable
    {
    public:

        static auto constexpr default_capacity = (1 << 16);

        struct configuration
        {
            network_interface_name  physicalNetworkInterfaceName_;
            poller::configuration   poller_;
            std::int64_t            capacity_{default_capacity};
        };

        virtual_network_interface() = default;

        virtual_network_interface
        (
            configuration const &
        );

        virtual_network_interface
        (
            virtual_network_interface &&
        );

        virtual_network_interface & operator =
        (
            virtual_network_interface &&
        );

        ~virtual_network_interface();

        tcp_listener_socket tcp_listen
        (
            port_id,
            tcp_listener_socket::configuration,
            tcp_listener_socket::event_handlers
        );

        tcp_socket tcp_accept
        (
            system::file_descriptor,
            tcp_socket::configuration,
            tcp_socket::event_handlers
        );

        tcp_socket tcp_connect
        (
            socket_address,
            tcp_socket::configuration,
            tcp_socket::event_handlers
        );

        udp_socket udp_connect
        (
            port_id,
            socket_address,
            udp_socket::configuration,
            udp_socket::event_handlers
        );

        udp_socket udp_connect
        (
            socket_address,
            udp_socket::configuration,
            udp_socket::event_handlers
        );

        udp_socket udp_connectionless
        (
            port_id,
            udp_socket::configuration,
            udp_socket::event_handlers
        );

        udp_socket udp_connectionless
        (
            udp_socket::configuration,
            udp_socket::event_handlers
        );

        udp_socket multicast_join
        (
            socket_address,
            udp_socket::configuration,
            udp_socket::event_handlers
        );

        template <socket_concept P>
        stream<P> open_stream
        (
            socket_address,
            typename stream<P>::configuration const &,
            typename stream<P>::event_handlers const & 
        );

        void poll();

        void poll
        (
            std::chrono::milliseconds
        );

        void service_sockets();

        void service_sockets
        (
            std::chrono::nanoseconds
        );

        void stop();

        bool is_loop_back() const;

        ip_address get_ip_address() const;

        network_interface_name const & get_name() const;

        bool is_valid() const;

    private:

        template <socket_concept P, typename T>
        P open_socket
        (
            T,
            typename P::configuration,
            typename P::event_handlers
        );

        network_interface_name                                  physicalNetworkInterfaceName_;
        ip_address                                              ipAddress_;

        std::shared_ptr<poller>                                 poller_;
        std::unique_ptr<system::blocking_work_contract_group>   workContractGroup_;

        std::atomic<bool>                                       stopped_{true};

        std::mutex mutable                                      mutex_;

    }; // class virtual_network_interface

} // namespace bcpp::network


//=============================================================================
template <bcpp::network::socket_concept P>
auto bcpp::network::virtual_network_interface::open_stream
(
    socket_address remoteSocketAddress,
    typename stream<P>::configuration const & config,
    typename stream<P>::event_handlers const & eventHandlers
) -> bcpp::network::stream<P>
{
    return stream<P>(remoteSocketAddress, config, eventHandlers, this, *workContractGroup_);
}
