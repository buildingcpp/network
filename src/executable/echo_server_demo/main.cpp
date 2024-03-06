#include "./server.h"
#include "./client.h"

#include <chrono>
#include <fmt/format.h>


//=============================================================================
int main
(
    int,
    char **
)
{
    using namespace bcpp::network::literals;
    using namespace std::chrono;

    // this example will not define a network interface.  Therefore use 'any' interface.
    echo_server echoServer({}, 3000_port);
    echo_client echoClient({}, {echoServer.get_ip_address(), 3000_port});

    for (auto i = 0; i < 100; ++i)
        while (!echoClient.send(fmt::format("message {}\n", i)));

    std::this_thread::sleep_for(1s); // demo is async so give it a moment to complete
    return 0;
}
