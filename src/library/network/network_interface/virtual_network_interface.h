#pragma once

#include "./network_interface_configuration.h"
#include "./network_interface_name.h"

#include <include/non_movable.h>
#include <include/non_copyable.h>

#include <library/network/poller/poller.h>
#include <library/network/socket/active_socket.h>
#include <library/network/socket/passive_socket.h>

#include <library/system.h>

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
            network_interface_configuration     networkInterfaceConfiguration_;
            poller::configuration               poller_;
            std::int64_t                        capacity_{default_capacity};
        };

        virtual_network_interface();

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

        tcp_listener_socket create_tcp_socket
        (
            tcp_listener_socket::configuration,
            tcp_listener_socket::event_handlers
        );

        tcp_socket accept_tcp_socket
        (
            system::file_descriptor,
            tcp_socket::configuration,
            tcp_socket::event_handlers
        );

        tcp_socket create_tcp_socket
        (
            socket_address,
            tcp_socket::configuration,
            tcp_socket::event_handlers
        );

        udp_socket create_udp_socket
        (
            port_id,
            udp_socket::configuration,
            udp_socket::event_handlers
        );

        udp_socket create_udp_socket
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

        network_interface_configuration                         networkInterfaceConfiguration_;
        std::shared_ptr<poller>                                 poller_;
        std::unique_ptr<work_contract_group>                sendWorkContractGroup_;
        std::unique_ptr<work_contract_group>                receiveWorkContractGroup_;

        std::atomic<bool>                                       stopped_{true};
        
    }; // class virtual_network_interface

} // namespace bcpp::network
