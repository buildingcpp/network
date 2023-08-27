#include "./active_socket_impl.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>


namespace
{
    static auto constexpr max_tcp_read_buffer_size = ((1ul << 10) * 64);
    static auto constexpr default_tcp_read_buffer_size = ((1ul << 10) * 4);
    static auto constexpr default_udp_read_buffer_size = ((1ul << 10) * 2);
            
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bcpp::network::active_socket_impl<P>::socket_impl
(
    socket_address socketAddress,
    configuration const & config,
    event_handlers const & eventHandlers,
    system::non_blocking_work_contract_group & workContractGroup,
    poller & p
) :
    socket_base_impl(socketAddress, {.ioMode_ = config.ioMode_}, eventHandlers, 
            (P == network_transport_protocol::udp) ? ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP) : ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP),
            workContractGroup.create_contract([this](){this->receive();}, [this](){this->destroy();})),
    pollerRegistration_(p.register_socket(*this)),
    receiveHandler_(eventHandlers.receiveHandler_),
    receiveErrorHandler_(eventHandlers.receiveErrorHandler_),
    packetAllocationHandler_(eventHandlers.packetAllocationHandler_ ? 
            eventHandlers.packetAllocationHandler_ : 
            [](auto, auto size){return packet({.deleteHandler_ = [](auto const & p){delete [] p.data();}}, {new char[size], size});})
{
    if constexpr (tcp_protocol_concept<P>)
        readBufferSize_ = (config.readBufferSize_ != 0) ? std::min(config.readBufferSize_, max_tcp_read_buffer_size) : default_tcp_read_buffer_size;
    if constexpr (udp_protocol_concept<P>)
    {
        readBufferSize_ = default_udp_read_buffer_size;
        if (config.multicastTtl_)
            set_socket_option(IPPROTO_IP, IP_MULTICAST_TTL, config.multicastTtl_);
        if (config.ttl_)
            set_socket_option(IPPROTO_IP, IP_TTL, config.ttl_);
    }

    if (config.socketReceiveBufferSize_ > 0)
        set_socket_option(SOL_SOCKET, SO_RCVBUF, config.socketReceiveBufferSize_);
    if (config.socketSendBufferSize_ > 0)
        set_socket_option(SOL_SOCKET, SO_SNDBUF, config.socketSendBufferSize_);
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bcpp::network::active_socket_impl<P>::socket_impl
(
    // this ctor is for 'accepted' tcp sockets where the socket file
    // descriptor is created prior to the socket_impl
    system::file_descriptor fileDescriptor,
    configuration const & config,
    event_handlers const & eventHandlers,
    system::non_blocking_work_contract_group & workContractGroup,
    poller & p
) requires (tcp_protocol_concept<P>) :
    socket_base_impl({.ioMode_ = config.ioMode_}, eventHandlers, std::move(fileDescriptor),
            workContractGroup.create_contract([this](){this->receive();}, [this](){this->destroy();})),
    pollerRegistration_(p.register_socket(*this)),
    receiveHandler_(eventHandlers.receiveHandler_),
    receiveErrorHandler_(eventHandlers.receiveErrorHandler_),
    packetAllocationHandler_(eventHandlers.packetAllocationHandler_ ? 
            eventHandlers.packetAllocationHandler_ : 
            [](auto, auto size){return packet({.deleteHandler_ = [](auto const & p){delete [] p.data();}}, {new char[size], size});})
{
    readBufferSize_ = (config.readBufferSize_ != 0) ? std::min(config.readBufferSize_, max_tcp_read_buffer_size) : default_tcp_read_buffer_size;
    peerSocketAddress_ = get_peer_name();
    if (config.socketReceiveBufferSize_ > 0)
        set_socket_option(SOL_SOCKET, SO_RCVBUF, config.socketReceiveBufferSize_);
    if (config.socketSendBufferSize_ > 0)
        set_socket_option(SOL_SOCKET, SO_SNDBUF, config.socketSendBufferSize_);
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
auto bcpp::network::active_socket_impl<P>::connect_to
(
    socket_address const & destination
) noexcept -> connect_result
{
    if (!destination.is_valid())
        return connect_result::invalid_destination;

    if (!fileDescriptor_.is_valid())
        return connect_result::invalid_file_descriptor;

    if (is_connected())
        return connect_result::already_connected;

    ::sockaddr_in socketAddress = destination;
    socketAddress.sin_family = AF_INET;
    auto result = ::connect(fileDescriptor_.get(), (sockaddr const *)&socketAddress, sizeof(socketAddress));
    if ((result != 0) && (errno != EINPROGRESS))
        return connect_result::connect_error;
    peerSocketAddress_ = destination;
    return connect_result::success;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
auto bcpp::network::active_socket_impl<P>::join
(
    ip_address ipAddress
) -> connect_result requires (udp_protocol_concept<P>)
{
    if (!ipAddress.is_valid())
        return connect_result::invalid_destination;

    if (!fileDescriptor_.is_valid())
        return connect_result::invalid_file_descriptor;

    if (is_connected())
        return connect_result::already_connected;

    // join multicast
    ::ip_mreq mreq;
    ::memset(&mreq, 0x00, sizeof(mreq));
    mreq.imr_multiaddr = ipAddress;
    mreq.imr_interface = in_addr_any;
    if (!set_socket_option(IPPROTO_IP, IP_ADD_MEMBERSHIP, mreq))
    {
        // TODO: log failure
        return connect_result::connect_error;
    }
    peerSocketAddress_ = {ipAddress, port_id{0}};
    set_io_mode(system::io_mode::read);
    return connect_result::success;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket_impl<P>::disconnect
(
)
{
    if ((!is_connected()) || (!fileDescriptor_.is_valid()))
        return false;

    if constexpr (P == network_transport_protocol::udp)
    {
        if (peerSocketAddress_.is_multicast())
        {
            // drop multicast membership
            ::ip_mreq mreq;
            ::memset(&mreq, 0x00, sizeof(mreq));
            mreq.imr_multiaddr = peerSocketAddress_.get_network_id();
            mreq.imr_interface = in_addr_any;
            if (!set_socket_option(IPPROTO_IP, IP_DROP_MEMBERSHIP, mreq))
            {
                // TODO: log failure
                return false;
            }
            peerSocketAddress_ = {};
            return true;
        }
    }
    return true;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
auto bcpp::network::active_socket_impl<P>::send
(
    std::span<char const> source
) -> std::tuple<std::span<char const>, std::int32_t> 
requires (tcp_protocol_concept<P>) 
{
    while (!source.empty())
    {
        auto result = ::send(fileDescriptor_.get(), source.data(), source.size(), MSG_NOSIGNAL);
        if (result < 0)
        {
            if (result != EAGAIN)
                return {source, result};
        }
        else
        {
            source = source.subspan(result);
        }
    }
    return {source, 0};
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
auto bcpp::network::active_socket_impl<P>::send
(
    std::span<char const> source
) -> std::tuple<std::span<char const>, std::int32_t>
requires (udp_protocol_concept<P>) 
{
    return send_to({}, source);
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
auto bcpp::network::active_socket_impl<P>::send_to
(
    socket_address destinationSocketAddress,
    std::span<char const> source
) -> std::tuple<std::span<char const>, std::int32_t>
requires (udp_protocol_concept<P>) 
{
    ::sockaddr_in sockAddr = destinationSocketAddress;
    sockAddr.sin_family = AF_INET;
    auto p = destinationSocketAddress.is_valid() ? reinterpret_cast<sockaddr const *>(&sockAddr) : nullptr;
    while (true)
    {
        auto result = ::sendto(fileDescriptor_.get(), source.data(), source.size(), MSG_NOSIGNAL, p, (p == nullptr) ? 0 : sizeof(sockAddr));
        if (result < 0)
        {
            if (result != EAGAIN)
                return {source, result};
        }
        else
        {
            return {source.subspan(result), 0};
        }
    } 
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
void bcpp::network::active_socket_impl<P>::receive
(
) requires (tcp_protocol_concept<P>)
{
    packet buffer = packetAllocationHandler_(id_, readBufferSize_);
    if (auto bytesReceived = ::recv(fileDescriptor_.get(), buffer.data(), buffer.capacity(), 0); bytesReceived >= 0)
    {
        if (bytesReceived == 0)        
        {
            // graceful shutdown
            close();
            return;
        }
        buffer.resize(bytesReceived);
        receiveHandler_(id_, std::move(buffer), peerSocketAddress_);
        on_polled(); // there could be more ...
    }
    else
    {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK) && (receiveErrorHandler_))
            receiveErrorHandler_(id_, errno);
    }
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
void bcpp::network::active_socket_impl<P>::receive
(
) requires (udp_protocol_concept<P>)
{
    ::sockaddr_in sockAddrIn;
    ::socklen_t addressLength = sizeof(sockAddrIn);

    packet buffer = packetAllocationHandler_(id_, readBufferSize_);
    if (auto bytesReceived = ::recvfrom(fileDescriptor_.get(), buffer.data(), buffer.capacity(), 0, 
            reinterpret_cast<::sockaddr *>(&sockAddrIn), &addressLength); bytesReceived >= 0)
    {
        buffer.resize(bytesReceived);
        receiveHandler_(id_, std::move(buffer), sockAddrIn);
        on_polled(); // there could be more ...
    }
    else
    {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK) && (receiveErrorHandler_))
            receiveErrorHandler_(id_, errno);
        else
            ;// TODO: there was no reason to allocate the packet in the first place
            // when allocators are added this will be a trivial issue.
    }
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
void bcpp::network::active_socket_impl<P>::destroy
(
    // use the work contract to asynchronously delete 'this'.
    // doing it this way ensures that the work contract's primary
    // work can not be executed any longer just prior to deleting
    // this.  This allows the primary work contract function to 
    // use a raw 'this' 
)
{
    if (workContract_.is_valid())
    {
        workContract_.surrender();
    }
    else
    {
        // remove this socket from the poller before deleting 
        // 'this' as the poller has a raw pointer to 'this'.
        disconnect();
        pollerRegistration_.release();
        delete this;
    }
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket_impl<P>::is_connected
(
) const noexcept
{
    return (peerSocketAddress_.is_valid());
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
auto bcpp::network::active_socket_impl<P>::get_peer_socket_address
(
) const noexcept -> socket_address
{
    return peerSocketAddress_;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
auto bcpp::network::active_socket_impl<P>::get_peer_name
(
) const noexcept -> socket_address
{
    ::sockaddr_in socketAddress;
    socketAddress.sin_family = AF_INET;
    ::socklen_t sizeofSocketAddress(sizeof(socketAddress));
    if (::getpeername(fileDescriptor_.get(), (struct sockaddr *)&socketAddress, &sizeofSocketAddress) == 0)
        return {socketAddress};
    return {};
}


//=============================================================================
namespace bcpp::network
{
    template class socket_impl<tcp_socket_traits>;
    template class socket_impl<udp_socket_traits>;
}
