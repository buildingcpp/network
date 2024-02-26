#include "./stream.h"
#include <library/network/network_interface/virtual_network_interface.h>


//=============================================================================
template <bcpp::network::network_transport_protocol T>
bcpp::network::stream<T>::stream
(
    socket_address remoteSocketAddress,
    configuration const & config,
    event_handlers const & eventHandlers,
    virtual_network_interface * virtualNetworkInterface,
    system::blocking_work_contract_group & workContractGroup
):
    sendQueue_(config.sendCapacity_),
    sendWorkContract_(workContractGroup.create_contract([this](){this->send();}))
{
    if constexpr (tcp_concept<T>)
    {
        socket_ = virtualNetworkInterface->create_tcp_socket(remoteSocketAddress,
                config,
                {
                    .closeHandler_ = [this, closeHandler = eventHandlers.closeHandler_](auto){if (closeHandler) closeHandler(*this);},
                    .pollErrorHandler_ = [this, pollErrorHandler = eventHandlers.pollErrorHandler_](auto){if (pollErrorHandler) pollErrorHandler(*this);},
                    .receiveHandler_ = [this, receiveHandler = eventHandlers.receiveHandler_](auto, auto packet, auto){if (receiveHandler) receiveHandler(*this, std::move(packet));},
                    .receiveErrorHandler_ = [this, receiveErrorHandler = eventHandlers.receiveErrorHandler_](auto, auto error){if (receiveErrorHandler) receiveErrorHandler(*this, error);},
                    //.packetAllocationHandler_;
                    .hangUpHandler_ = [this, hangUpHandler = eventHandlers.hangUpHandler_](auto){if (hangUpHandler) hangUpHandler(*this);},
                    .peerHangUpHandler_ = [this, peerHangUpHandler = eventHandlers.peerHangUpHandler_](auto){if (peerHangUpHandler) peerHangUpHandler(*this);}
                });
    }
    else
    {
        socket_ = virtualNetworkInterface->create_udp_socket(
                config,
                bcpp::network::udp_socket::event_handlers{
                    .closeHandler_ = [this, closeHandler = eventHandlers.closeHandler_](auto) mutable{if (closeHandler) closeHandler(*this);},
                    .pollErrorHandler_ = [this, pollErrorHandler = eventHandlers.pollErrorHandler_](auto){if (pollErrorHandler) pollErrorHandler(*this);},
                    .receiveHandler_ = [this, receiveHandler = eventHandlers.receiveHandler_](auto, auto packet, auto){if (receiveHandler) receiveHandler(*this, std::move(packet));},
                    .receiveErrorHandler_ = [this, receiveErrorHandler = eventHandlers.receiveErrorHandler_](auto, auto error){if (receiveErrorHandler) receiveErrorHandler(*this, error);},
                    //.packetAllocationHandler_;
                    .hangUpHandler_ = [this, hangUpHandler = eventHandlers.hangUpHandler_](auto){if (hangUpHandler) hangUpHandler(*this);},
                    .peerHangUpHandler_ = [this, peerHangUpHandler = eventHandlers.peerHangUpHandler_](auto){if (peerHangUpHandler) peerHangUpHandler(*this);}
                });
        socket_.connect_to(remoteSocketAddress);
    }
}


//=============================================================================
template <bcpp::network::network_transport_protocol T>
bool bcpp::network::stream<T>::send
(
    // TODO: write test to prove that packet is not moved if push fails
    packet && thePacket
)
{
    std::lock_guard lockGuard(mutex_);
    if (sendQueue_.push(std::move(thePacket)))
    {
        sendWorkContract_.schedule();
        return true;
    }
    return false;
}


//=============================================================================
template <bcpp::network::network_transport_protocol T>
auto bcpp::network::stream<T>::connect_to
(
    socket_address const & destination
) noexcept -> connect_result
{
    return socket_.connect_to(destination);
}


//=============================================================================
template <bcpp::network::network_transport_protocol T>
bool bcpp::network::stream<T>::close
(
)
{
    return socket_.close();
}


//=============================================================================
template <bcpp::network::network_transport_protocol T>
bool bcpp::network::stream<T>::is_valid
(
) const noexcept
{
    return socket_.is_valid();
}


//=============================================================================
template <bcpp::network::network_transport_protocol T>
auto bcpp::network::stream<T>::get_socket_address
(
) const noexcept -> socket_address
{
    return socket_.get_socket_address();
}


//=============================================================================
template <bcpp::network::network_transport_protocol T>
bool bcpp::network::stream<T>::is_connected
(
) const noexcept
{
    return socket_.is_connected();
}


//=============================================================================
template <bcpp::network::network_transport_protocol T>
auto bcpp::network::stream<T>::get_peer_socket_address
(
) const noexcept -> socket_address
{
    return socket_.get_peer_socket_address();
}


//=============================================================================
namespace bcpp::network
{
    template class stream<network_transport_protocol::tcp>;
    template class stream<network_transport_protocol::udp>;

}
