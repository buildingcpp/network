#pragma once

#include "./socket_base_impl.h"

#include <library/network/socket/socket.h>
#include <library/network/poller/poller.h>
#include <library/network/packet/packet.h>

#include <include/io_mode.h>
#include <include/spsc_fixed_queue.h>
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
            using hang_up_handler = std::function<void(socket_id)>;
            using peer_hang_up_handler = std::function<void(socket_id)>;

            receive_handler             receiveHandler_;
            receive_error_handler       receiveErrorHandler_;
            packet_allocation_handler   packetAllocationHandler_;
            hang_up_handler             hangUpHandler_;
            peer_hang_up_handler        peerHangUpHandler_;
        };

        struct configuration
        {
            static auto constexpr default_send_queue_capacity = ((1 << 20));//((1 << 10) * 8);

            std::size_t     socketReceiveBufferSize_{0};
            std::size_t     socketSendBufferSize_{0};
            std::size_t     readBufferSize_{0};
            std::size_t     sendQueueSize_{default_send_queue_capacity};
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
            work_contract_tree_type &,
            work_contract_tree_type &,
            std::shared_ptr<poller> const &
        );

        socket_impl
        (
            system::file_descriptor,
            configuration const &,
            event_handlers const &,
            work_contract_tree_type &,
            work_contract_tree_type &,
            std::shared_ptr<poller> const &
        ) requires (tcp_concept<P>);

        virtual ~socket_impl() = default;

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
            socket_address const &
        ) noexcept;

        void receive() requires (udp_concept<P>);

        void receive() requires (tcp_concept<P>);

        void destroy();

        bool is_connected() const noexcept;

        socket_address get_peer_socket_address() const noexcept;

        connect_result join
        (
            ip_address
        ) requires (udp_concept<P>);

    private:

        socket_address get_peer_name() const noexcept;

        bool disconnect();

        std::uint32_t get_bytes_available() const noexcept;

        void on_hang_up() override;

        void on_peer_hang_up() override;

        void execute_next_send();

        std::size_t                                         readBufferSize_;

        socket_address                                      peerSocketAddress_;

        std::weak_ptr<poller>                               poller_;

        typename event_handlers::receive_handler            receiveHandler_;

        typename event_handlers::receive_error_handler      receiveErrorHandler_;
        
        typename event_handlers::packet_allocation_handler  packetAllocationHandler_;

        event_handlers::hang_up_handler                     hangUpHandler_;

        event_handlers::peer_hang_up_handler                peerHangUpHandler_;

        struct send_info 
        {
            send_info() = default;
            send_info(packet p, send_completion_token sendCompletionToken, socket_address destination):
                packet_(std::move(p)), sendToken_(sendCompletionToken), destination_(destination){}
            send_info(send_info &&) = default;
            send_info & operator = (send_info &&) = default;

            packet                      packet_;
            send_completion_token       sendToken_;
            socket_address              destination_;
        };

        spsc_fixed_queue<send_info>                         sendQueue_;

        work_contract_type                                  sendContract_;

        packet                                              pendingReceivePacket_;

    }; // class socket_impl<socket_traits<P, socket_type::active>>


    template <network_transport_protocol P>
    using active_socket_impl = socket_impl<socket_traits<P, socket_type::active>>;


    using tcp_socket_impl = active_socket_impl<network_transport_protocol::tcp>;
    using udp_socket_impl = active_socket_impl<network_transport_protocol::udp>;

} // namespace bcpp::network
