#include <library/network.h>

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <atomic>


std::mutex mutex;
std::condition_variable conditionVariable;
std::atomic<bool> acceptedConnect{false};




//=============================================================================
int main
(
    int,
    char **
)
{
    using namespace std::chrono;
    using namespace bcpp::network::literals;

    bcpp::network::tcp_socket acceptedSocket;

    // for this test we will use the default network interface and not specify one
    std::cout << "create virtual network interface\n";
    bcpp::network::virtual_network_interface virtualNetworkInterface;
    if (!virtualNetworkInterface.is_valid())
    {
        std::cerr << "Failed to create virtual network interface\n";
        return -1;
    }
    std::cout << "\tcreate tcp listener socket\n";

    auto tcpListenerSocket = virtualNetworkInterface.create_tcp_socket({.portId_ = 50000_port}, 
            {
                .acceptHandler_ = [&]
                        (
                            bcpp::network::socket_id socketId, 
                            bcpp::system::file_descriptor fileDescriptor
                        )
                        {
                                // wrap accepted file descriptor in new TCP socket
                                bcpp::network::tcp_socket::configuration acceptedTcpSocketConfiguration;
                                bcpp::network::tcp_socket::event_handlers acceptedTcpSocketEventHandlers;
                                acceptedSocket = virtualNetworkInterface.accept_tcp_socket(std::move(fileDescriptor),
                                        acceptedTcpSocketConfiguration, acceptedTcpSocketEventHandlers);
                            std::unique_lock uniqueLock(mutex);
                            acceptedConnect = true;
                            conditionVariable.notify_all();
                        }
            });
    if (!tcpListenerSocket.is_valid())
    {
        std::cerr << "Failed to create tcp listener socket\n";
        return -1;
    }

    // create worker thread to do async connect accept
    std::jthread workerThread([&](std::stop_token const & stopToken)
            {
                while (!stopToken.stop_requested())
                {
                    virtualNetworkInterface.poll();
                    virtualNetworkInterface.service_sockets();
                }
        });
/*
    auto tcpSocket = virtualNetworkInterface.create_tcp_socket(
            tcpListenerSocket.get_socket_address(), 
            {}, 
            {
                .receiveHandler_ = [&]
                        (
                            auto, 
                            auto packet, 
                            auto
                        )
                        {
                            std::cout << std::string(packet.data(), packet.size()) << std::flush;
                        }
            });

    if (!tcpSocket.is_valid())
    {
        std::cerr << "Failed to connect to tcp listener socket\n";
        return -1;
    }
*/
    std::unique_lock uniqueLock(mutex);
    if (!conditionVariable.wait_for(uniqueLock, 1s, [&](){return acceptedConnect.load();}))
    {
        std::cerr << "Failed to accept connection\n";
        return -1;
    }

    for (auto i = 0; i < (1 << 20); ++i)
    {
        std::string message = "this is message # ";
        message += std::to_string(i);
        bcpp::network::packet p(std::span<char const>(message.data(), message.size()));
        while (!acceptedSocket.send(std::move(p)))
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout << "success\n";
    return 0;
}
