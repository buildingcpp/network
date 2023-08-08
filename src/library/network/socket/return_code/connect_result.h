#pragma once

#include <cstdint>


namespace bcpp::network
{

    enum class connect_result : std::uint32_t
    {
        undefined               = 0,
        success                 = 1,
        invalid_file_descriptor = 2,
        already_connected       = 3,
        connect_error           = 4,
        in_progress             = 5,
        invalid_destination     = 6
    };

}