#pragma once

#include <library/network/socket/socket.h>
#include <library/network/polling/poller.h>
#include <library/network/packet/packet.h>

#include "./socket_base_impl.h"

#include <include/io_mode.h>
#include <library/system.h>

#include <functional>
#include <type_traits>
#include <span>
#include <tuple>
#include <cstdint>


namespace bcpp::network
{

    template <network_transport_protocol P>
    class socket_impl<socket_traits<P, socket_type::active>> :
        public socket_base_impl
    {
    public:

        using traits = socket_traits<P, socket_type::active>;

        struct event_handlers : socket_base_impl::event_handlers
        {
            using receive_handler = std::function<void(socket_id, packet, socket_address)>;
            using packet_allocation_handler = std::function<packet(socket_id, std::size_t)>;
            using receive_error_handler = std::function<void(socket_id, std::int32_t)>;

            receive_handler             receiveHandler_;
            receive_error_handler       receiveErrorHandler_;
            packet_allocation_handler   packetAllocationHandler_;
        };

        struct configuration
        {
            std::size_t     socketReceiveBufferSize_{0};
            std::size_t     socketSendBufferSize_{0};
            std::size_t     readBufferSize_{0};
            system::io_mode ioMode_{system::io_mode::read_write};

            // udp specific
            std::uint32_t   ttl_{0};
            std::uint32_t   multicastTtl_{0};
        };

        socket_impl
        (
            socket_address,
            configuration const &,
            event_handlers const &,
            system::work_contract_group &,
            poller &
        );

        socket_impl
        (
            system::file_descriptor,
            configuration const &,
            event_handlers const &,
            system::work_contract_group &,
            poller &
        ) requires (tcp_protocol_concept<P>);

        virtual ~socket_impl() = default;

        std::tuple<std::span<char const>, std::int32_t> send
        (
            std::span<char const>
        ) requires (tcp_protocol_concept<P>);

        std::tuple<std::span<char const>, std::int32_t> send
        (
            std::span<char const>
        ) requires (udp_protocol_concept<P>);

        std::tuple<std::span<char const>, std::int32_t> send_to
        (
            socket_address,
            std::span<char const>
        ) requires (udp_protocol_concept<P>);

        connect_result connect_to
        (
            socket_address const &
        ) noexcept;

        void receive() requires (udp_protocol_concept<P>);

        void receive() requires (tcp_protocol_concept<P>);

        void destroy();

        bool is_connected() const noexcept;

        socket_address get_peer_socket_address() const noexcept;

        connect_result join
        (
            ip_address
        ) requires (udp_protocol_concept<P>);

    private:

        socket_address get_peer_name() const noexcept;

        bool disconnect();

        std::size_t                                         readBufferSize_;

        socket_address                                      peerSocketAddress_;

        poller_registration                                 pollerRegistration_;

        typename event_handlers::receive_handler            receiveHandler_;

        typename event_handlers::receive_error_handler      receiveErrorHandler_;
        
        typename event_handlers::packet_allocation_handler  packetAllocationHandler_;

    }; // class socket_impl<socket_traits<P, socket_type::active>>


    template <network_transport_protocol P>
    using active_socket_impl = socket_impl<socket_traits<P, socket_type::active>>;


    using tcp_socket_impl = active_socket_impl<network_transport_protocol::tcp>;
    using udp_socket_impl = active_socket_impl<network_transport_protocol::udp>;

} // namespace bcpp::network
