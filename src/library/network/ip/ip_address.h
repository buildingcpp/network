#pragma once

#include <include/endian.h>

#include <fmt/format.h>

#include <cstdint>
#include <string>
#include <span>
#include <string_view>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <iostream>


namespace bcpp::network
{

    class ip_address
    {
    public:

        using value_type = ::in_addr;

        ip_address() noexcept = default;
        ip_address(ip_address const &) noexcept = default;
        ip_address & operator = (ip_address const &) noexcept = default;
        ip_address(ip_address &&) noexcept = default;
        ip_address & operator = (ip_address &&) noexcept = default;

        template <std::size_t N>
        ip_address
        (
            char const (&)[N]
        ) noexcept;

        ip_address  
        (
            std::span<char const> 
        ) noexcept;

        explicit ip_address  
        (
            std::string
        ) noexcept;

        constexpr ip_address
        (
            ::in_addr
        ) noexcept;

        constexpr bool is_valid() const noexcept;

        constexpr operator ::in_addr() const noexcept;

        constexpr bool is_loop_back() const noexcept;

        constexpr bool is_broadcast() const noexcept;

        constexpr bool is_multicast() const noexcept;

        constexpr auto operator ==
        (
            ip_address const &
        ) const;
     
    private:

        value_type value_{.s_addr = INADDR_NONE};
    };


    //=========================================================================
    static std::string to_string
    (
        ip_address ipAddress
    )
    {
        ::in_addr value = ipAddress;
        auto p = reinterpret_cast<std::uint8_t const *>(&value.s_addr);
        return fmt::format("{}.{}.{}.{}", p[0], p[1], p[2], p[3]);       
    }

} // namespace bcpp::network

#include "./host_name.h"


//=============================================================================
static std::ostream & operator << 
(
    std::ostream & stream,
    bcpp::network::ip_address const & ipAddress
)
{
    stream << to_string(ipAddress);
    return stream;
}


//=============================================================================
inline constexpr bcpp::network::ip_address::ip_address
(
    ::in_addr inAddr
) noexcept :
    value_(inAddr.s_addr)
{
}


//=============================================================================
inline bcpp::network::ip_address::ip_address  
(
    std::span<char const> value
) noexcept :
    ip_address(::in_addr{::inet_addr(value.data())})
{
    if (!is_valid())
        *this = host_name(value);
}


//=============================================================================
template <std::size_t N>
inline bcpp::network::ip_address::ip_address
( 
    char const (&value)[N]
) noexcept:
    ip_address(::in_addr{::inet_addr(value)})
{
    if (!is_valid())
        *this = host_name(value);
}


//=============================================================================
inline bcpp::network::ip_address::ip_address  
(
    std::string value
) noexcept :
    ip_address(::in_addr{::inet_addr(value.c_str())})
{
    if (!is_valid())
        *this = host_name(value);
}


//=============================================================================
inline constexpr bool bcpp::network::ip_address::is_multicast
(
) const noexcept
{
    auto constexpr mask = 0x000000e0ul;
    return ((value_.s_addr & mask) == mask);  
}


//=============================================================================
inline constexpr bool bcpp::network::ip_address::is_loop_back
(
) const noexcept
{
    auto constexpr loopback = byte_swap(INADDR_LOOPBACK);
    return (value_.s_addr == loopback);  
}


//=============================================================================
inline constexpr bool bcpp::network::ip_address::is_broadcast
(
) const noexcept
{
    auto constexpr broadcast = byte_swap(INADDR_BROADCAST);
    return (value_.s_addr == broadcast);  
}


//=============================================================================
inline constexpr bool bcpp::network::ip_address::is_valid
(
) const noexcept
{
    auto constexpr none = byte_swap(INADDR_NONE);
    return (value_.s_addr != none);
}


//=============================================================================
inline constexpr bcpp::network::ip_address::operator ::in_addr
(
) const noexcept
{
    return value_;
}


//=============================================================================
inline constexpr auto bcpp::network::ip_address::operator ==
(
    ip_address const & other
) const
{
    return (value_.s_addr == other.value_.s_addr);
}


//=============================================================================
namespace bcpp::network
{
    static ip_address constexpr local_host{::in_addr(byte_swap(INADDR_LOOPBACK))};
    static ip_address constexpr loop_back{::in_addr(byte_swap(INADDR_LOOPBACK))};
    static ip_address constexpr in_addr_loop_back{::in_addr(byte_swap(INADDR_LOOPBACK))};
    static ip_address constexpr in_addr_any{::in_addr(byte_swap(INADDR_ANY))};
    static ip_address constexpr in_addr_broadcast{::in_addr(byte_swap(INADDR_BROADCAST))};
    static ip_address constexpr in_addr_none{::in_addr(byte_swap(INADDR_NONE))};
}