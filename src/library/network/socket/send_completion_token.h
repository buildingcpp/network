#pragma once

#include <include/non_copyable.h>

#include <functional>


namespace bcpp::network
{

    struct send_completion_token
    {
        std::function<void(void *)> callback_;
        void * value_{nullptr};
        void operator()(){if (callback_) callback_(value_); callback_ = nullptr;}
        operator bool() const{return (callback_ != nullptr);}
    };

} // namespace bcpp::network
