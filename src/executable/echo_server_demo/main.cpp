#include "./server.h"
#include "./client.h"

#include <chrono>
#include <format>


//=============================================================================
int main
(
    int,
    char **
)
{
    using namespace bcpp::network::literals;
    using namespace std::chrono;

    // this example will use 'any' interface. 
    echo_server echoServer(bcpp::network::network_interface_configuration{.ipAddress_ = bcpp::network::in_addr_any}, 3000_port);
    echo_client echoClient(bcpp::network::network_interface_configuration{.ipAddress_ = bcpp::network::in_addr_any}, {echoServer.get_ip_address(), 3000_port});

    for (auto i = 0; i < 100; ++i)
        while (!echoClient.send(std::format("message {}\n", i)));

    std::this_thread::sleep_for(1s); // demo is async so give it a moment to complete
    return 0;
}
