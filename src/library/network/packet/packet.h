#pragma once

#include "./buffer_heap.h"
#include <include/non_copyable.h>

#include <span>
#include <cstdint>
#include <utility>


namespace bcpp::network
{

    class packet :
        non_copyable
    {
    public:

        using element_type = char;

        packet() = default;

        packet
        ( 
            std::size_t
        );

        packet
        ( 
            buffer_heap &
        );

        packet
        (
            std::span<element_type>
        );

        packet
        (
            packet &&
        );

        packet & operator = 
        (
            packet &&
        );

        ~packet();

        auto capacity() const;

        auto begin();

        auto begin() const;

        auto end();

        auto end() const;

        element_type const * data() const;

        element_type * data();

        auto size() const;

        bool empty() const;

        bool resize
        (
            std::size_t
        );

        std::size_t discard
        (
            std::size_t
        );

        operator std::span<element_type const>() const;

        operator bool() const;

    private:

        #pragma pack(push, 1)
        struct alignas(64) packet_header
        {
            buffer_heap * volatile heap_{nullptr};
        };
        #pragma pack(pop)

        void release();

        std::span<element_type>  buffer_;

        std::size_t              size_{0};

        std::size_t              begin_{0};

        bool                     ownsData_{false};

    };

} // namespace bcpp::network


//=============================================================================
inline bcpp::network::packet::packet
(
    // create a packet with process memory
    std::size_t capacity
):
    buffer_(new char[capacity + sizeof(packet_header)], capacity + sizeof(packet_header))
{
    if (!buffer_.empty())
    {
        size_ = 0;
        begin_ = sizeof(packet_header);
        ownsData_ = true;
        new (buffer_.data()) packet_header;
    }
}


//=============================================================================
inline bcpp::network::packet::packet
(
    // create a packet with allocator
    buffer_heap & bufferHeap
):
    buffer_(bufferHeap.pop())
{
    if (!buffer_.empty())
    {
        size_ = 0;
        begin_ = sizeof(packet_header);
        ownsData_ = true;
        new (buffer_.data()) packet_header(&bufferHeap);
    }
    else
    {
        auto capacity = buffer_heap::buffer_capacity;
        buffer_ = std::span<char>(new char[capacity], capacity);
        if (!buffer_.empty())
        {
            size_ = 0;
            begin_ = sizeof(packet_header);
            ownsData_ = true;
            new (buffer_.data()) packet_header;
        }
    }
}


//=============================================================================
inline bcpp::network::packet::packet
(
    std::span<element_type> buffer
):
    buffer_(buffer), 
    size_(buffer.size()),
    begin_(0),
    ownsData_(false)
{
}


//=============================================================================
inline bcpp::network::packet::packet
(
    packet && other
):
    buffer_(other.buffer_),
    size_(other.size_),
    begin_(other.begin_),
    ownsData_(other.ownsData_)
{
    other.size_ = {};
    other.buffer_ = {};
    other.begin_ = {};
    other.ownsData_ = {};
}


//=============================================================================
inline auto bcpp::network::packet::operator = 
(
    packet && other
) -> packet & 
{
    if (&other != this)
    {
        release();
        buffer_ = other.buffer_;
        size_ = other.size_;
        begin_ = other.begin_;
        ownsData_ = other.ownsData_;

        other.size_ = {};
        other.buffer_ = {};
        other.begin_ = {};
        other.ownsData_ = {};
    }
    return *this;
}


//=============================================================================
inline bcpp::network::packet::~packet
(
)
{
    release();
}


//=============================================================================
inline auto bcpp::network::packet::capacity
(
) const
{
    return (buffer_.size() - ((ownsData_) ? sizeof(packet_header) : 0));
}


//=============================================================================
inline auto bcpp::network::packet::begin
(
)
{
    return (buffer_.begin() + begin_);
}


//=============================================================================        
inline auto bcpp::network::packet::begin
(
) const
{
    return (buffer_.begin() + begin_);
}


//=============================================================================
inline auto bcpp::network::packet::end
(
)
{
    return begin() + size_;
}


//=============================================================================
inline auto bcpp::network::packet::end
(
) const
{
    return begin() + size_;
}


//=============================================================================
inline auto bcpp::network::packet::data
(
) const -> element_type const *
{
    return (buffer_.data() + ((ownsData_) ? sizeof(packet_header) : 0));
}


//=============================================================================
inline auto bcpp::network::packet::data
(
) -> element_type *
{
    return (buffer_.data() + ((ownsData_) ? sizeof(packet_header) : 0));
}


//=============================================================================
inline auto bcpp::network::packet::size
(
) const
{
    return size_;
}


//=============================================================================
inline bool bcpp::network::packet::empty
(
) const
{
    return (size_ == 0);
}


//=============================================================================
inline bool bcpp::network::packet::resize
(
    std::size_t size
)
{
    if (size > capacity()) 
        return false; 
    size_ = size; 
    return true;
}


//=============================================================================
inline std::size_t bcpp::network::packet::discard
(
    std::size_t sizeToDiscard
)
{
    if (sizeToDiscard > size_)
        sizeToDiscard = size_;
    size_ -= sizeToDiscard;
    begin_ += sizeToDiscard;
    return sizeToDiscard;
}


//=============================================================================
inline void bcpp::network::packet::release
(
)
{
    if (ownsData_ == true)
    {
        auto & packetHeader = *reinterpret_cast<packet_header *>(buffer_.data());
        if (packetHeader.heap_ != nullptr)
        {
            // was allocated from heap
            packetHeader.heap_->push(buffer_);
            packetHeader.heap_ = nullptr;
        }
        else
        {
            // was allocated from process memory
            delete [] buffer_.data();
        }
    }
    size_ = {};
    buffer_ = {};
    begin_ = {};
    ownsData_ = {};
}


//=============================================================================
inline bcpp::network::packet::operator bool
(
) const
{
    return (buffer_.size() > 0);
}


//=============================================================================
inline bcpp::network::packet::operator std::span<element_type const>
(
) const
{
    return {begin(), size_};
}
