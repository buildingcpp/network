#include <library/network.h>
#include <library/system.h>

#include <include/non_movable.h>
#include <include/non_copyable.h>

#include <iostream>
#include <memory>
#include <map>

using namespace bcpp;
using namespace bcpp::network;
using namespace bcpp::system;

static ip_address constexpr ipAddress{loop_back};
static port_id constexpr portId{3000};
static socket_address constexpr listenerSocketAddress{ipAddress, portId};


//=============================================================================
struct echo_server : non_movable, non_copyable
{
    echo_server():
        socket_(networkInterface_.tcp_listen(listenerSocketAddress, {},
                {.acceptHandler_ = [this](auto, auto fileDescriptor){sessions.push_back(std::make_unique<session>(networkInterface_, std::move(fileDescriptor)));}})),
        pollerThread_([this](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface_.poll();}),
        workerThread_([this](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface_.service_sockets();}){}

    ~echo_server()
    {
        networkInterface_.stop();
    }

    struct session : non_movable, non_copyable
    {
        session(network_interface & networkInterface, file_descriptor fileDescriptor):
            tcpSocket_(networkInterface.tcp_accept(std::move(fileDescriptor), {}, 
                    {.receiveHandler_ = [this](auto, auto packet, auto)
                            {tcpSocket_.send(std::span<char const>(packet.begin(), packet.size()));}})){}
        tcp_socket tcpSocket_;
    };

    network_interface                       networkInterface_;
    tcp_listener_socket                     socket_;
    std::vector<std::unique_ptr<session>>   sessions;
    std::jthread                            pollerThread_;
    std::jthread                            workerThread_;
};


//=============================================================================
struct echo_client : non_movable, non_copyable
{
    echo_client():
        socket_(networkInterface_.tcp_connect(ipAddress, listenerSocketAddress, {}, 
            {.receiveHandler_ = [&](auto, auto packet, auto){std::cout << std::string(packet.data(), packet.size()) << std::flush;}})),
        pollerThread_([this](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface_.poll();}),
        workerThread_([this](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface_.service_sockets();}){}

    ~echo_client()
    {
        networkInterface_.stop();
    }

    void send(std::span<char const> data){socket_.send(data);}

    network_interface   networkInterface_;
    tcp_socket          socket_;
    std::jthread        pollerThread_;
    std::jthread        workerThread_;
};


//=============================================================================
int main
(
    int,
    char **
)
{
    echo_server echoServer;
    echo_client echoClient;

    for (auto i = 0; i < 10; ++i)
        echoClient.send(fmt::format("message {}\n", i));

    std::this_thread::sleep_for(std::chrono::seconds(1)); // demo is async so give it a moment to complete
    return 0;
}
