#pragma once

#include <include/non_movable.h>
#include <include/non_copyable.h>

#include <library/network/polling/poller.h>
#include <library/network/socket/active_socket.h>
#include <library/network/socket/passive_socket.h>
#include <library/network/stream/stream.h>

#include <library/system.h>

#include <mutex>
#include <condition_variable>
#include <chrono>


namespace bcpp::network
{

    class network_interface :
        non_movable,
        non_copyable
    {
    public:

        static auto constexpr default_capacity = (1 << 16);

        struct configuration
        {
            poller::configuration   poller_;
            std::int64_t            capacity_{default_capacity};
        };

        network_interface();

        network_interface
        (
            configuration const &
        );

        ~network_interface();

        tcp_listener_socket tcp_listen
        (
            socket_address,
            tcp_listener_socket::configuration,
            tcp_listener_socket::event_handlers
        );

        tcp_socket tcp_accept
        (
            system::file_descriptor,
            tcp_socket::configuration,
            tcp_socket::event_handlers
        );

        template <socket_concept P, typename T>
        P open_socket
        (
            T,
            typename P::configuration,
            typename P::event_handlers
        );

        tcp_socket tcp_connect
        (
            ip_address,
            socket_address,
            tcp_socket::configuration,
            tcp_socket::event_handlers
        );

        udp_socket udp_connect
        (
            socket_address,
            socket_address,
            udp_socket::configuration,
            udp_socket::event_handlers
        );

        udp_socket udp_connectionless
        (
            socket_address,
            udp_socket::configuration,
            udp_socket::event_handlers
        );

        udp_socket multicast_join
        (
            socket_address,
            udp_socket::configuration,
            udp_socket::event_handlers
        );

        template <socket_concept P, typename T, typename B = default_buffer_type>
        stream<P> open_stream
        (
            T && socketHandle,
            typename P::configuration config,
            typename P::event_handlers eventHandlers
        )
        {
            return stream<P, B>(open_socket<P>(socketHandle, config, eventHandlers), workContractGroup_);
        }

        void poll();

        void service_sockets();

        void stop();
        
    private:

        std::shared_ptr<poller>     poller_;
        system::work_contract_group workContractGroup_;

        std::mutex mutable          mutex_;
        std::condition_variable     conditionVariable_;

        std::atomic<bool>           stopped_{false};

    }; // class network_interface

} // namespace bcpp::network
