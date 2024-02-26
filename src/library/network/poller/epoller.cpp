#if !defined(USE_KQUEUE)

#include "./poller.h"

#include <library/network/socket/private/active_socket_impl.h>
#include <library/network/socket/private/passive_socket_impl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/epoll.h>

#include <iostream>
#include <span>
#include <array>

#include <sys/types.h>
#include <sys/time.h>


namespace
{
    static bcpp::system::file_descriptor const invalid_file_descriptor;
}


//=============================================================================
auto bcpp::network::poller::create
(
    configuration const & config
) -> std::shared_ptr<poller> 
{
    return std::shared_ptr<poller>(new poller(config));
}


//=============================================================================
bcpp::network::poller::poller
(
    configuration const & config
):
    fileDescriptor_(::epoll_create1(0))
{
}


//=============================================================================
bcpp::network::poller::~poller
(
)
{
    close();
}


//=============================================================================
void bcpp::network::poller::close
(
)
{
    fileDescriptor_ = {};
}


//=============================================================================
void bcpp::network::poller::poll
(
)
{
    return poll(std::chrono::milliseconds(0)); 
}


//=============================================================================
void bcpp::network::poller::poll
(
    std::chrono::milliseconds duration
)
{
    static thread_local std::array<::epoll_event, 1024> epollEvents;
    for (auto const & event : std::span(epollEvents.data(), ::epoll_wait(fileDescriptor_.get(), epollEvents.data(), epollEvents.size(), duration.count())))
    {
        auto impl = reinterpret_cast<socket_base_impl *>(event.data.ptr);
        if (event.events & EPOLLERR)
        {
            impl->on_poll_error();
            continue;
        }

        if (event.events & EPOLLIN)
            impl->on_polled();

        if (event.events & EPOLLHUP)
            impl->on_hang_up();

        if (event.events & EPOLLRDHUP)
            impl->on_peer_hang_up();
    }   
}

#endif
