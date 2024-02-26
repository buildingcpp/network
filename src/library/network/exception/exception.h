#pragma once

#include <cstdint>
#include <exception>


namespace bcpp::network
{

    enum class exception_type : std::uint32_t
    {
        undefined               = 0,
        bind                    = 1,
        socket_option           = 2,
        socket_configuration    = 3
    };


    template <exception_type T>
    class exception : public std::exception
    {
    public:
        static auto constexpr type = T;
        explicit exception(char const * what) noexcept :what_(what){}
        virtual ~exception() noexcept = default;
        virtual char const * what() const noexcept{return what_;}
    private:
        char const * what_;
    };


    using bind_exception = exception<exception_type::bind>;
    using socket_option_exception = exception<exception_type::socket_option>;
    using socket_configuration_exception = exception<exception_type::socket_configuration>;

} // namespace bcpp::network