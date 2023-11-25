#pragma once

#include "./poller_registration.h"
#include <include/non_copyable.h>
#include <include/file_descriptor.h>

#include <library/network/socket/socket.h>

#include <mutex>
#include <vector>
#include <memory>
#include <chrono>


namespace bcpp::network
{

    class poller :
        public std::enable_shared_from_this<poller>,
        non_copyable,
        non_movable
    {
    public:

        enum class trigger_type : std::uint32_t
        {
            edge_triggered,
            level_triggered
        };

        struct configuration
        {
            trigger_type trigger_{trigger_type::edge_triggered};
        };

        static std::shared_ptr<poller> create
        (
            configuration const &
        );

        ~poller();

        template <socket_impl_concept S>
        poller_registration register_socket
        (
            S &
        );

        bool unregister_socket
        (
            system::file_descriptor const &
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

        trigger_type            trigger_;

    }; // class poller

} // namespace bcpp::network
