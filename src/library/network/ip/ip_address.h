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

        using value_type = std::uint32_t;

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

        explicit constexpr ip_address
        (
            value_type
        ) noexcept;

        explicit constexpr ip_address
        (
            ::in_addr
        ) noexcept;
        
        value_type get() const noexcept;

        bool is_valid() const noexcept;

        operator ::in_addr() const noexcept;

        bool is_multicast() const noexcept;
        
    private:

        value_type value_{};
    };


    //=========================================================================
    static ip_address byte_swap
    (
        ip_address source
    ) 
    {
        return ip_address(bcpp::byte_swap(source.get()));
    }


    //=========================================================================
    static std::string to_string
    (
        ip_address ipAddress
    )
    {
        auto value = ipAddress.get();
        auto p = reinterpret_cast<std::uint8_t const *>(&value);
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
    std::uint32_t value
) noexcept :
    value_(value)
{
}


//=============================================================================
constexpr bcpp::network::ip_address::ip_address
(
    ::in_addr inAddr
) noexcept :
    value_(endian_swap<std::endian::big, std::endian::native>(inAddr.s_addr))
{
}


//=============================================================================
inline bcpp::network::ip_address::ip_address  
(
    std::string const & value
) noexcept :
    ip_address(endian_swap<std::endian::big, std::endian::native>(::inet_addr(value.data())))
{
}


//=============================================================================
template <std::size_t N>
inline bcpp::network::ip_address::ip_address
( 
    char const (&value)[N]
) noexcept:
    ip_address(endian_swap<std::endian::big, std::endian::native>(::inet_addr(value)))
{
}


//=============================================================================
inline std::uint32_t bcpp::network::ip_address::get
(
) const noexcept
{
    return value_;
}


//=============================================================================
inline bool bcpp::network::ip_address::is_multicast
(
) const noexcept
{
    static auto constexpr mask = 0xe0000000ul;
    return ((value_ & mask) == mask);  
}


//=============================================================================
inline bool bcpp::network::ip_address::is_valid
(
) const noexcept
{
    return (value_ != 0);
}


//=============================================================================
inline bcpp::network::ip_address::operator ::in_addr
(
) const noexcept
{
    return {.s_addr = endian_swap<std::endian::native, std::endian::big>(value_)};
}


//=============================================================================
namespace bcpp::network
{
    static ip_address constexpr local_host{INADDR_LOOPBACK};
    static ip_address constexpr loop_back{INADDR_LOOPBACK};
    static ip_address constexpr in_addr_any{INADDR_ANY};
}