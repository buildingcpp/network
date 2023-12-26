#if defined(USE_KQUEUE)

#include "./poller.h"

#include <library/network/socket/private/active_socket_impl.h>
#include <library/network/socket/private/passive_socket_impl.h>

#include <iostream>
#include <span>
#include <array>
#include <chrono>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/event.h>
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
    fileDescriptor_(kqueue())
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
    static thread_local std::array<struct kevent, 1024> events;
    struct timespec timeout{.tv_sec = 0, .tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count()};
    for (auto const & event : std::span(events.data(), kevent(fileDescriptor_.get(), nullptr, 0, events.data(), 32, &timeout)))
    {
        auto impl = reinterpret_cast<socket_base_impl *>(event.udata);
        if (event.flags & EV_ERROR)
        {
            impl->on_poll_error();
            continue;
        }
        impl->on_polled();
    }
}


//=============================================================================
template <bcpp::network::socket_impl_concept S>
bool bcpp::network::poller::register_socket
(
    S & socket
)
{
    struct kevent event;
    EV_SET(&event, socket.fileDescriptor_.get(), EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, reinterpret_cast<socket_base_impl *>(&socket));
    kevent(fileDescriptor_.get(), &event, 1, nullptr, 0, nullptr);
    return true;
}


//=============================================================================
template <bcpp::network::socket_impl_concept S>
bool bcpp::network::poller::unregister_socket
(
    S & socket
)
{
    struct kevent event;
    EV_SET(&event, socket.get_file_descriptor().get(), EVFILT_READ, EV_DELETE, 0, 0, nullptr);    
    kevent(fileDescriptor_.get(), &event, 1, nullptr, 0, nullptr);
    return true;
}


//=============================================================================
namespace bcpp::network
{
    template bool poller::register_socket(tcp_socket_impl &);
    template bool poller::register_socket(udp_socket_impl &);
    template bool poller::register_socket(tcp_listener_socket_impl &);

    template bool poller::unregister_socket(tcp_socket_impl &);
    template bool poller::unregister_socket(udp_socket_impl &);
    template bool poller::unregister_socket(tcp_listener_socket_impl &);
}

#endif
