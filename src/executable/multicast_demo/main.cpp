#include <library/network.h>
#include <library/system.h>

#include <array>
#include <iostream>


//=============================================================================
int main
(
    int,
    char **
)
{
    using namespace std::chrono;

    // set up network interface
    if (bcpp::network::virtual_network_interface networkInterface({.physicalNetworkInterfaceName_ = "lo"}); networkInterface.is_valid())
    {
        bcpp::network::socket_address multicastChannel = "239.0.0.1:3000"; 
        // set up threads
        bcpp::system::thread_pool threadPool
        ({
            // one polling thread
            {.function_ = [&](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface.poll();}},
            // three socket reader threads (abitrary amount to prove concept)
            {.function_ = [&](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface.service_sockets();}},
            {.function_ = [&](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface.service_sockets();}},
            {.function_ = [&](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface.service_sockets();}}
        });

        // set up multicast receiver sockets
        static auto constexpr number_of_receivers = 10;
        std::array<bcpp::network::udp_socket, number_of_receivers> receivers;
        auto receiverId = 0;
        for (auto & receiver : receivers)
            receiver = networkInterface.multicast_join(multicastChannel, {}, 
                    {
                        .receiveHandler_ = [id = receiverId++](auto, auto packet, auto)
                                {
                                    // special print function with locking just so that demo output doesn't get all mashed up
                                    auto print = [](std::string_view const input)
                                            {
                                                static std::mutex mutex;
                                                std::lock_guard lockGuard(mutex);
                                                std::cout << input << '\n';
                                            };
                                    print(fmt::format("receiver #{} got multicast packet. data = {}", id, std::string_view(packet.data(), packet.size())));
                                },
                        .packetAllocationHandler_ = [](bcpp::network::socket_id, std::size_t capacity)
                                {
                                    // new/delete here are overkill but demonstrate where an allocator would fit in
                                    return bcpp::network::packet({.deleteHandler_ = [](auto const & p){delete [] p.data();}}, std::span(new char[capacity], capacity));
                                }
                    });

        // set up multicast sender socket and then send messages
        auto sender = networkInterface.udp_connect(multicastChannel, {}, {});
        for (auto i = 0; i < 1024; ++i)
        {
            sender.send(fmt::format("this is a multicast message # {}", i));
            std::this_thread::sleep_for(10us); 
        }

        // demo is async so give it a moment to complete
        std::this_thread::sleep_for(1s); 
    }
    else
    {
        std::cout << "Failed to get network interface\n";
        return -1;
    }
    return 0;
}
