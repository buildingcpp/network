#pragma once

#include <library/network/socket/socket_id.h>

#include <library/network/socket/return_code/connect_result.h>

#include <library/network/ip/socket_address.h>
#include <include/file_descriptor.h>
#include <include/io_mode.h>
#include <include/synchronization_mode.h>

#include <library/system.h>

#include <include/non_copyable.h>
#include <include/non_movable.h>

#include <functional>


namespace bcpp::network
{

    //=========================================================================
    class socket_base_impl :
        public non_copyable,
        public non_movable
    {
    public:

        struct event_handlers
        {
            using close_handler = std::function<void(socket_id)>;
            using poll_error_handler = std::function<void(socket_id)>;

            close_handler       closeHandler_;
            poll_error_handler  pollErrorHandler_;
        };

        struct configuration
        {
            system::io_mode ioMode_{system::io_mode::read_write};
        };

        socket_base_impl
        (
            configuration const &,
            event_handlers const &,
            system::file_descriptor,
            system::non_blocking_work_contract
        );

        socket_base_impl
        (
            socket_address,
            configuration const &,
            event_handlers const &,
            system::file_descriptor,
            system::non_blocking_work_contract
        );

        virtual ~socket_base_impl();

        bool close();

        bool is_valid() const noexcept;

        system::file_descriptor const & get_file_descriptor() const noexcept;
        
        socket_address get_socket_address() const noexcept;

        socket_id get_id() const noexcept;

        bool shutdown() noexcept;

        bool set_io_mode
        (
            system::io_mode
        ) noexcept;

    protected:

        // unfortunate
        friend class poller;

        bool set_synchronicity
        (
            system::synchronization_mode
        ) noexcept;

        bool shutdown
        (
            system::io_mode
        ) noexcept;

        void on_polled();

        void on_poll_error();

        template <typename T>
        bool set_socket_option
        (
            std::int32_t,
            std::int32_t,
            T
        ) noexcept;

        void bind
        (
            socket_address const &
        );

        socket_address get_socket_name() const noexcept;

        system::file_descriptor             fileDescriptor_;

        socket_address                      socketAddress_;

        socket_id                           id_;

        event_handlers::close_handler       closeHandler_;

        event_handlers::poll_error_handler  pollErrorHandler_;

        system::non_blocking_work_contract               workContract_;

    }; // class socket_base_impl

} // namespace bcpp::network


//=============================================================================
template <typename T>
bool bcpp::network::socket_base_impl::set_socket_option
(
    std::int32_t level,
    std::int32_t optionName,
    T optionValue
) noexcept
{
    return (::setsockopt(fileDescriptor_.get(), level, optionName, &optionValue, sizeof(optionValue)) == 0);
}
