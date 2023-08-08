#include <library/network.h>
#include <iostream>
#include <string_view>


//=============================================================================
int main
(
    int,
    char **
)
{
    using namespace bcpp::network;

    // create network interface
    network_interface networkInterface;

    // create two threads (we could do test with one)
    // one thread to continually poll the network interface for events
    std::jthread pollerThread([&](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface.poll();});
    // one worker thread to service any sockets which have data to receive
    std::jthread workerThread([&](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface.service_sockets();});

    // create a standard (connectionless) UDP socket
    auto socket1 = networkInterface.udp_connectionless({loop_back, port_id_any}, {/*default configuration for this demo*/}, 
            {.receiveHandler_ = [](auto socketId, auto packet, auto senderSocketAddress){std::cout << "udp socket [id = " << socketId << "] received message from " << senderSocketAddress <<
            ".  Message is \"" << std::string_view((char const *)packet.data(), packet.size()) << "\"\n";}});

    // create a standard (connectionless) UDP socket. 
    // use default config and handers because this socket wont receive any messages in this example
    auto socket2 = networkInterface.udp_connectionless({loop_back, port_id_any}, {}, {});

    // demonstrate send_to
    if (auto && [unsent, errorCode] = socket2.send_to(socket1.get_ip_address(), "guess what!"); ((errorCode != 0) || (!unsent.empty())))
    {
        if (errorCode != 0)
            std::cout << "send error code = " << errorCode << "\n";
        if (!unsent.empty())
            std::cout << "send failed to send " << unsent.size() << " bytes\n";
    }

    // now connect and then use send
    socket2.connect_to(socket1.get_ip_address());
    if (auto && [unsent, errorCode] = socket2.send("chicken butt!"); ((errorCode != 0) || (!unsent.empty())))
    {
        if (errorCode != 0)
            std::cout << "send error code = " << errorCode << "\n";
        if (!unsent.empty())
            std::cout << "send failed to send " << unsent.size() << " bytes\n";
    }

    // since demo is async we wait for the messages to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    networkInterface.stop();
    
    return 0;
}
