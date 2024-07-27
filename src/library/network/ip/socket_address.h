#pragma once

#include "./ip_address.h"
#include "./port_id.h"

#include <algorithm>
#include <string>
#include <iostream>
#include <charconv>
#include <string_view>
#include <system_error>


namespace bcpp::network
{

    // TODO: abstract to include version (4 vs 6)

    class socket_address
    {
    public:

        socket_address() noexcept = default;
        socket_address(socket_address const &) noexcept = default;
        socket_address & operator = (socket_address const &) noexcept = default;
        socket_address(socket_address &&) noexcept = default;
        socket_address & operator = (socket_address &&) noexcept = default;

        template <std::size_t N>
        socket_address
        ( 
            char const (&)[N]
        );

        socket_address
        (
            std::span<char const> 
        );

        constexpr socket_address
        (
            ip_address,
            port_id
        ) noexcept;

        constexpr socket_address
        (
            ip_address
        ) noexcept;

        constexpr socket_address
        (
            ::sockaddr_in
        ) noexcept;

        ip_address get_ip_address() const noexcept;

        port_id get_port_id() const noexcept;
        
        operator ::sockaddr_in() const noexcept;

        bool is_valid() const noexcept;

        bool is_multicast() const noexcept;

    private:

        ip_address  ipAddress_{};

        port_id     portId_{};

    }; // class socket_address


    //=========================================================================
    static std::string to_string
    (
        socket_address socketAddress
    )
    {
        return to_string(socketAddress.get_ip_address()) + ':' + to_string(socketAddress.get_port_id());     
    }

} // namespace bcpp::network


//=============================================================================
template <std::size_t N>
inline bcpp::network::socket_address::socket_address
( 
    char const (&value)[N]
)
{
    auto begin = value;
    auto end = value + N;

    auto iter = std::find(begin, end, ':');
    ipAddress_ = std::span(begin, std::distance(begin, iter));
    portId_ = std::string((iter < end) ? iter + 1 : iter, end);
}


//=============================================================================
constexpr bcpp::network::socket_address::socket_address
(
    ip_address ipAddress,
    port_id portId
) noexcept :
    ipAddress_(ipAddress),
    portId_(portId)
{
}


//=============================================================================
constexpr bcpp::network::socket_address::socket_address
(
    ip_address ipAddress
) noexcept :
    ipAddress_(ipAddress)
{
}


//=============================================================================
constexpr bcpp::network::socket_address::socket_address
(
    ::sockaddr_in socketAddrIn
) noexcept :
    ipAddress_(socketAddrIn.sin_addr),
    portId_(endian_swap<std::endian::big, std::endian::native>(socketAddrIn.sin_port))
{
}  


//=============================================================================
[[maybe_unused]]
static std::ostream & operator << 
(
    std::ostream & stream,
    bcpp::network::socket_address const & socketAddress
)
{
    stream << to_string(socketAddress);
    return stream;
}
