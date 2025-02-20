#pragma once

#include "./socket.h"
#include "./traits/traits.h"

#include <include/file_descriptor.h>

#include <library/network/poller/poller.h>
#include <library/network/ip/socket_address.h>

#include <library/work_contract.h>
#include <library/system.h>

#include <functional>
#include <type_traits>
#include <span>
#include <optional>


namespace bcpp::network
{

    //=========================================================================
    template <>
    class socket<tcp_listener_socket_traits>
    {
    public:
        
        using traits = tcp_listener_socket_traits; 

        static auto constexpr default_backlog{128};

        struct event_handlers
        {
            using close_handler = std::function<void(socket_id)>;
            using poll_error_handler = std::function<void(socket_id)>;
            using accept_handler = std::function<void(socket_id, system::file_descriptor)>;

            close_handler           closeHandler_;
            poll_error_handler      pollErrorHandler_;
            accept_handler          acceptHandler_;
        };

        struct configuration
        {
            port_id         portId_;
            std::uint32_t   backlog_{default_backlog};
        };

        socket(socket const &) = delete;
        socket & operator = (socket const &) = delete;
        
        socket() = default;
        socket(socket &&) = default;
        socket & operator = (socket &&) = default;

        socket
        (
            socket_address,
            configuration const &,
            event_handlers const &,
            work_contract_group &,
            std::shared_ptr<poller> &
        );

        ~socket() = default;

        bool close();

        bool is_valid() const noexcept;

        socket_address get_socket_address() const noexcept;

        ip_address get_ip_address() const noexcept;

        socket_id get_id() const;
        
        std::optional<std::int32_t> get_socket_option
        (
            std::int32_t,
            std::int32_t
        ) const noexcept;

        bool set_socket_option
        (
            std::int32_t,
            std::int32_t,
            std::int32_t
        ) noexcept;

    private:

        using impl_type = socket_impl<traits>;

        std::unique_ptr<impl_type, std::function<void(impl_type *)>>   impl_;

    }; // class socket<tcp_listener_socket_traits>


    using passive_socket = socket<tcp_listener_socket_traits>;

    template <typename T>
    concept passive_socket_concept = socket_concept<T> && passive_socket_traits_concept<typename T::traits>;

    using tcp_listener_socket = passive_socket;

} // namespace bcpp::network
