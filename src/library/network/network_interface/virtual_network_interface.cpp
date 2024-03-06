#include "./virtual_network_interface.h"

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>


//=============================================================================
bcpp::network::virtual_network_interface::virtual_network_interface
(
)
{
    networkInterfaceConfiguration_.ipAddress_ = in_addr_any;
    poller_ = poller_->create({});
    sendWorkContractGroup_ = std::make_unique<system::blocking_work_contract_group>(default_capacity);
    receiveWorkContractGroup_ = std::make_unique<system::blocking_work_contract_group>(default_capacity);
    stopped_ = false;
}


//=============================================================================
bcpp::network::virtual_network_interface::virtual_network_interface
(
    configuration const & config
):
    networkInterfaceConfiguration_(config.networkInterfaceConfiguration_)
{
    if (networkInterfaceConfiguration_.ipAddress_.is_valid())
    {
        poller_ = poller_->create(config.poller_);
        sendWorkContractGroup_ = std::make_unique<system::blocking_work_contract_group>(config.capacity_);
        receiveWorkContractGroup_ = std::make_unique<system::blocking_work_contract_group>(config.capacity_);
        stopped_ = false;
    }
}


//=============================================================================
bcpp::network::virtual_network_interface::virtual_network_interface
(
    virtual_network_interface && other
):
    networkInterfaceConfiguration_(other.networkInterfaceConfiguration_),
    poller_(other.poller_),
    sendWorkContractGroup_(std::move(other.sendWorkContractGroup_)),
    receiveWorkContractGroup_(std::move(other.receiveWorkContractGroup_)),
    stopped_(other.stopped_.load())
{
    other.networkInterfaceConfiguration_ = {};
    other.poller_ = {};
    other.stopped_ = true;
}


//=============================================================================
auto bcpp::network::virtual_network_interface::operator =
(
    virtual_network_interface && other
) -> virtual_network_interface & 
{
    if (&other != this)
    {
        stop();

        networkInterfaceConfiguration_ = other.networkInterfaceConfiguration_;
        poller_ = other.poller_;
        sendWorkContractGroup_ = std::move(other.sendWorkContractGroup_);
        receiveWorkContractGroup_ = std::move(other.receiveWorkContractGroup_);
        stopped_ = other.stopped_.load();

        other.networkInterfaceConfiguration_ = {};
        other.poller_ = {};
        other.stopped_ = true;
    }
    return *this;
}


//=============================================================================
bcpp::network::virtual_network_interface::~virtual_network_interface
(
)
{
    stop();
}


//=============================================================================
bool bcpp::network::virtual_network_interface::is_valid
(
) const
{
    return networkInterfaceConfiguration_.ipAddress_.is_valid();
}


//=============================================================================
bool bcpp::network::virtual_network_interface::is_loop_back
(
) const
{
    return (networkInterfaceConfiguration_.loopback_);
}


//=============================================================================
auto bcpp::network::virtual_network_interface::get_ip_address
(
) const -> ip_address
{
    return networkInterfaceConfiguration_.ipAddress_;
}


//=============================================================================
auto bcpp::network::virtual_network_interface::get_name
(
) const -> network_interface_name const &
{
    return networkInterfaceConfiguration_.name_;
}


//=============================================================================
void bcpp::network::virtual_network_interface::stop
(
)
{
    // stop the work contract group.  this will release each of the work contracts 
    // that are associated with the sockets that were created by this network interface.
    if (auto wasRunning = (stopped_.exchange(true) == false); wasRunning)
    {
        networkInterfaceConfiguration_ = {};
        receiveWorkContractGroup_->stop();
        receiveWorkContractGroup_ = {};
        sendWorkContractGroup_->stop();
        sendWorkContractGroup_ = {};
        poller_ = {};

        // any work contracts that were surrendered in the previous step must not be 
        // serviced to complete the async close and destroy (the impl) of any existing sockets.
       // while (workContractGroup_->get_active_contract_count())
        //    service_sockets();
    }
}


//=============================================================================
template <bcpp::network::socket_concept S, typename T>
auto bcpp::network::virtual_network_interface::open_socket
(
    T handle,
    typename S::configuration config,
    typename S::event_handlers eventHandlers
) -> S
{
    if constexpr (active_socket_concept<S>)
        return S(std::move(handle), config, eventHandlers, *sendWorkContractGroup_, *receiveWorkContractGroup_, poller_);
    else
        return S(std::move(handle), config, eventHandlers, *receiveWorkContractGroup_, poller_);
}


//=============================================================================
auto bcpp::network::virtual_network_interface::create_tcp_socket
(
    tcp_listener_socket::configuration config,
    tcp_listener_socket::event_handlers eventHandlers
) -> tcp_listener_socket
{
    return open_socket<tcp_listener_socket>(socket_address{networkInterfaceConfiguration_.ipAddress_, config.portId_}, config, eventHandlers);
}


//=============================================================================
auto bcpp::network::virtual_network_interface::accept_tcp_socket
(
    system::file_descriptor fileDescriptor,
    tcp_socket::configuration config,
    tcp_socket::event_handlers eventHandlers
) -> tcp_socket
{
    return open_socket<tcp_socket>(std::move(fileDescriptor), config, eventHandlers);
}


//=============================================================================
auto bcpp::network::virtual_network_interface::create_tcp_socket
(
    socket_address remoteSocketAddress,
    tcp_socket::configuration config,
    tcp_socket::event_handlers eventHandlers
) -> tcp_socket
{
    auto tcpSocket = open_socket<tcp_socket>(networkInterfaceConfiguration_.ipAddress_, config, eventHandlers);
    tcpSocket.connect_to(remoteSocketAddress);
    return tcpSocket;
}


//=============================================================================
auto bcpp::network::virtual_network_interface::create_udp_socket
(
    port_id localPortId,
    udp_socket::configuration config,
    udp_socket::event_handlers eventHandlers
) -> udp_socket
{
    return open_socket<udp_socket>(socket_address{networkInterfaceConfiguration_.ipAddress_, port_id_any}, config, eventHandlers);
}


//=============================================================================
auto bcpp::network::virtual_network_interface::create_udp_socket
(
    udp_socket::configuration config,
    udp_socket::event_handlers eventHandlers
) -> udp_socket
{
    return create_udp_socket(port_id_any, config, eventHandlers);
}


//=============================================================================
auto bcpp::network::virtual_network_interface::multicast_join
(
    socket_address socketAddress,
    udp_socket::configuration config,
    udp_socket::event_handlers eventHandlers
) -> udp_socket
{
    auto udpSocket = open_socket<udp_socket>(socket_address{in_addr_any, socketAddress.get_port_id()}, config, eventHandlers);
    udpSocket.join(socketAddress.get_ip_address());
    return udpSocket;
}


//=============================================================================
void bcpp::network::virtual_network_interface::poll
(
    std::chrono::milliseconds duration
)
{
    poller_->poll(duration);
}


//=============================================================================
void bcpp::network::virtual_network_interface::poll
(
)
{
    poller_->poll();
}


//=============================================================================
void bcpp::network::virtual_network_interface::service_sockets
(
    std::chrono::nanoseconds duration
)
{
    receiveWorkContractGroup_->execute_next_contract(); // TODO: need to restore blocking mode ... duration);
    sendWorkContractGroup_->execute_next_contract(); // TODO: need to restore blocking mode ... duration);
}


//=============================================================================
void bcpp::network::virtual_network_interface::service_sockets
(
)
{
    receiveWorkContractGroup_->execute_next_contract();
    sendWorkContractGroup_->execute_next_contract();
}


//=============================================================================
namespace bcpp::network
{
    template tcp_socket virtual_network_interface::open_socket(system::file_descriptor, tcp_socket::configuration, tcp_socket::event_handlers);
    template tcp_socket virtual_network_interface::open_socket(ip_address, tcp_socket::configuration, tcp_socket::event_handlers);
    template tcp_listener_socket virtual_network_interface::open_socket(socket_address, tcp_listener_socket::configuration, tcp_listener_socket::event_handlers);
    template udp_socket virtual_network_interface::open_socket(socket_address, udp_socket::configuration, udp_socket::event_handlers);

}