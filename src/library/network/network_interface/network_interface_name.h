#pragma once

#include <string>
#include <string_view>
#include <span>


namespace bcpp::network
{

    class network_interface_name
    {
    public:

        network_interface_name() = default;

        network_interface_name
        (
            std::span<char const>
        );

        network_interface_name
        (
            std::string const &
        );

        network_interface_name
        (
            std::string_view const
        );

        network_interface_name
        (
            char const *
        );

        auto operator <=>
        (
            network_interface_name const &
        ) const = default;

        std::string const & get() const;

        bool empty() const;

    private:

        std::string value_;
    };

} // namespace bcpp::network


//=============================================================================
[[maybe_unused]]
static std::ostream & operator << 
(
    std::ostream & stream,
    bcpp::network::network_interface_name const & source
)
{
    stream << source.get();
    return stream;
}
