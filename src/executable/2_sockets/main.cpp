//=============================================================================
//
// These examples demonstrate:
//
// 1. creating a udp socket from a virtual_network_interface
// 2. creating a tcp listener socket from a virtual network interface
// 3. creating a tcp socket and connecting it to a tcp listener socket
//
// NOTES: 
//
// Because bcpp::network is 100% asynchronous neither sends nor receives will
// take place until such time as some thread invokes poll() and service_sockets()
// on the virtual_network_interface associated with the sockets which are sending
// and receiving data.
//
// To keep these examples simple the main thread will invoke these functions in
// a loop in order to ensure that all async send/receive actually occur.
// 
// See the README in 4_async_poll_send_and_recv 
// for more details on the motivation for this architecture.
//
//=============================================================================


#include <library/network.h>

#include <iostream>
#include <chrono>


//============================================================================
namespace example_1
{

    auto create_udp_socket
    (
        // create a basic connectionless UDP socket with default configuration and no event handlers
    )
    {
        // create a virtual network interface
        bcpp::network::virtual_network_interface virtualNetworkInterface;
        if (!virtualNetworkInterface.is_valid())
        {
            std::cerr << "Failed to create virtual network interface\n";
            return -1;
        }

        // create a udp socket from the virtual network interface
        bcpp::network::udp_socket::configuration udpSocketConfiguration;
        bcpp::network::udp_socket::event_handlers udpSocketEventHandlers;
        auto udpSocket = virtualNetworkInterface.create_udp_socket(udpSocketConfiguration, udpSocketEventHandlers);
        if (!udpSocket.is_valid())
        {
            std::cerr << "Failed to create UDP socket from virtual network interface\n";
            return -1;
        }

        std::cout << "Succesfully created UDP socket\n";
        return 0;
    }

} // namespace example_1


//============================================================================
namespace example_2
{

    auto create_tcp_listener_socket
    (
        // create a basic TCP listener socket with default configuration and no event handlers
    )
    {
        // create a virtual network interface
        bcpp::network::virtual_network_interface virtualNetworkInterface;
        if (!virtualNetworkInterface.is_valid())
        {
            std::cerr << "Failed to create virtual network interface\n";
            return -1;
        }

        // create a TCP listener socket from the virtual network interface
        bcpp::network::tcp_listener_socket::configuration tcpListenerSocketConfiguration;
        bcpp::network::tcp_listener_socket::event_handlers tcpListenerSocketEventHandlers;
        auto tcpListenerSocket = virtualNetworkInterface.create_tcp_socket(tcpListenerSocketConfiguration, tcpListenerSocketEventHandlers);
        if (!tcpListenerSocket.is_valid())
        {
            std::cerr << "Failed to create TCP listener socket from virtual network interface\n";
            return -1;
        }

        std::cout << "Succesfully created TCP listener socket\n";
        return 0;
    }

} // namespace example_2


//============================================================================
namespace example_3
{

    auto create_tcp_socket_and_connect_to_listener_socket
    (
        // create a TCP listener socket and assign an `accept_handler`
        // create a basic TCP socket with default configuration and no event handlers
        // connect TCP socket to TCP listener socket
    )
    {
        // create a virtual network interface
        bcpp::network::virtual_network_interface virtualNetworkInterface;
        if (!virtualNetworkInterface.is_valid())
        {
            std::cerr << "Failed to create virtual network interface\n";
            return -1;
        }

        std::atomic<bool> connectionAccepted{false};

        // create a TCP listener socket from the virtual network interface.  We can use the
        // default configuration but we need to assign an accept handler to accept the TCP socket connection.
        bcpp::network::tcp_listener_socket::configuration tcpListenerSocketConfiguration;
        bcpp::network::tcp_listener_socket::event_handlers tcpListenerSocketEventHandlers
                {
                    .acceptHandler_ = [&](auto socketId, auto fileDescriptor)
                            {
                                // wrap accepted file descriptor in new TCP socket
                                bcpp::network::tcp_socket::configuration acceptedTcpSocketConfiguration;
                                bcpp::network::tcp_socket::event_handlers acceptedTcpSocketEventHandlers;
                                auto tcpSocket = virtualNetworkInterface.accept_tcp_socket(std::move(fileDescriptor),
                                        acceptedTcpSocketConfiguration, acceptedTcpSocketEventHandlers);
                                std::cout << "Accepted TCP connection\n";
                                connectionAccepted = true;
                            }
                };
        auto tcpListenerSocket = virtualNetworkInterface.create_tcp_socket(tcpListenerSocketConfiguration, tcpListenerSocketEventHandlers);
        if (!tcpListenerSocket.is_valid())
        {
            std::cerr << "Failed to create TCP listener socket from virtual network interface\n";
            return -1;
        }

        // create a TCP socket from the virtual network interface and connect it to the tcp listener socket address
        bcpp::network::tcp_socket::configuration tcpSocketConfiguration;
        bcpp::network::tcp_socket::event_handlers tcpSocketEventHandlers;
        auto tcpSocket = virtualNetworkInterface.create_tcp_socket(tcpListenerSocket.get_socket_address(), tcpSocketConfiguration, tcpSocketEventHandlers);
        if (!tcpSocket.is_valid())
        {
            std::cerr << "Failed to create TCP socket from virtual network interface\n";
            return -1;
        }
        std::cout << "Succesfully created TCP socket\n";

        // connections are accepted async.  some thread needs to doing the polling and service the sockets
        // in order to complete the async process.  This thread will do it on its own.
        // NOTE: poll and service_sockets are covered in 4_async_poll_send_and_recv
        using namespace std::chrono_literals;
        auto maxWaitTime = 1s;
        auto endWait = std::chrono::system_clock::now() + maxWaitTime;
        while ((!connectionAccepted) && (std::chrono::system_clock::now() <= endWait))
        {
            virtualNetworkInterface.poll();
            virtualNetworkInterface.service_sockets();
        }
        if (!connectionAccepted)
        {
            std::cerr << "Failed to accept socket connection before timeout\n";
            return -1;
        }
        return 0;
    }

} // namespace example_3


//=============================================================================
int main
(
    int,
    char **
)
{
    if (auto result = example_1::create_udp_socket(); result != 0)
    {
        std::cout << "example_1 failed\n";
        return -1;
    }

    if (auto result = example_2::create_tcp_listener_socket(); result != 0)
    {
        std::cout << "example_2 failed\n";
        return -1;
    }

    if (auto result = example_3::create_tcp_socket_and_connect_to_listener_socket(); result != 0)
    {
        std::cout << "example_3 failed\n";
        return -1;
    }

    return 0;
}
