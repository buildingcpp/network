#include "./socket_base_impl.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/un.h>
#include <cerrno>

#include <cstdint>
#include <string_view>
#include <exception>


//=============================================================================
bcpp::network::socket_base_impl::socket_base_impl
(
    socket_address socketAddress,
    configuration const & config,
    event_handlers const & eventHandlers,
    system::file_descriptor fileDescriptor,
    work_contract workContract
) try :
    fileDescriptor_(std::move(fileDescriptor)),
    closeHandler_(eventHandlers.closeHandler_),
    pollErrorHandler_(eventHandlers.pollErrorHandler_),
    receiveContract_(std::move(workContract))
{
    if (not set_socket_option(SOL_SOCKET, SO_REUSEADDR, 1))
        throw std::runtime_error("reuse address failure");
    if (!socketAddress.is_multicast())
    {
        bind(socketAddress);
        socketAddress_ = get_socket_name();
    }
    if (auto success = set_synchronicity(synchronization_mode::non_blocking); !success)
        throw std::runtime_error("set non_blocking failure");
    if (auto success = set_io_mode(config.ioMode_); !success)
        throw std::runtime_error("set_io_mode failure");
}
catch (std::exception const & exception)
{
    fileDescriptor_ = {};
    socketAddress_ = {};
    closeHandler_ = nullptr;
    pollErrorHandler_ = nullptr;
    std::rethrow_exception(std::current_exception());
}


//=============================================================================
bcpp::network::socket_base_impl::socket_base_impl
(
    configuration const & config,
    event_handlers const & eventHandlers,
    system::file_descriptor fileDescriptor,
    work_contract workContract
) try :
    fileDescriptor_(std::move(fileDescriptor)),
    closeHandler_(eventHandlers.closeHandler_),
    pollErrorHandler_(eventHandlers.pollErrorHandler_),
    receiveContract_(std::move(workContract))
{
    if (not set_socket_option(SOL_SOCKET, SO_REUSEADDR, 1))
        throw std::runtime_error("reuse address failure");
    if (auto success = set_synchronicity(synchronization_mode::non_blocking); !success)
        throw std::runtime_error("set non_blocking failure");
    if (auto success = set_io_mode(config.ioMode_); !success)
        throw std::runtime_error("set_io_mode failure");
    socketAddress_ = get_socket_name();
}
catch (std::exception const &)
{
    fileDescriptor_ = {};
    socketAddress_ = {};
    closeHandler_ = nullptr;
    pollErrorHandler_ = nullptr;
    std::rethrow_exception(std::current_exception());
}



//=============================================================================
bcpp::network::socket_base_impl::~socket_base_impl
(
)
{
    close();
}


//=============================================================================
std::optional<std::int32_t> bcpp::network::socket_base_impl::get_socket_option
(
    std::int32_t level,
    std::int32_t optionName
) const noexcept
{
    std::int32_t result = 0;
    if (::setsockopt(fileDescriptor_.get(), level, optionName, &result, sizeof(result)) == 0)
        return {result};
    return {};
}


//=============================================================================
void bcpp::network::socket_base_impl::on_polled
(
)
{
    receiveContract_.schedule();
}


//=============================================================================
void bcpp::network::socket_base_impl::on_poll_error
(
)
{
    if (pollErrorHandler_)
        pollErrorHandler_(id_);
}


//=============================================================================
auto bcpp::network::socket_base_impl::get_socket_name
(
) const noexcept -> socket_address
{
    ::sockaddr_in socketAddress;
    ::socklen_t sizeofSocketAddress(sizeof(socketAddress));
    if (::getsockname(fileDescriptor_.get(), (struct sockaddr *)&socketAddress, &sizeofSocketAddress) == 0)
        return socketAddress;
    return {};
}


//=============================================================================
void bcpp::network::socket_base_impl::bind
(
    socket_address const & socketAddress
)
{
    if (!fileDescriptor_.is_valid())
        throw std::runtime_error("invalid file descriptor");
    ::sockaddr_in sockAddrIn = socketAddress;
    sockAddrIn.sin_family = AF_INET;
    if (::bind(fileDescriptor_.get(), (sockaddr const *)&sockAddrIn, sizeof(sockAddrIn)) == -1)
        throw std::runtime_error("bind_error");
    socketAddress_ = get_socket_name();
}


//=============================================================================
bool bcpp::network::socket_base_impl::close
(
)
{
    if (fileDescriptor_.close())
    {
        if (closeHandler_)
            closeHandler_(id_);
        socketAddress_ = {};
        return true;
    }
    return false;
}


//=============================================================================
bool bcpp::network::socket_base_impl::is_valid
(
) const noexcept
{
    return (fileDescriptor_.is_valid());
}


//=============================================================================
auto bcpp::network::socket_base_impl::get_file_descriptor
(
) const noexcept -> system::file_descriptor const & 
{
    return fileDescriptor_;
}


//=============================================================================
auto bcpp::network::socket_base_impl::get_socket_address
(
) const noexcept -> socket_address
{
    return socketAddress_;
}


//=============================================================================
auto bcpp::network::socket_base_impl::get_ip_address
(
) const noexcept -> ip_address
{
    return socketAddress_.get_ip_address();
}


//=============================================================================
auto bcpp::network::socket_base_impl::get_id
(
) const noexcept -> socket_id
{
    return id_;
}


//=============================================================================
bool bcpp::network::socket_base_impl::set_synchronicity
(
    synchronization_mode mode
) noexcept
{
    auto flags = ::fcntl(fileDescriptor_.get(), F_GETFL, 0);
    if (flags == -1)
        return false;
    if (mode == synchronization_mode::blocking)
    {
        // synchronous/blocking mode
        flags &= ~O_NONBLOCK;
    }
    else
    {
        // asynchronous/non-blocking
        flags |= O_NONBLOCK;
    }
    auto fcntlResult = ::fcntl(fileDescriptor_.get(), F_SETFL, flags);
    if (fcntlResult != 0)
        return false;
    return true; 
}


//=============================================================================
bool bcpp::network::socket_base_impl::shutdown
(
) noexcept
{
    return set_io_mode(system::io_mode::none);
}


//=============================================================================
bool bcpp::network::socket_base_impl::set_io_mode
(
    system::io_mode ioMode
) noexcept
{
    switch (ioMode)
    {
        case system::io_mode::read:
        {
            return shutdown(system::io_mode::write);
        }
        case system::io_mode::write:
        {
            return shutdown(system::io_mode::read);
        }
        case system::io_mode::read_write:
        {
            return shutdown(system::io_mode::none);
        }
        case system::io_mode::none:
        {
            return shutdown(system::io_mode::read_write);
        }
        default:
        {
            return true;
        }
    }
}


//=============================================================================
bool bcpp::network::socket_base_impl::shutdown
(
    system::io_mode ioMode
) noexcept
{
    switch (ioMode)
    {
        case system::io_mode::read:
        {
            return (::shutdown(fileDescriptor_.get(), SHUT_RD) == 0);
        }
        case system::io_mode::write:
        {
            return (::shutdown(fileDescriptor_.get(), SHUT_WR) == 0);
        }
        case system::io_mode::read_write:
        {
            return (::shutdown(fileDescriptor_.get(), SHUT_RDWR) == 0);
        }
        case system::io_mode::none:
        default:
        {
            return true;
        }
    }
}
