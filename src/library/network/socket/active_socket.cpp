#include "./active_socket.h"
#include "./private/active_socket_impl.h"


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bcpp::network::active_socket<P>::socket
(
    socket_address socketAddress,
    configuration const & config,
    event_handlers const & eventHandlers,
    work_contract_group & sendWorkContractGroup,
    work_contract_group & receiveWorkContractGroup,
    std::shared_ptr<poller> & p
) requires (udp_concept<P>) 
try
{
    impl_ = std::move(decltype(impl_)(new impl_type(
            socketAddress, 
            {
                .socketReceiveBufferSize_ = config.socketReceiveBufferSize_,
                .socketSendBufferSize_ = config.socketSendBufferSize_,
                .readBufferSize_ = config.readBufferSize_,
                .ioMode_ = config.ioMode_
            },
            {
                eventHandlers.closeHandler_,
                eventHandlers.pollErrorHandler_,
                eventHandlers.receiveHandler_,
                eventHandlers.receiveErrorHandler_,
                eventHandlers.packetAllocationHandler_,
                eventHandlers.hangUpHandler_,
                eventHandlers.peerHangUpHandler_
            },
            sendWorkContractGroup, receiveWorkContractGroup, p), 
            [](auto * impl){impl->destroy();}));
}
catch (std::exception const & exception)
{
    std::cerr << "active_socket ctor failure.  reason: " << exception.what() << "\n";
    impl_.reset();
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bcpp::network::active_socket<P>::socket
(
    ip_address ipAddress,
    configuration const & config,
    event_handlers const & eventHandlers,
    work_contract_group & sendWorkContractGroup,
    work_contract_group & receiveWorkContractGroup,
    std::shared_ptr<poller> & p
) requires (tcp_concept<P>)
try 
{
    impl_ = std::move(decltype(impl_)(new impl_type(
            {ipAddress}, 
            {
                .socketReceiveBufferSize_ = config.socketReceiveBufferSize_,
                .socketSendBufferSize_ = config.socketSendBufferSize_,
                .readBufferSize_ = config.readBufferSize_,
                .ioMode_ = config.ioMode_
            },
            {
                eventHandlers.closeHandler_,
                eventHandlers.pollErrorHandler_,
                eventHandlers.receiveHandler_,
                eventHandlers.receiveErrorHandler_,
                eventHandlers.packetAllocationHandler_,
                eventHandlers.hangUpHandler_,
                eventHandlers.peerHangUpHandler_
            },
            sendWorkContractGroup, receiveWorkContractGroup, p), 
            [](auto * impl){impl->destroy();}));
}
catch (std::exception const & exception)
{
    std::cerr << "active_socket ctor failure.  reason: " << exception.what() << "\n";
    impl_.reset();
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bcpp::network::active_socket<P>::socket
(
    system::file_descriptor fileDescriptor,
    configuration const & config,
    event_handlers const & eventHandlers,
    work_contract_group & sendWorkContractGroup,
    work_contract_group & recevieWorkContractGroup,
    std::shared_ptr<poller> & p
) requires (tcp_concept<P>)
try 
{
    impl_ = std::move(decltype(impl_)(new impl_type(
            std::move(fileDescriptor), 
            {
                .socketReceiveBufferSize_ = config.socketReceiveBufferSize_,
                .socketSendBufferSize_ = config.socketSendBufferSize_,
                .readBufferSize_ = config.readBufferSize_,
                .ioMode_ = config.ioMode_
            },
            {
                eventHandlers.closeHandler_,
                eventHandlers.pollErrorHandler_,
                eventHandlers.receiveHandler_,
                eventHandlers.receiveErrorHandler_,
                eventHandlers.packetAllocationHandler_,
                eventHandlers.hangUpHandler_,
                eventHandlers.peerHangUpHandler_
            },
            sendWorkContractGroup, recevieWorkContractGroup, p), 
            [](auto * impl){impl->destroy();}));
}
catch (std::exception const & exception)
{
    std::cerr << "active_socket ctor failure.  reason: " << exception.what() << "\n";
    impl_.reset();
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
auto bcpp::network::active_socket<P>::connect_to
(
    socket_address destination
) noexcept -> connect_result
{
    return (impl_) ? impl_->connect_to(destination) : connect_result::connect_error;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
auto bcpp::network::active_socket<P>::join
(
    ip_address ipAddress
) -> connect_result 
requires (udp_concept<P>)
{
    return (impl_) ? impl_->join(ipAddress) : connect_result::connect_error;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket<P>::send
(
    packet && data
)
{
    return (impl_) ? impl_->send(std::move(data)) : false;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket<P>::send
(
    packet && data,
    send_completion_token sendCompletionToken
)
{
    return (impl_) ? impl_->send(std::move(data), sendCompletionToken) : false;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket<P>::send_to
(
    socket_address destinationSocketAddress,
    packet && data
)
requires (udp_concept<P>)
{
    return (impl_) ? impl_->send_to(destinationSocketAddress, std::move(data)) : false;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket<P>::send_to
(
    socket_address destinationSocketAddress,
    packet && data,
    send_completion_token sendCompletionToken
)
requires (udp_concept<P>)
{
    return (impl_) ? impl_->send_to(destinationSocketAddress, std::move(data), sendCompletionToken) : false;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket<P>::close
(
)
{
    return (impl_) ? impl_->close() : false;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket<P>::is_valid
(
) const noexcept
{
    return (impl_) ? impl_->is_valid() : false;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
auto bcpp::network::active_socket<P>::get_socket_address
(
) const noexcept -> socket_address
{
    return (impl_) ? impl_->get_socket_address() : socket_address{};
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
auto bcpp::network::active_socket<P>::get_ip_address
(
) const noexcept -> ip_address
{
    return (impl_) ? impl_->get_ip_address() : ip_address{};
}

//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket<P>::is_connected
(
) const noexcept
{
    return (impl_) ? impl_->is_connected() : false;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
auto bcpp::network::active_socket<P>::get_peer_socket_address
(
) const noexcept -> socket_address
{
    return (impl_) ? impl_->get_peer_socket_address() : socket_address{};
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
auto bcpp::network::active_socket<P>::get_id
(
) const -> socket_id
{
    return (impl_) ? impl_->get_id() : socket_id{};
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket<P>::shutdown
(
) noexcept
{
    return (impl_) ? impl_->shutdown() : false;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket<P>::set_read_only
(
) noexcept
{
    return set_io_mode(system::io_mode::read);
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket<P>::set_write_only
(
) noexcept
{
    return set_io_mode(system::io_mode::write);
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket<P>::set_read_write
(
) noexcept
{
    return set_io_mode(system::io_mode::read_write);
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket<P>::set_io_mode
(
    system::io_mode ioMode
) noexcept
{
    return (impl_) ? impl_->set_io_mode(ioMode) : false;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
std::optional<std::int32_t> bcpp::network::active_socket<P>::get_socket_option
(
    std::int32_t level,
    std::int32_t optionName
) const noexcept
{
    return (impl_) ? impl_->get_socket_option(level, optionName) : std::nullopt;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
template <typename V>
bool bcpp::network::active_socket<P>::set_socket_option
(
    std::int32_t level,
    std::int32_t optionName,
    std::int32_t value
) noexcept
{
    return (impl_) ? impl_->set_socket_option(level, optionName, value) : false;
}


//=============================================================================
namespace bcpp::network
{
    template class socket<tcp_socket_traits>;
    template class socket<udp_socket_traits>;
}
