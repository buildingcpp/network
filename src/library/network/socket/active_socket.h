#pragma once

#include "./socket.h"
#include "./traits/traits.h"
#include "./return_code/connect_result.h"

#include <library/system.h>
#include <library/network/ip/socket_address.h>
#include <library/network/packet/packet.h>

#include <include/file_descriptor.h>
#include <include/io_mode.h>
#include <library/system.h>

#include <functional>
#include <type_traits>
#include <span>
#include <tuple>
#include <cstdint>


namespace bcpp::network
{

    class poller;


    //=========================================================================
    template <network_transport_protocol P>
    class socket<active_socket_traits<P>>
    {
    public:

        using traits = active_socket_traits<P>; 

        struct event_handlers
        {
            using close_handler = std::function<void(socket_id)>;
            using poll_error_handler = std::function<void(socket_id)>;
            using receive_handler = std::function<void(socket_id, packet, socket_address)>;
            using receive_error_handler = std::function<void(socket_id, std::int32_t)>;
            using packet_allocation_handler = std::function<packet(socket_id, std::size_t)>;

            close_handler               closeHandler_;
            poll_error_handler          pollErrorHandler_;
            receive_handler             receiveHandler_;
            receive_error_handler       receiveErrorHandler_;
            packet_allocation_handler   packetAllocationHandler_;
        };

        struct configuration
        {
            std::size_t socketReceiveBufferSize_{0};
            std::size_t socketSendBufferSize_{0};
            std::size_t readBufferSize_{0};
            system::io_mode ioMode_{system::io_mode::read_write};
        };

        socket(socket const &) = delete;
        socket & operator = (socket const &) = delete;
        
        socket() = default;
        socket(socket &&) = default;
        socket & operator = (socket &&) = default;

        socket
        (
            socket_address,
            configuration const &,
            event_handlers const &,
            system::work_contract_group &,
            poller &
        ) requires (udp_protocol_concept<P>);

        socket
        (
            ip_address,
            configuration const &,
            event_handlers const &,
            system::work_contract_group &,
            poller &
        ) requires (tcp_protocol_concept<P>);

        socket
        (
            system::file_descriptor,
            configuration const &,
            event_handlers const &,
            system::work_contract_group &,
            poller &
        ) requires (tcp_protocol_concept<P>);

        ~socket() = default;

        std::tuple<std::span<char const>, std::int32_t> send
        (
            std::span<char const>
        );

        std::tuple<std::span<char const>, std::int32_t> send_to
        (
            socket_address,
            std::span<char const>
        ) requires (P == network_transport_protocol::udp);

        connect_result connect_to
        (
            socket_address
        ) noexcept;

        bool close();

        bool is_valid() const noexcept;

        socket_address get_socket_address() const noexcept;

        bool is_connected() const noexcept;

        socket_address get_peer_socket_address() const noexcept;

        socket_id get_id() const;
        
        connect_result join
        (
            ip_address
        ) requires (P == network_transport_protocol::udp);

        bool shutdown() noexcept;

        system::file_descriptor const & get_file_descriptor() const;

        bool set_read_only() noexcept;

        bool set_write_only() noexcept;

        bool set_read_write() noexcept;

        bool set_io_mode
        (
            system::io_mode
        ) noexcept;

    private:

        using impl_type = socket_impl<traits>;

        std::unique_ptr<impl_type, std::function<void(impl_type *)>>   impl_;

    }; // class socket<active_socket_traits<P>>


    template <network_transport_protocol T>
    using active_socket = socket<active_socket_traits<T>>;

    using udp_socket = active_socket<network_transport_protocol::udp>;
    using tcp_socket = active_socket<network_transport_protocol::tcp>;

} // namespace bcpp::network
