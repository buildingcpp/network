#pragma once

#include <library/network/socket/socket.h>
#include <library/system.h>

#include <deque>
#include <mutex>
#include <vector>
#include <span>


namespace bcpp::network
{

    // quick proof of concept for async send 
    // by wrapping socket with 'stream' class

    class default_buffer_type
    {
    public:
        default_buffer_type() = default;
        default_buffer_type(std::span<char const> data):data_(data.begin(), data.end()){}
        auto begin() const{return data_.begin();}
        auto end() const{return data_.end();}
        auto size() const{return data_.size();}
        auto empty() const{return data_.empty();}
        operator std::span<char const>()const{return data_;}
    private:
        std::vector<char> data_;
    };


    template <socket_concept S, typename B = default_buffer_type>
    class stream
    {
    public:

        using socket_type = S;
        using buffer_type = B;

        stream
        (
            socket_type socket,
            system::work_contract_group & workContractGroup
        ):
            socket_(std::move(socket)),
            workContract_(workContractGroup.create_contract([this](){this->send();}))
        {
        }

        template <typename T>
        void send
        (
            T && data
        )
        {
            std::lock_guard lockGuard(mutex_);
            buffers_.emplace_back(std::forward<T>(data));
            workContract_.invoke();
        }

        connect_result connect_to
        (
            socket_address const & destination
        ) noexcept
        {
            return socket_.connect_to(destination);
        }

        bool close(){return socket_.close();}

        bool is_valid() const noexcept{return socket_.is_valid();}

        socket_address get_ip_address() const noexcept{return socket_.get_ip_address();}

        bool is_connected() const noexcept{return socket_.is_connected();}

        socket_address get_connected_ip_address() const noexcept{return socket_.get_connected_ip_address();}


    private:

        void send
        (
        )
        {
            std::lock_guard lockGuard(mutex_);
            if (!buffers_.empty())
            {
                auto const & buffer = buffers_.front();
                socket_.send(buffer);
                buffers_.pop_front();
                if (!buffers_.empty())
                    workContract_.invoke();
            }
        }

        socket_type                     socket_;
        system::work_contract           workContract_;
        std::deque<buffer_type>         buffers_;
        std::mutex mutable              mutex_;
    };


    using tcp_stream = stream<tcp_socket>;
    using udp_stream = stream<udp_socket>;

} // bcpp::network
