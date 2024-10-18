#include <library/network.h>
#include <library/system.h>

#include <array>
#include <iostream>


//=============================================================================
auto select_network_interface
(
    bool loopback
) -> bcpp::network::network_interface_configuration
{
    for (auto const & networkInterfaceConfiguration : bcpp::network::get_available_network_interfaces())
        if (networkInterfaceConfiguration.loopback_ == loopback)
            return networkInterfaceConfiguration;
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
    auto networkInterfaceConfiguration = select_network_interface(useLoopback);
    if (!networkInterfaceConfiguration.ipAddress_.is_valid())
    {
        std::cerr << "Failed to locate suitable network interface for test\n";
        return -1;
    }
    std::cout << "using network interface " << networkInterfaceConfiguration.name_ << "\n";

    // set up network interface
    if (bcpp::network::virtual_network_interface networkInterface({.networkInterfaceConfiguration_ = networkInterfaceConfiguration}); networkInterface.is_valid())
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

        bcpp::network::buffer_heap bufferHeap({.capacity_ = 1 << 20});

        // set up multicast receiver sockets
        static auto constexpr number_of_receivers = 10;
        std::array<bcpp::network::udp_socket, number_of_receivers> receivers;
        auto receiverId = 0;
        for (auto & receiver : receivers)
        {
            receiver = networkInterface.multicast_join(multicastChannel, {}, 
                    {
                        .receiveHandler_ = [id = receiverId++, expected = 0]  
                                (
                                    auto, 
                                    auto packet, 
                                    auto
                                ) mutable
                                {
                                    // special print function with locking just so that demo output doesn't get all mashed up
                                    auto print = [](std::string_view const input)
                                            {
                                                static std::mutex mutex;
                                                std::lock_guard lockGuard(mutex);
                                                std::cout << input << '\n';
                                            };
                                    auto received = std::atoi(packet.data());
                                    if (expected != received)
                                        print(std::format("UDP loss *** Expected {} but got {}", expected, received));
                                    else
                                        print(std::format("receiver #{} got multicast packet - data = {}", id, std::string_view(packet.data(), packet.size())));
                                    expected = received + 1;
                                },
                        .packetAllocationHandler_ = [&]
                                (
                                    bcpp::network::socket_id, 
                                    std::size_t
                                )
                                {
                                    return bcpp::network::packet(bufferHeap);
                                }
                    });
        }

        // set up multicast sender socket and then send messages
        auto sender = networkInterface.create_udp_socket({}, {});
        sender.connect_to(multicastChannel);
        for (auto i = 0; i < 1000; ++i)
        {
            auto message = std::format("{}", i);
            bcpp::network::packet packet(bufferHeap);
            std::copy_n(message.data(), message.size(), packet.data());
            packet.resize(message.size());

            while (!sender.send(std::move(packet)))
                ;
        }

        // demo is async so give it a moment to complete
        std::this_thread::sleep_for(1s); 

        threadPool.stop();
        networkInterface.stop();
    }
    else
    {
        std::cout << "Failed to get network interface\n";
        return -1;
    }
    return 0;
}
