#pragma once

#include <include/non_copyable.h>
#include <library/system.h>
#include <include/spsc_fixed_queue.h>

#include <cstdint>
#include <functional>


namespace bcpp::network
{

    class packet_queue
        : non_copyable
    {
    public:

        static auto constexpr default_capacity = (1 << 8);
        using packet_handler = std::function<void(packet_queue const &, packet)>;

        struct configuration
        {
            std::size_t capacity_{default_capacity};
        };

        struct event_handlers
        {
            packet_handler  packetHandler_;
        };

        packet_queue
        (
            configuration const &,
            event_handlers const &,
            system::work_contract_group &
        );

        bool push
        (
            packet &
        );

    private:

        void process_next_packet();

        packet_handler              packetHandler_;

        spsc_fixed_queue<packet>    queue_;

        system::work_contract       workContract_;
    };

} // namespace bcpp::network


//=============================================================================
bcpp::network::packet_queue::packet_queue
(
    configuration const & configuration,
    event_handlers const & eventHandlers,
    system::work_contract_group & workContractGroup
):
    packetHandler_(eventHandlers.packetHandler_),
    queue_(configuration.capacity_),
    workContract_(workContractGroup.create_contract([this](){process_next_packet();}))
{
}


//=============================================================================
bool bcpp::network::packet_queue::push
(
    packet & p
)
{
    if (queue_.push(p))
    {
        workContract_.schedule();
        return true;
    }
    return false;
}


//=============================================================================
void bcpp::network::packet_queue::process_next_packet
(
)
{
    packetHandler_(*this, std::move(queue_.pop()));
    if (!queue_.empty())
        workContract_.schedule();
}
