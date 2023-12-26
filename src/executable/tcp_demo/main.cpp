#include "./server.h"
#include "./client.h"

#include <iostream>
#include <thread>
#include <chrono>


//=============================================================================
int main
(
    int,
    char **
)
{
    using namespace bcpp::literals;
    using namespace std::chrono;

    server myServer("wlp5s0", 3000_port);
    client myClient("wlp5s0", {myServer.get_ip_address(), 3000_port});

    myClient.send("this is the message");

    std::this_thread::sleep_for(100ms); // demo is async so give it a moment to complete
    return 0;
}
