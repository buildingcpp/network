#include <library/network.h>

#include <array>
#include <iostream>
#include <thread>


//=============================================================================
int main
(
    int,
    char **
)
{
    using namespace bcpp::network;

    auto print = [](std::string_view const input)
            {
                static std::mutex mutex;
                std::lock_guard lockGuard(mutex);
                std::cout << input << '\n';
            };

    // set up network interface and threads to poll and receive
    network_interface networkInterface;
    std::jthread pollerThread([&](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface.poll();});

    static auto constexpr num_works = 4;
    std::array<std::jthread, num_works> workerThreads;
    for (auto & workerThread : workerThreads)
        workerThread = std::jthread{[&](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface.service_sockets();}};

    // set up sender
    auto sender = networkInterface.udp_connect({loop_back, port_id_any}, "239.0.0.1:3000", {}, {});

    // set up receivers
    static auto constexpr number_of_receivers = 1;
    std::array<udp_socket, number_of_receivers> receivers;
    auto receiverId = 0;
    for (auto & receiver : receivers)
        receiver = networkInterface.multicast_join("239.0.0.1:3000", {}, 
                {
                    .receiveHandler_ = [&, id = receiverId++](auto, auto packet, auto)
                            {
                                print(fmt::format("receiver #{} got multicast packet. data = {}", id, std::string_view(packet.data(), packet.size())));
                            },
                    .packetAllocationHandler_ = [](socket_id, std::size_t capacity)
                            {
                                auto allocation = new char[capacity];
                                return packet(
                                        {
                                            .deleteHandler_ = [](auto const & p){delete [] p.data();}
                                        },  std::span(allocation, capacity));
                            }
                });

    // send messages
    for (auto i = 0; i < 100; ++i)
    {
        sender.send(fmt::format("this is a multicast message # {}", i));
        std::this_thread::sleep_for(std::chrono::microseconds(10)); 
    }

    // demo is async so give it a moment to complete
    std::this_thread::sleep_for(std::chrono::seconds(1)); 

    networkInterface.stop();
    
    return 0;
}
