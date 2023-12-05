#include "./host_name.h"

#include "./ip_address.h"

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include <ifaddrs.h>


//=============================================================================
bcpp::network::host_name::host_name
(
    std::span<char const> value
)
{
    value_.reserve(value.size() + 1);
    value_ = {value.data(), value.size()};
    value_ += '\0';
}


//=============================================================================
bcpp::network::host_name::operator ip_address
(
) const
{
    struct ::addrinfo * result = nullptr;
    ::getaddrinfo(value_.c_str(), 0, 0, &result);
    for (struct addrinfo * cur = result; cur != nullptr; cur = cur->ai_next)
        if (cur->ai_addr->sa_family == AF_INET) 
            return {((struct sockaddr_in *)(cur->ai_addr))->sin_addr};
    return {};
}
