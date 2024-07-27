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
        // bcpp::network is 100% async.  "sent" packets are not immediately
        // sent and are, instead, queued for asynchronous sending.  Therefore
        // the contents of a packet must persist for an undefined duration after
        // the actual call to send().
        // However, to avoid mandating copying data into a packet in order to preserve
        // the data, a packet is effectively a span to actual data which exists 
        // elsewhere.  Packet's maintain a 'delete_handler' which is invoked when a
        // packet is destroyed.  At that time, the data to which the span refers can
        // be addressed from within that delete handler.
        //
        // In this example we will allocate the packet's actual memory from the heap
        // and we will deallocate that memory in the packet's delete handler.
    )
    {
        static std::atomic<bool> dataDeleted{false};
        auto deleteHandler = []
                (
                    auto const & packet
                )
                {
                    delete [] packet.data();
                    std::cout << "data has been deleted\n";
                    dataDeleted = true;
                };

        // create a packet
        auto packetSize = 64;
        bcpp::network::packet packet({.deleteHandler_ = deleteHandler}, std::span(new char[packetSize], packetSize));

        packet = {}; // destroy the packet - should invoke the delete handler
        if (!dataDeleted)
        {
            std::cerr << "Failed to delete packet data\n";
            return -1;
        }
        return 0;
    }

} // namespace example_1


//============================================================================
namespace example_2
{

    auto packet_allocator
    (
        // proof of concept: packets can have their underlying memory drawn from
        // an allocator for performance reasons.
    )
    {
        // a toy allocator
        static auto constexpr buffer_size = 2048;
        static auto constexpr allocator_capacity = 4;

        static std::vector<char> pool(allocator_capacity);
        static std::vector<std::int32_t> avail;
        for (auto i = 0; i < allocator_capacity; ++i)
            avail.push_back(i);
        
        auto allocate = [&]() -> std::span<char>
                {
                    std::int32_t bufferIndex = avail.back();
                    std::cout << "Allocating buffer " << bufferIndex << "\n";
                    avail.pop_back();
                    return {&pool[bufferIndex], buffer_size};
                };

        auto deleteHandler = []
                (
                    auto const & packet
                )
                {
                    auto bufferIndex = std::distance(pool.data(), packet.data());
                    std::cout << "Returing buffer " << bufferIndex << " to pool\n";
                    avail.push_back(bufferIndex);
                };

        std::vector<bcpp::network::packet> packets;
        while (!avail.empty())
            packets.push_back({{.deleteHandler_ = deleteHandler}, allocate()});

        packets.clear();
        if (avail.size() != allocator_capacity)
        {
            std::cerr << "Failed to deallocate packet buffer\n";
        }
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
