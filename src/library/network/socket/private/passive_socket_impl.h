#pragma once

#include <library/network/socket/socket.h>
#include <library/network/polling/poller.h>

#include "./socket_base_impl.h"

#include <library/system.h>

#include <functional>
#include <type_traits>
#include <span>


namespace bcpp::network
{

    template <>
    class socket_impl<tcp_listener_socket_traits> :
        public socket_base_impl
    {
    public:

        using traits = tcp_listener_socket_traits;

        struct event_handlers : socket_base_impl::event_handlers
        {
            using accept_handler = std::function<void(socket_id, system::file_descriptor)>;

            accept_handler acceptHandler_;
        };

        struct configuration
        {
            system::synchronization_mode synchronicityMode_{system::synchronization_mode::non_blocking};
            std::uint32_t backlog_;
            system::io_mode ioMode_{system::io_mode::read_write};
        };

        socket_impl
        (
            socket_address,
            configuration const &,
            event_handlers const &,
            system::non_blocking_work_contract_group &,
            poller &
        );

        void destroy();

    private:

        void accept();

        poller_registration    pollerRegistration_;

        typename event_handlers::accept_handler acceptHandler_;

    }; // namespace socket_impl<tcp_listener_socket_traits> 

    using passive_socket_impl = socket_impl<tcp_listener_socket_traits>;

    using tcp_listener_socket_impl = passive_socket_impl;

} // namespace bcpp::network
