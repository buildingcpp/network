#if !defined(USE_KQUEUE)

#pragma once

#include "./poller.h"

#include <include/non_movable.h>
#include <include/non_copyable.h>
#include <include/file_descriptor.h>

#include <library/network/socket/socket.h>

#include <sys/epoll.h>

#include <vector>
#include <memory>
#include <chrono>


namespace bcpp::network
{

    class socket_base_impl;

    class poller :
        public std::enable_shared_from_this<poller>,
        non_copyable,
        non_movable
    {
    public:

        struct configuration{};

        static std::shared_ptr<poller> create
        (
            configuration const &
        );

        ~poller();

        bool register_socket
        (
            socket_impl_concept auto &
        );

        bool unregister_socket
        (
            socket_impl_concept auto &
        );

        void poll();

        void poll
        (
            std::chrono::milliseconds
        );
        
        void close();

    private:

        poller
        (
            configuration const &
        );

        system::file_descriptor fileDescriptor_;

    }; // class poller

} // namespace bcpp::network

#endif


//=============================================================================
inline bool bcpp::network::poller::register_socket
(
    // add socket to poller
    socket_impl_concept auto & socket
)
{
    ::epoll_event epollEvent =
            {
                .events = (EPOLLIN | EPOLLET),
                .data = {.ptr = reinterpret_cast<socket_base_impl *>(&socket)}
            };
    return (::epoll_ctl(fileDescriptor_.get(), EPOLL_CTL_ADD, socket.get_file_descriptor().get(), &epollEvent) == 0);
}


//=============================================================================
inline bool bcpp::network::poller::unregister_socket
(
    socket_impl_concept auto & socket
)
{
    return (::epoll_ctl(fileDescriptor_.get(), EPOLL_CTL_DEL, socket.get_file_descriptor().get(), nullptr) == 0);
}