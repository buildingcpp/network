#include <library/network.h>
#include <library/system.h>

#include <array>
#include <iostream>


//=============================================================================
auto select_network_interface
(
    bool loopback
) -> bcpp::network::network_interface_name
{
    std::string networkInterfaceName;
    for (auto && [name, ipAddress, netmask] : bcpp::network::get_available_network_interfaces())
        if (ipAddress.is_valid() && (ipAddress.is_loop_back() == loopback))
            return name;
    return {};
}


//=============================================================================
int main
(
    int,
    char **
)
{
    using namespace std::chrono;

    // find a suitable network interface (a loop back interface for this example)
    auto useLoopback = true;
    auto networkInterfaceName = select_network_interface(useLoopback);
    if (networkInterfaceName.empty())
    {
        std::cerr << "Failed to locate suitable network interface for test\n";
        return -1;
    }
    std::cout << "using network interface " << networkInterfaceName << "\n";

    // set up network interface
    if (bcpp::network::virtual_network_interface networkInterface({.physicalNetworkInterfaceName_ = networkInterfaceName}); networkInterface.is_valid())
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
        auto sender = networkInterface.create_udp_socket({}, {});
        sender.connect_to(multicastChannel);
        for (auto i = 0; i < 1024; ++i)
        {
            bcpp::network::packet p(fmt::format("this is a multicast message # {}", i));
            sender.send(std::move(p));
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
