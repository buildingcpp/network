#pragma once

#include <string>
#include <span>


namespace bcpp::network
{

    class ip_address;


    class host_name
    {
    public:

        host_name() = default;

        host_name
        (
            std::span<char const>
        );

        operator ip_address() const;

    private:

        std::string value_;
    };

} // namespace bcpp::network
