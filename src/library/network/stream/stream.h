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


    template <network_transport_protocol T>
    class stream :
        non_copyable,
        non_movable
    {
    public:

        struct configuration : active_socket<T>::configuration
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
        
        bool send
        (
            packet &&
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

        active_socket<T>                socket_;
        system::blocking_work_contract  sendWorkContract_;
        std::deque<packet>              packets_;
        std::mutex mutable              mutex_;
    }; // class stream<>


    using tcp_stream = stream<network_transport_protocol::tcp>;
    using udp_stream = stream<network_transport_protocol::udp>;

} // bcpp::network


//=============================================================================
template <bcpp::network::network_transport_protocol T>
inline void bcpp::network::stream<T>::send
(
)
{
    std::lock_guard lockGuard(mutex_);
    if (!sendQueue_.empty())
    {
        auto & packet = sendQueue_.front();

        if constexpr (tcp_concept<T>)
        {
            // TODO: write tests for partial send
            auto [unsentData, returnCode] = socket_.send(packet);
            auto bytesSent = (packet.size() - unsentData.size());
            packet.discard(bytesSent);                
            if (!packet.empty())
            {
                sendWorkContract_.schedule();
                return; // partial send
            }
        }
        if constexpr (udp_concept<T>)
        {
            // TODO: write test for send failure
            if (auto [_, returnCode] = socket_.send(packet); returnCode != 0)
            {
                // send failed
                sendWorkContract_.schedule();
                return;
            }
        }
        sendQueue_.pop();
        if (!sendQueue_.empty())
            sendWorkContract_.schedule();
    }
}

