#pragma once
 
#include <include/non_copyable.h>
#include <include/non_movable.h>
#include <include/bit.h>
#include <library/system.h>
 
#include <span>
#include <cstdint>
#include <atomic>
#include <vector>
  

namespace bcpp::network
{

    class buffer_heap final :
        non_copyable,
        non_movable
    {
    public:
 
        static auto constexpr default_heap_capacity = (1 << 16);
        static auto constexpr buffer_capacity = ((1 << 10) * 2);
        using type = std::span<char>;
        using buffer_index = std::int64_t;
 
        struct configuration
        {
            std::uint64_t capacity_{default_heap_capacity};
            std::uint64_t mmapFlags_{MAP_PRIVATE};
        };

        buffer_heap
        (
            configuration const & 
        );

        ~buffer_heap() = default;
 
        type pop();
 
        void push
        (
            type
        );
 
        bool owns
        (
            type const &
        ) const;
 
    private:
 
        static auto constexpr invalid_buffer_index{-1};
 
        system::shared_memory                               allocation_;
        char *                                              allocationBegin_;
        char *                                              allocationEnd_;
 
        std::unique_ptr<buffer_index volatile []>           queue_;
 
        std::atomic<std::uint64_t>                          front_{0};
        std::atomic<std::uint64_t>                          back_{0};
        std::uint64_t                                       capacityMask_{0};
 
    }; // class buffer_heap
 
} // namespace bcpp::network
 
 
//=============================================================================
inline bcpp::network::buffer_heap::buffer_heap
(
    configuration const & config
)
{
    auto capacity = minimum_power_of_two(config.capacity_);
    capacityMask_ = (capacity - 1);
    allocation_ = allocation_.create({
            .path_ = "",
            .size_ = (buffer_capacity * capacity),
            .ioMode_ = system::io_mode::read_write,
            .mmapFlags_ = config.mmapFlags_,
            .unlinkPolicy_ = system::shared_memory::unlink_policy::on_attach},
            {});
    allocationBegin_ = reinterpret_cast<char *>(allocation_.data());
    allocationEnd_ = (allocationBegin_ != nullptr) ? (allocationBegin_ + capacity) : nullptr;
    queue_ = std::move(std::make_unique<buffer_index volatile []>(capacity));
    for (auto bufferIndex = 0ull; bufferIndex < capacity; ++bufferIndex)
        queue_[bufferIndex] = bufferIndex;
    back_ = std::distance(allocationBegin_, allocationEnd_);
}
 
 
//=============================================================================
inline auto bcpp::network::buffer_heap::pop
(
) -> type
{
    auto front = front_.load();
    while (front < back_)
    {
        if (front_.compare_exchange_strong(front, front + 1))
        {
            front &= capacityMask_;
            buffer_index bufferIndex = queue_[front];
            while (bufferIndex == invalid_buffer_index)
                bufferIndex = queue_[front];
            queue_[front] = invalid_buffer_index;
            return {allocationBegin_ + (bufferIndex * buffer_capacity), buffer_capacity};
        }
    }
    return {};
}
 
 
//=============================================================================
inline void bcpp::network::buffer_heap::push
(
    type allocation
)
{
    queue_[back_++ & capacityMask_] = (std::distance(allocationBegin_, allocation.data()) / buffer_capacity);
}
 
 
//=============================================================================
inline bool bcpp::network::buffer_heap::owns
(
    type const & allocation
) const
{
    return ((allocation.data() >= allocationBegin_) && (allocation.data() < allocationEnd_));
}