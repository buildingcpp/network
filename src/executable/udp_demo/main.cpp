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
    


    // this socket will actually be using the 'stream' rather than the 'socket'
    // streams are still a work in progress but allow for asynchronous send.
    // demonstrate send_to
    auto socket2 = networkInterface.open_stream<udp_socket>(socket_address{loop_back, port_id_any}, {}, {});
    auto buffer = new char[2048];
    std::copy_n("guess what!", 12, buffer);
    packet p({[](auto const & p){delete [] p.data();}}, std::span(buffer, 2048));
    p.resize(12);    
    socket2.send_to(socket1.get_socket_address(), std::move(p));

    // now connect and then use send rather than send_to
    socket2.connect_to(socket1.get_socket_address());
    auto buffer2 = new char[2048];
    std::copy_n("chicken butt!!", 14, buffer2);
    packet p2({[](auto const & p){delete [] p.data();}}, std::span(buffer2, 2048));
    p2.resize(14);
    socket2.send_to({}, std::move(p2));

    // since demo is async we wait for the messages to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    networkInterface.stop();
    
    return 0;
}
