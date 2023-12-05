#include "./stream.h"
#include <library/network/network_interface/virtual_network_interface.h>


//=============================================================================
template <bcpp::network::socket_concept S>
bcpp::network::stream<S>::stream
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
    if constexpr (tcp_socket_concept<S>)
    {
        socket_ = virtualNetworkInterface->tcp_connect(remoteSocketAddress,
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
        socket_ = virtualNetworkInterface->udp_connect(remoteSocketAddress,
                config,
                {
                    .closeHandler_ = [this, closeHandler = eventHandlers.closeHandler_](auto) mutable{if (closeHandler) closeHandler(*this);},
                    .pollErrorHandler_ = [this, pollErrorHandler = eventHandlers.pollErrorHandler_](auto){if (pollErrorHandler) pollErrorHandler(*this);},
                    .receiveHandler_ = [this, receiveHandler = eventHandlers.receiveHandler_](auto, auto packet, auto){if (receiveHandler) receiveHandler(*this, std::move(packet));},
                    .receiveErrorHandler_ = [this, receiveErrorHandler = eventHandlers.receiveErrorHandler_](auto, auto error){if (receiveErrorHandler) receiveErrorHandler(*this, error);},
                    //.packetAllocationHandler_;
                    .hangUpHandler_ = [this, hangUpHandler = eventHandlers.hangUpHandler_](auto){if (hangUpHandler) hangUpHandler(*this);},
                    .peerHangUpHandler_ = [this, peerHangUpHandler = eventHandlers.peerHangUpHandler_](auto){if (peerHangUpHandler) peerHangUpHandler(*this);}
                });
    }
}


//=============================================================================
template <bcpp::network::socket_concept S>
void bcpp::network::stream<S>::send
(
    packet b
)
{
    std::lock_guard lockGuard(mutex_);
    packets_.emplace_back(std::move(b));
    sendWorkContract_.schedule();
}


//=============================================================================
template <bcpp::network::socket_concept S>
auto bcpp::network::stream<S>::connect_to
(
    socket_address const & destination
) noexcept -> connect_result
{
    return socket_.connect_to(destination);
}


//=============================================================================
template <bcpp::network::socket_concept S>
bool bcpp::network::stream<S>::close
(
)
{
    return socket_.close();
}


//=============================================================================
template <bcpp::network::socket_concept S>
bool bcpp::network::stream<S>::is_valid
(
) const noexcept
{
    return socket_.is_valid();
}


//=============================================================================
template <bcpp::network::socket_concept S>
auto bcpp::network::stream<S>::get_socket_address
(
) const noexcept -> socket_address
{
    return socket_.get_socket_address();
}


//=============================================================================
template <bcpp::network::socket_concept S>
bool bcpp::network::stream<S>::is_connected
(
) const noexcept
{
    return socket_.is_connected();
}


//=============================================================================
template <bcpp::network::socket_concept S>
auto bcpp::network::stream<S>::get_peer_socket_address
(
) const noexcept -> socket_address
{
    return socket_.get_peer_socket_address();
}


//=============================================================================
namespace bcpp::network
{
    template class stream<tcp_socket>;
    template class stream<udp_socket>;

}
