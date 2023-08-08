#include "./socket_address.h"

#include <include/endian.h>
#include <netinet/ip.h>


//=============================================================================
bcpp::network::socket_address::socket_address
(
    std::string const & value
)
{
    auto iter = std::find(value.begin(), value.end(), ':');
    ipAddress_ = std::string(value.data(), std::distance(value.begin(), iter));
    portId_ = std::string((iter < value.end()) ? iter + 1 : iter, value.end());
}


//=============================================================================
bool bcpp::network::socket_address::is_valid
(
) const noexcept
{
    return (ipAddress_.is_valid() && portId_.is_valid());
}


//=============================================================================
bool bcpp::network::socket_address::is_multicast
(
) const noexcept
{
    return ipAddress_.is_multicast();
}


//=============================================================================
auto bcpp::network::socket_address::get_network_id
(
) const noexcept -> ip_address
{
    return ipAddress_;
}


//=============================================================================
auto bcpp::network::socket_address::get_port_id
(
) const noexcept -> port_id
{
    return portId_;
}


//=============================================================================
bcpp::network::socket_address::operator ::sockaddr_in
(
) const noexcept
{
    return {.sin_port = endian_swap<std::endian::native, std::endian::big>(portId_).get(), .sin_addr =  ipAddress_};
}