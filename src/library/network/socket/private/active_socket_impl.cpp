#include "./active_socket_impl.h"

#include <cstring>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>


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
    system::blocking_work_contract_group & sendWorkContractGroup,
    system::blocking_work_contract_group & receiveWorkContractGroup,
    std::shared_ptr<poller> const & p
) :
    socket_base_impl(socketAddress, {.ioMode_ = config.ioMode_}, eventHandlers, 
            (P == network_transport_protocol::udp) ? ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP) : ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP),
            receiveWorkContractGroup.create_contract([this](){this->receive();}, [this](){this->destroy();})),
    poller_(p),
    receiveHandler_(eventHandlers.receiveHandler_),
    receiveErrorHandler_(eventHandlers.receiveErrorHandler_),
    packetAllocationHandler_(eventHandlers.packetAllocationHandler_ ? 
            eventHandlers.packetAllocationHandler_ : 
            [](auto, auto size){return packet({.deleteHandler_ = [](auto const & p){delete [] p.data();}}, {new char[size], size});}),
    sendQueue_(config.sendQueueSize_ ? config.sendQueueSize_ : configuration::default_send_queue_capacity),
    sendContract_(sendWorkContractGroup.create_contract([this](){this->execute_next_send();}, [this](){this->destroy();}))
{
    p->register_socket(*this);
    if constexpr (tcp_concept<P>)
        readBufferSize_ = (config.readBufferSize_ != 0) ? std::min(config.readBufferSize_, max_tcp_read_buffer_size) : default_tcp_read_buffer_size;
    if constexpr (udp_concept<P>)
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
    system::blocking_work_contract_group & sendWorkContractGroup,
    system::blocking_work_contract_group & receiveWorkContractGroup,
    std::shared_ptr<poller> const & p
) requires (tcp_concept<P>) :
    socket_base_impl({.ioMode_ = config.ioMode_}, eventHandlers, std::move(fileDescriptor),
            receiveWorkContractGroup.create_contract([this](){this->receive();}, [this](){this->destroy();})),
    poller_(p),
    receiveHandler_(eventHandlers.receiveHandler_),
    receiveErrorHandler_(eventHandlers.receiveErrorHandler_),
    packetAllocationHandler_(eventHandlers.packetAllocationHandler_ ? 
            eventHandlers.packetAllocationHandler_ : 
            [](auto, auto size){return packet({.deleteHandler_ = [](auto const & p){delete [] p.data();}}, {new char[size], size});}),
    sendQueue_(config.sendQueueSize_ ? config.sendQueueSize_ : configuration::default_send_queue_capacity),
    sendContract_(sendWorkContractGroup.create_contract([this](){this->execute_next_send();}, [this](){this->destroy();}))
{
    p->register_socket(*this);
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
) -> connect_result requires (udp_concept<P>)
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
            mreq.imr_multiaddr = peerSocketAddress_.get_ip_address();
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
void bcpp::network::active_socket_impl<P>::on_hang_up
(
) 
{
    if (auto hangUpHandler = std::exchange(hangUpHandler_, nullptr); hangUpHandler != nullptr)
        hangUpHandler(id_);
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
void bcpp::network::active_socket_impl<P>::on_peer_hang_up
(
)
{
    if (auto peerHangUpHandler = std::exchange(peerHangUpHandler_, nullptr); peerHangUpHandler != nullptr)
        peerHangUpHandler(id_);
    close();
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket_impl<P>::send
(
    packet && data
)
{
    return send(std::move(data), {});
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket_impl<P>::send
(
    packet && data,
    send_completion_token sendCompletionToken
)
{
    if (auto queued = sendQueue_.emplace(std::move(data), sendCompletionToken, socket_address{}); queued)
    {
        sendContract_.schedule();
        return true;
    }
    return false;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket_impl<P>::send_to
(
    socket_address destination,
    packet && data
)
requires (udp_concept<P>) 
{
    return send_to(destination, std::move(data), {});
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
bool bcpp::network::active_socket_impl<P>::send_to
(
    socket_address destination,
    packet && data,
    send_completion_token sendCompletionToken
)
requires (udp_concept<P>) 
{
    if (auto queued = sendQueue_.emplace(std::move(data), sendCompletionToken, destination); queued)
    {
        sendContract_.schedule();
        return true;
    }
    return false;
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
void bcpp::network::active_socket_impl<P>::execute_next_send
(
) 
{ 
    if constexpr (udp_concept<P>)
    {
        auto & [packet, sendCompletionToken, destination] = sendQueue_.front();
        ::sockaddr_in sockAddr = destination;
        sockAddr.sin_family = AF_INET;
        auto p = destination.is_valid() ? reinterpret_cast<sockaddr const *>(&sockAddr) : nullptr;
        if (auto result = ::sendto(fileDescriptor_.get(), packet.data(), packet.size(), MSG_NOSIGNAL, p, (p == nullptr) ? 0 : sizeof(sockAddr)); result < 0)
        {
            if (result != EAGAIN)
            {
                // issue send error callback here
            }
        }
        else
        {
            sendCompletionToken();
            if (auto sizeAfterDiscard = sendQueue_.discard(); sizeAfterDiscard == 0)
                return; // no more data to send
        }
    }
    else
    {
        auto & [packet, sendCompletionToken, _] = sendQueue_.front();
        if (auto result = ::send(fileDescriptor_.get(), packet.data(), packet.size(), MSG_NOSIGNAL); result < 0)
        {
            if (result != EAGAIN)
            {
                // issue send error callback here
            }
        }
        else
        {
            packet.discard(result);
            if (packet.empty())
            {
                sendCompletionToken();
                if (auto sizeAfterDiscard = sendQueue_.discard(); sizeAfterDiscard == 0)
                    return; // no more data to send
            }
        }       
    }

    sendContract_.schedule();
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
std::uint32_t bcpp::network::active_socket_impl<P>::get_bytes_available
(
) const noexcept
{
    std::uint32_t available = 0;
    if (::ioctl(fileDescriptor_.get(), FIONREAD, &available) == 0)
        return available;
    return {};
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
void bcpp::network::active_socket_impl<P>::receive
(
) requires (tcp_concept<P>)
{
    packet buffer = packetAllocationHandler_(id_, readBufferSize_);
    if (auto bytesReceived = ::recv(fileDescriptor_.get(), buffer.data(), buffer.capacity(), 0); bytesReceived > 0)
    {
        buffer.resize(bytesReceived);
        receiveHandler_(id_, std::move(buffer), peerSocketAddress_);
        if (get_bytes_available() > 0)
            on_polled(); // there is more data so reschedule the work contract
        return;
    }

    if ((errno == EWOULDBLOCK) || (errno == EAGAIN))   
        return; // nothing to do

    if ((errno == ECONNRESET) || (errno == 0))
    {   // connection reset or graceful shutdown
        close(); 
        return;
    }

    // an actual error
    if (receiveErrorHandler_)
        receiveErrorHandler_(id_, errno);
}


//=============================================================================
template <bcpp::network::network_transport_protocol P>
void bcpp::network::active_socket_impl<P>::receive
(
) requires (udp_concept<P>)
{
    if (auto bytesAvailable = get_bytes_available(); bytesAvailable > 0)
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
        }
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
    if (receiveContract_.is_valid())
    {
        receiveContract_.release();
    }    
    else
    {
        if (sendContract_.is_valid())
        {
            sendContract_.release();
        }
        else
        {
            // remove this socket from the poller before deleting 
            // 'this' as the poller has a raw pointer to 'this'.
            disconnect();
            if (auto poller = poller_.lock(); poller)
                poller->unregister_socket(*this);
            delete this;
        }
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
