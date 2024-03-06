#if defined(USE_KQUEUE)

#pragma once

#include "./poller.h"

#include <include/non_copyable.h>
#include <include/non_movable.h>
#include <include/file_descriptor.h>
#include <library/network/socket/socket.h>

#include <vector>
#include <memory>
#include <chrono>
#include <array>

#include <sys/event.h>


namespace bcpp::network
{

    class poller :
        public std::enable_shared_from_this<poller>,
        non_copyable,
        non_movable
    {
    public:

        struct configuration
        {
        };

        static std::shared_ptr<poller> create
        (
            configuration const &
        );

        ~poller();

        template <concept::socket_impl S>
        bool register_socket
        (
            S &
        );

        template <concept::socket_impl S>
        bool unregister_socket
        (
            S &
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

        system::file_descriptor             fileDescriptor_;

    }; // class poller

} // namespace bcpp::network

#endif
