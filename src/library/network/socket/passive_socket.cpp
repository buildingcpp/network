#include "./passive_socket.h"
#include "./private/passive_socket_impl.h"


//=============================================================================
bcpp::network::passive_socket::socket
(
    socket_address socketAddress,
    configuration const & config,
    event_handlers const & eventHandlers,
    work_contract_group & workContractGroup,
    std::shared_ptr<poller> & p
) 
try
{
    impl_ = std::move(decltype(impl_)(new impl_type(
            socketAddress,
            {
                .backlog_ = config.backlog_
            }, 
            {
                eventHandlers.closeHandler_,
                eventHandlers.pollErrorHandler_,
                eventHandlers.acceptHandler_
            },
            workContractGroup, p), 
            [](auto * impl){impl->destroy();}));
}
catch (std::exception const & exception)
{
    std::cerr << "passive_socket ctor failure.  reason: " << exception.what() << "\n";
    impl_.reset();
}


//=============================================================================
bool bcpp::network::passive_socket::close
(
)
{
    return (impl_) ? impl_->close() : false;
}


//=============================================================================
bool bcpp::network::passive_socket::is_valid
(
) const noexcept
{
    return (impl_) ? impl_->is_valid() : false;
}


//=============================================================================
auto bcpp::network::passive_socket::get_socket_address
(
) const noexcept -> socket_address
{
    return (impl_) ? impl_->get_socket_address() : socket_address{};
}


//=============================================================================
auto bcpp::network::passive_socket::get_ip_address
(
) const noexcept -> ip_address
{
    return (impl_) ? impl_->get_ip_address() : ip_address{};
}


//=============================================================================
auto bcpp::network::passive_socket::get_id
(
) const -> socket_id
{
    return (impl_) ? impl_->get_id() : socket_id{};
}


//=============================================================================
std::optional<std::int32_t> bcpp::network::passive_socket::get_socket_option
(
    std::int32_t level,
    std::int32_t optionName
) const noexcept
{
    return (impl_) ? impl_->get_socket_option(level, optionName) : std::nullopt;
}


//=============================================================================
bool bcpp::network::socket<bcpp::network::tcp_listener_socket_traits>::set_socket_option
(
    std::int32_t level,
    std::int32_t optionName,
    std::int32_t value
) noexcept
{
    return (impl_) ? impl_->set_socket_option(level, optionName, value) : false;
}
