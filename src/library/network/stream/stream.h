#pragma once

#include <library/network/ip/socket_address.h>
#include <library/network/packet/packet.h>
#include <library/network/socket/active_socket.h>
#include <library/system.h>

#include <deque>
#include <mutex>
#include <vector>
#include <span>


namespace bcpp::network
{

    template <socket_concept S>
    class stream
    {
    public:

        using socket_type = S;
        static auto constexpr is_tcp = tcp_socket_concept<S>;
        static auto constexpr is_udp = udp_socket_concept<S>;

        struct packet_type
        {
            packet          packet_;
            socket_address  destination_;
        };

        stream
        (
            socket_type,
            system::work_contract_group &
        );

        void send
        (
            packet
        );

        void send_to
        (
            socket_address,
            packet
        ) requires (is_udp);

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

        socket_type                     socket_;
        system::work_contract           workContract_;
        std::deque<packet_type>         packets_;
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
        auto && [packet, destinationSocketAddress] = packets_.front();

        if constexpr (is_tcp)
        {
            // TODO: handle partial sends
            socket_.send(packet);
        }
        if constexpr (is_udp)
        {
            // TODO: handle send failure
            socket_.send_to(destinationSocketAddress, packet);
        }
        packets_.pop_front();
        if (!packets_.empty())
            workContract_.invoke();
    }
}

