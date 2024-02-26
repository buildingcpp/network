#pragma once

#include <include/non_copyable.h>


namespace bcpp::network
{

    struct send_token
    {
        void * value_{nullptr};
        operator bool() const{return (value_ != nullptr);}
    };

} // namespace bcpp::network
