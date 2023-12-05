#include "./server.h"
#include "./client.h"

#include <chrono>
#include <fmt/format.h>
#include <thread>


//=============================================================================
int main
(
    int,
    char **
)
{
    using namespace bcpp::literals;
    using namespace std::chrono;

    echo_server echoServer(3000_port);
    echo_client echoClient({echoServer.get_ip_address(), 3000_port});

    for (auto i = 0; i < 100; ++i)
        echoClient.send(fmt::format("message {}\n", i));

    std::this_thread::sleep_for(1s); // demo is async so give it a moment to complete
    return 0;
}
