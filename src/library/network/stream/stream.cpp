#include "./stream.h"


//=============================================================================
template <bcpp::network::socket_concept S>
bcpp::network::stream<S>::stream
(
    socket_type socket,
    system::blocking_work_contract_group & workContractGroup
):
    socket_(std::move(socket)),
    workContract_(workContractGroup.create_contract([this](){this->send();}))
{
}


//=============================================================================
template <bcpp::network::socket_concept S>
void bcpp::network::stream<S>::send
(
    packet b
)
{
    std::lock_guard lockGuard(mutex_);
    packets_.emplace_back(std::move(b), socket_.get_peer_socket_address());
    workContract_.schedule();
}


//=============================================================================
template <bcpp::network::socket_concept S>
void bcpp::network::stream<S>::send_to
(
    socket_address socketAddress,
    packet b
) requires (is_udp)
{
    std::lock_guard lockGuard(mutex_);
    packets_.emplace_back(std::move(b), socketAddress);
    workContract_.schedule();
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
