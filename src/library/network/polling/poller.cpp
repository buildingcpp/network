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


namespace
{
    static bcpp::system::file_descriptor const invalid_file_descriptor;
}


//=============================================================================
bcpp::network::poller::poller
(
    configuration const & config
):
    fileDescriptor_(::epoll_create1(0)),
    trigger_(config.trigger_)
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
    std::array<::epoll_event, 1024> epollEvents;
    for (auto const & event : std::span(epollEvents.data(), ::epoll_wait(fileDescriptor_.get(), epollEvents.data(), epollEvents.size(), 0)))
    {
        auto impl = reinterpret_cast<socket_base_impl *>(event.data.ptr);
        if (event.events & EPOLLERR)
        {
            impl->on_poll_error();
            continue;
        }
        if (event.events & EPOLLIN)
            impl->on_polled();
    }   
}


//=============================================================================
template <bcpp::network::socket_impl_concept S>
auto bcpp::network::poller::register_socket
(
    // add socket to epoller
    S & socket
) -> poller_registration
{
    ::epoll_event epollEvent =
            {
                .events = (EPOLLIN | ((trigger_ == trigger_type::edge_triggered) ? EPOLLET : 0)),
                .data = {.ptr = reinterpret_cast<socket_base_impl *>(&socket)}
            };

    return {weak_from_this(), (::epoll_ctl(fileDescriptor_.get(), EPOLL_CTL_ADD, socket.get_file_descriptor().get(), &epollEvent) == 0) 
            ? socket.get_file_descriptor() : invalid_file_descriptor};
}


//=============================================================================
bool bcpp::network::poller::unregister_socket
(
    system::file_descriptor const & fileDescriptor
)
{
    return (::epoll_ctl(fileDescriptor_.get(), EPOLL_CTL_DEL, fileDescriptor.get(), nullptr) == 0);
}


//=============================================================================
namespace bcpp::network
{
    template poller_registration poller::register_socket(tcp_socket_impl &);
    template poller_registration poller::register_socket(udp_socket_impl &);
    template poller_registration poller::register_socket(tcp_listener_socket_impl &);
}
