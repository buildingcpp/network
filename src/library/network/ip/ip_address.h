#pragma once

#include <include/endian.h>

#include <fmt/format.h>

#include <cstdint>
#include <string>
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
            std::string const &
        ) noexcept;

        constexpr ip_address
        (
            ::in_addr
        ) noexcept;

        bool is_valid() const noexcept;

        operator ::in_addr() const noexcept;

        bool is_multicast() const noexcept;
        
    private:

        value_type value_{};
    };


    //=========================================================================
    static std::string to_string
    (
        ip_address ipAddress
    )
    {
        ::in_addr value = ipAddress;
        auto p = reinterpret_cast<std::uint8_t const *>(&value.s_addr);
        return fmt::format("{}.{}.{}.{}", p[3], p[2], p[1], p[0]);       
    }

} // namespace bcpp::network


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
constexpr bcpp::network::ip_address::ip_address
(
    ::in_addr inAddr
) noexcept :
    value_(inAddr.s_addr)
{
}


//=============================================================================
inline bcpp::network::ip_address::ip_address  
(
    std::string const & value
) noexcept :
    ip_address(::in_addr{::inet_addr(value.data())})
{
}


//=============================================================================
template <std::size_t N>
inline bcpp::network::ip_address::ip_address
( 
    char const (&value)[N]
) noexcept:
    ip_address(::in_addr{::inet_addr(value.data())})
{
}


//=============================================================================
inline bool bcpp::network::ip_address::is_multicast
(
) const noexcept
{
    static auto constexpr mask = 0x000000e0ul;
    return ((value_.s_addr & mask) == mask);  
}


//=============================================================================
inline bool bcpp::network::ip_address::is_valid
(
) const noexcept
{
    return (value_.s_addr != 0);
}


//=============================================================================
inline bcpp::network::ip_address::operator ::in_addr
(
) const noexcept
{
    return value_;
}


//=============================================================================
namespace bcpp::network
{
    static ip_address constexpr local_host{::in_addr(byte_swap(INADDR_LOOPBACK))};
    static ip_address constexpr loop_back{::in_addr(byte_swap(INADDR_LOOPBACK))};
    static ip_address constexpr in_addr_any{::in_addr(byte_swap(INADDR_ANY))};
}