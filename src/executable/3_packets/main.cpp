//=============================================================================
//
// These examples demonstrate:
//
// 1. creating a basic packet
// 2. creating a packets from an allocator
//
// NOTES: 
//
//=============================================================================


#include <library/network.h>

#include <iostream>
#include <chrono>




//============================================================================
namespace example_1
{
    auto basic_packet
    (
    )
    {
        bcpp::network::packet packet(64);
        return 0;
    }

} // namespace example_1


//============================================================================
namespace example_2
{


    auto packet_allocator
    (
    )
    {
        bcpp::network::buffer_heap bufferHeap({});
        // create a packet
        bcpp::network::packet packet(bufferHeap);
        packet = {}; // destroy the packet
        return 0;
    }

} // namespace example_2


//=============================================================================
int main
(
    int,
    char **
)
{
    if (auto result = example_1::basic_packet(); result != 0)
    {
        std::cout << "example_1 failed\n";
        return -1;
    }

    if (auto result = example_2::packet_allocator(); result != 0)
    {
        std::cout << "example_2 failed\n";
        return -1;
    }

    return 0;
}
