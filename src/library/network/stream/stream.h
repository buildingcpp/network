#pragma once

#include <include/non_copyable.h>
#include <include/non_movable.h>

#include <library/network/ip/socket_address.h>
#include <library/network/packet/packet.h>
#include <library/network/socket/active_socket.h>
#include <library/system.h>

#include <include/spsc_fixed_queue.h>

#include <deque>
#include <mutex>
#include <vector>
#include <span>


namespace bcpp::network
{

    class virtual_network_interface;


    template <socket_concept S>
    class stream :
        non_copyable,
        non_movable
    {
    public:

        using socket_type = S;
        static auto constexpr is_tcp = tcp_socket_concept<S>;
        static auto constexpr is_udp = udp_socket_concept<S>;

        struct configuration : socket_type::configuration
        {
            static auto constexpr default_send_capacity = (1 << 10);
            std::size_t sendCapacity_{default_send_capacity};
        };

        struct event_handlers
        {
            using receive_handler = std::function<void(stream const &, packet)>;
            using receive_error_handler = std::function<void(stream const &, std::int32_t)>;
            using close_handler = std::function<void(stream const &)>;
            using poll_error_handler = std::function<void(stream const &)>;
            using hang_up_handler = std::function<void(stream const &)>;
            using peer_hang_up_handler = std::function<void(stream const &)>;

            receive_handler             receiveHandler_;
            receive_error_handler       receiveErrorHandler_;
            close_handler               closeHandler_;
            poll_error_handler          pollErrorHandler_;
            hang_up_handler             hangUpHandler_;
            peer_hang_up_handler        peerHangUpHandler_;
        };

        stream
        (
            socket_address,
            configuration const &,
            event_handlers const &,
            virtual_network_interface *,
            system::blocking_work_contract_group &
        );
        
        void send
        (
            packet
        );

        connect_result connect_to
        (
            socket_address const &
        ) noexcept;

        bool close();

        bool is_valid() const noexcept;

        socket_address get_socket_address() const noexcept;

        bool is_connected() const noexcept;

        socket_address get_peer_socket_address() const noexcept;

    private:

        void send();

        spsc_fixed_queue<packet>        sendQueue_;

        socket_type                     socket_;
        system::blocking_work_contract  sendWorkContract_;
        std::deque<packet>              packets_;
        std::mutex mutable              mutex_;
    }; // class stream<>


    using tcp_stream = stream<tcp_socket>;
    using udp_stream = stream<udp_socket>;

} // bcpp::network


//=============================================================================
template <bcpp::network::socket_concept S>
inline void bcpp::network::stream<S>::send
(
)
{
    std::lock_guard lockGuard(mutex_);
    if (!packets_.empty())
    {
        auto const & packet = packets_.front();

        if constexpr (is_tcp)
        {
            // TODO: handle partial sends
            socket_.send(packet);
        }
        if constexpr (is_udp)
        {
            // TODO: handle send failure
            socket_.send(packet);
        }
        packets_.pop_front();
        if (!packets_.empty())
            sendWorkContract_.schedule();
    }
}

