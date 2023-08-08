#pragma once

#include "./socket_id.h"

#include "./traits/traits.h"

#include <concepts>
#include <type_traits>


namespace bcpp::network
{

    //=========================================================================
    // concept for any socket
    template <socket_traits_concept T> class socket;

    template <typename T>
    concept socket_concept = std::is_same_v<T, socket<typename T::traits>>;


    //=========================================================================
    // concept for any socket implementation
    template <socket_traits_concept T> class socket_impl;

    template <typename T>
    concept socket_impl_concept = std::is_same_v<T, socket_impl<typename T::traits>>;

} // namespace bcpp::network
