//=============================================================================
//
// These examples demonstrate:
//
// 1. creating dedicated polling thread(s)
// 2. 
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
// See the README for more details on the motivation for this architecture.
//
//=============================================================================


#include <library/network.h>

#include <iostream>
#include <chrono>

namespace
{

    auto create_packet
    (
        std::string_view message
    ) -> bcpp::network::packet
    {
        bcpp::network::packet packet({[](auto const & p){delete [] p.data();}}, std::span(new char[message.size()], message.size()));
        std::copy_n(message.data(), message.size(), packet.data());
        packet.resize(message.size());
        return packet; 
    }

}


//============================================================================
namespace example_1
{

    auto send_packet
    (
        // demonstrate creation of a virtual network interface
        // create a udp socket from that virtual network interface
        // send a packet from that udp socket
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
        
        // we will simply send a packet back to the same socket. 
        // This example is only concerned with the send itself
        auto packet = create_packet("here is your message!");
        auto success = udpSocket.send_to(udpSocket.get_socket_address(), std::move(packet));
        if (!success)
        {
            std::cerr << "Failed to send packet\n";
            return -1;
        }
        std::cout << "packet sent\n";
        return 0;
    }

} // namespace example_1




//============================================================================
namespace example_2
{

    auto send_completion_token
    (
        // create a virtual network interface
        // create udp socket from that virtual network interface
        // send packet from that socket to that same socket
        // attach a send completion token to the send
        // wait for the send completion notification indicating that the async send has been completed
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
        
        // we will simply send a packet back to the same socket.  This example is only concerned
        // with receiving the async send completion callback.
        auto packet = create_packet("here is your message!");
        std::atomic<bool> sendCompletion{false};
        bcpp::network::send_completion_token sendCompletionToken{
                [&](auto)
                {
                    std::cout << "Received send completion notification\n";
                    sendCompletion = true;
                }};
        udpSocket.send_to(udpSocket.get_socket_address(), std::move(packet), sendCompletionToken);

        // sending and receiving are asynchronous processes.  some thread needs to doing the polling and service the sockets
        // in order to complete the async process.  This thread will do it on its own.
        using namespace std::chrono_literals;
        auto maxWaitTime = 1s;
        auto endWait = std::chrono::system_clock::now() + maxWaitTime;
        while ((!sendCompletion) && (std::chrono::system_clock::now() <= endWait))
        {
            virtualNetworkInterface.poll();                 // poll for any socket which has data to receive
            virtualNetworkInterface.service_sockets();      // process any pending sends and/or receives
        }
        if (!sendCompletion)
        {
            std::cerr << "Failed to receive send completion notification before timeout\n";
            return -1;
        }
        return 0;
    }

} // namespace example_2


//============================================================================
namespace example_3
{

    auto receive_packet
    (
        // create virtual network interface
        // create udp socket 1 from that network interface (the sender)
        // create udp socket 2 from that network interface (the receiver)
        // create a receive_handler for udp socket 2 (receiver) which is invoked when the socket receives the packet
        // send packet from udp socket 1 to udp socket 2
        // wait for udp socket 2's receive handler to be invoked when it receives the packet
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
        bcpp::network::udp_socket::configuration udpSocket1Configuration;
        bcpp::network::udp_socket::event_handlers udpSocket1EventHandlers;
        auto udpSocket1 = virtualNetworkInterface.create_udp_socket(udpSocket1Configuration, udpSocket1EventHandlers);
        if (!udpSocket1.is_valid())
        {
            std::cerr << "Failed to create UDP socket 1 from virtual network interface\n";
            return -1;
        }

        // create a udp socket from the virtual network interface
        bcpp::network::udp_socket::configuration udpSocket2Configuration;
        bcpp::network::udp_socket::event_handlers udpSocket2EventHandlers;
        // flag used to indicate when socket 2 has received the packet
        std::atomic<bool> messageReceived{false};                       
        // socket 2 will be receiving a packet from socket 1.  We need to provide
        // a 'receive handler' which will be invoked when socket 2 receives a packet.
        // Once the socket has been selected during polling (socket has data) this handler
        // will be invoked when virtual_network_interface::service_sockets() is called.
        udpSocket2EventHandlers.receiveHandler_ = [&]
                (
                    auto socketId,              // receiving socket's id
                    auto packet,                // the packet being received
                    auto senderSocketAddress    // the address of the sending socket (udp socket 1)
                )
                {
                    std::cout << "socket 2 has received a packet containing message \"" << packet.data() << "\"\n";
                    messageReceived = true; // ok to exit the example now.
                };
        auto udpSocket2 = virtualNetworkInterface.create_udp_socket(udpSocket2Configuration, udpSocket2EventHandlers);
        if (!udpSocket2.is_valid())
        {
            std::cerr << "Failed to create UDP socket 2 from virtual network interface\n";
            return -1;
        }
        
        // send a packet from udp socket 1 to udp socket 2
        // NOTE: send() and send_to() do not actually send any data.  Instead, they
        // queue the packet to be sent.  Packets are actually send calling virtual_network_interface::service_sockets()
        auto packet = create_packet("here is your message!");
        udpSocket1.send_to(udpSocket2.get_socket_address(), std::move(packet));

        // sending and receiving are asynchronous processes.  some thread needs to doing the polling and service the sockets
        // in order to complete the async process.  This thread will do it on its own.
        using namespace std::chrono_literals;
        auto maxWaitTime = 1s;
        auto endWait = std::chrono::system_clock::now() + maxWaitTime;
        while ((!messageReceived) && (std::chrono::system_clock::now() <= endWait))
        {
            virtualNetworkInterface.poll();                 // poll for any socket which has data to receive
            virtualNetworkInterface.service_sockets();      // process any pending sends and/or receives
        }
        if (!messageReceived)
        {
            std::cerr << "Failed to receive message before timeout\n";
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
    if (auto result = example_1::send_packet(); result != 0)
    {
        std::cout << "example_1 failed\n";
        return -1;
    }

    if (auto result = example_2::send_completion_token(); result != 0)
    {
        std::cout << "example_2 failed\n";
        return -1;
    }

    if (auto result = example_3::receive_packet(); result != 0)
    {
        std::cout << "example_3 failed\n";
        return -1;
    }

    return 0;
}
