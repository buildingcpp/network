#include "./network_interface_name.h"


//=============================================================================
bcpp::network::network_interface_name::network_interface_name
(
    std::span<char const> value
):
    value_(value.data(), value.size())
{
}


//=============================================================================
bcpp::network::network_interface_name::network_interface_name
(
    std::string const & value
):
    value_(value.data(), value.size())
{
}


//=============================================================================
bcpp::network::network_interface_name::network_interface_name
(
    std::string_view const value
):
    value_(value.data(), value.size())
{
}


//=============================================================================
bcpp::network::network_interface_name::network_interface_name
(
    char const * value
):
    value_(value)
{
}


//=============================================================================
auto bcpp::network::network_interface_name::get
(
) const -> std::string const &
{
    return value_;
}


//=============================================================================
bool bcpp::network::network_interface_name::empty
(
) const
{
    return value_.empty();
}