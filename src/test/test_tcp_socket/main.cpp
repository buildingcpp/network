#include <library/network.h>

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <atomic>


std::mutex mutex;
std::condition_variable conditionVariable;
std::atomic<bool> acceptedConnect{false};


//=============================================================================
void accept_connection
(
    bcpp::network::socket_id socketId, 
    bcpp::system::file_descriptor fileDescriptor
)
{
    std::cerr << "\taccepted connection\n";
    std::unique_lock uniqueLock(mutex);
    acceptedConnect = true;
    conditionVariable.notify_all();
}


//=============================================================================
int main
(
    int,
    char **
)
{
    using namespace std::chrono;
    using namespace bcpp::literals;

    std::cout << "create virtual network interface\n";
    bcpp::network::virtual_network_interface virtualNetworkInterface({.physicalNetworkInterfaceName_ = "lo"});
    if (!virtualNetworkInterface.is_valid())
    {
        std::cerr << "Failed to create virtual network interface\n";
        return -1;
    }
    std::cout << "\tcreate tcp listener socket\n";

    auto tcpListenerSocket = virtualNetworkInterface.tcp_listen(3000_port, {}, {.acceptHandler_ = accept_connection});
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

    auto tcpSocket = virtualNetworkInterface.tcp_connect(tcpListenerSocket.get_socket_address(), {}, {});
    if (!tcpSocket.is_valid())
    {
        std::cerr << "Failed to connect to tcp listener socket\n";
        return -1;
    }

    std::unique_lock uniqueLock(mutex);
    if (!conditionVariable.wait_for(uniqueLock, 1s, [&](){return acceptedConnect.load();}))
    {
        std::cerr << "Failed to accept connection\n";
        return -1;
    }
    std::cout << "success\n";
    return 0;
}
