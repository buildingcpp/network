#pragma once

#include "./socket.h"
#include "./send_completion_token.h"
#include "./traits/traits.h"
#include "./connect_result.h"

#include <library/system.h>
#include <library/work_contract.h>
#include <library/network/poller/poller.h>
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

    //=========================================================================
    template <network_transport_protocol P>
    class socket<active_socket_traits<P>>
    {
    public:

        using work_contract_tree_type = work_contract_tree<synchronization_mode::async>; // really we want blocking here but that implementation needs to be restored 
        using work_contract_type = work_contract<synchronization_mode::async>; // really we want blocking here but that implementation needs to be restored 

        using traits = active_socket_traits<P>; 

        struct event_handlers
        {
            using close_handler = std::function<void(socket_id)>;
            using poll_error_handler = std::function<void(socket_id)>;
            using hang_up_handler = std::function<void(socket_id)>;
            using peer_hang_up_handler = std::function<void(socket_id)>;
            using receive_handler = std::function<void(socket_id, packet, socket_address)>;
            using receive_error_handler = std::function<void(socket_id, std::int32_t)>;
            using packet_allocation_handler = std::function<packet(socket_id, std::size_t)>;

            close_handler               closeHandler_;
            poll_error_handler          pollErrorHandler_;
            receive_handler             receiveHandler_;
            receive_error_handler       receiveErrorHandler_;
            packet_allocation_handler   packetAllocationHandler_;
            hang_up_handler             hangUpHandler_;
            peer_hang_up_handler        peerHangUpHandler_;
        };

        struct configuration
        {
            std::size_t socketReceiveBufferSize_{0};
            std::size_t socketSendBufferSize_{0};
            std::size_t readBufferSize_{0};
            std::size_t sendQueueSize_{0};
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
            work_contract_tree_type &,
            work_contract_tree_type &,
            std::shared_ptr<poller> &
        ) requires (udp_concept<P>);

        socket
        (
            ip_address,
            configuration const &,
            event_handlers const &,
            work_contract_tree_type &,
            work_contract_tree_type &,
            std::shared_ptr<poller> &
        ) requires (tcp_concept<P>);

        socket
        (
            system::file_descriptor,
            configuration const &,
            event_handlers const &,
            work_contract_tree_type &,
            work_contract_tree_type &,
            std::shared_ptr<poller> &
        ) requires (tcp_concept<P>);

        ~socket() = default;

        bool send
        (
            packet &&
        );

        bool send
        (
            packet &&,
            send_completion_token
        );

        bool send_to
        (
            socket_address,
            packet &&
        ) requires (udp_concept<P>);

        bool send_to
        (
            socket_address,
            packet &&,
            send_completion_token
        ) requires (udp_concept<P>);

        connect_result connect_to
        (
            socket_address
        ) noexcept;

        bool close();

        bool is_valid() const noexcept;

        socket_address get_socket_address() const noexcept;

        ip_address get_ip_address() const noexcept;
        
        bool is_connected() const noexcept;

        socket_address get_peer_socket_address() const noexcept;

        socket_id get_id() const;
        
        connect_result join
        (
            ip_address
        ) requires (udp_concept<P>);

        bool shutdown() noexcept;

        system::file_descriptor const & get_file_descriptor() const;

        bool set_read_only() noexcept;

        bool set_write_only() noexcept;

        bool set_read_write() noexcept;

        bool set_io_mode
        (
            system::io_mode
        ) noexcept;
       
        template <typename V>
        std::int32_t get_socket_option
        (
            std::int32_t,
            std::int32_t,
            V &
        ) const noexcept;

        template <typename V>
        std::int32_t set_socket_option
        (
            std::int32_t,
            std::int32_t,
            V
        ) noexcept;

    private:

        using impl_type = socket_impl<traits>;

        std::unique_ptr<impl_type, std::function<void(impl_type *)>>   impl_;

    }; // class socket<active_socket_traits<P>>


    template <network_transport_protocol T>
    using active_socket = socket<active_socket_traits<T>>;

    template <typename T>
    concept active_socket_concept = socket_concept<T> && active_socket_traits_concept<typename T::traits>;

    using udp_socket = active_socket<network_transport_protocol::udp>;
    using tcp_socket = active_socket<network_transport_protocol::tcp>;

} // namespace bcpp::network
