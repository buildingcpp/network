#include <library/network.h>

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <atomic>


//=============================================================================
int main
(
    int,
    char **
)
{
    using namespace std::chrono;
    using namespace bcpp::network::literals;

    std::mutex mutex;
    std::condition_variable conditionVariable;
    std::atomic<bool> sendCompletion{false};

    std::cout << "create virtual network interface\n";
    bcpp::network::virtual_network_interface virtualNetworkInterface({.physicalNetworkInterfaceName_ = "lo"});
    if (!virtualNetworkInterface.is_valid())
    {
        std::cerr << "Failed to create virtual network interface\n";
        return -1;
    }
    std::cout << "\tcreate tcp listener socket\n";

    auto tcpListenerSocket = virtualNetworkInterface.create_tcp_socket({.portId_ = 3000_port}, {});
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

    auto tcpSocket = virtualNetworkInterface.create_tcp_socket(tcpListenerSocket.get_socket_address(), {}, {});
    if (!tcpSocket.is_valid())
    {
        std::cerr << "Failed to connect to tcp listener socket\n";
        return -1;
    }

    bcpp::network::send_completion_token sendCompletionToken{
            [&](auto)
            { 
                std::lock_guard lockGuard(mutex);
                std::cout << "\tReceived send completion\n";
                sendCompletion = true;
                conditionVariable.notify_all();
            }};
    tcpSocket.send(""_packet, sendCompletionToken);

    std::unique_lock uniqueLock(mutex);
    if (!conditionVariable.wait_for(uniqueLock, 1s, [&](){return sendCompletion.load();}))
    {
        std::cerr << "Failed to receive send completion callback\n";
        return -1;
    }
    std::cout << "success\n";
    return 0;
}
