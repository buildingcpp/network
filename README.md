
# network - a simple, easy to use async network library using work contracts
lots of logging, documentation and error handling to be added

# notes on the multicast example
The multicast example relies on the ip address `239.0.0.1`.  It will not work without it.
If you want to run this example check for the `239.0.0.1` using `ip address` to list
all interfaces and ip addresses.

Example:

```
~$ ip address
<SNIP>
3: wlp5s0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether xx:xx:xx:xx:xx:xx brd ff:ff:ff:ff:ff:ff
    inet 192.168.1.161/24 brd 192.168.1.255 scope global dynamic noprefixroute wlp5s0
       valid_lft 84299sec preferred_lft 84299sec
</SNIP>
```

To add `239.0.0.1`:  (in my case to interface `wlp5s0`)

```
~$ sudo ip addr add 239.0.0.1/32 dev wlp5s0 
```

Verify that `239.0.0.1` now exits:

```
$ ip address
<SNIP>
3: wlp5s0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether xx:xx:xx:xx:xx:xx brd ff:ff:ff:ff:ff:ff
    inet 192.168.1.161/24 brd 192.168.1.255 scope global dynamic noprefixroute wlp5s0
       valid_lft 84116sec preferred_lft 84116sec
    inet 239.0.0.1/32 scope global wlp5s0           <----- ip was added
       valid_lft forever preferred_lft forever
</SNIP>
```

The multicast demo should now (hopefully) function correctly.

To remove `239.0.0.1`: (in my case from interface `wlp5s0`)

```
~$ sudo ip addr del 239.0.0.1/32 dev wlp5s0
```


# Virtual Network Interface

Virtual network interfaces are the foundation for every other component of this networking library.  A virtual network interface is associated with any one specific physical network interface and is 'virtual' because it is possible to create multiple virtual network interfaces, each of which is associated with the same underlying physical network interface.

Virtual network interfaces provide the ability to create sockets and to poll those sockets.  All sockets created by an instance of a virtual network interface are associated exclusively with that instance and polling a virtual network interface instance will only poll for those specific sockets.

Examples of creating virtual network interfaces:

**Create a virtual network interface which is not specifically assoicated with any physical network interface:** 
```
using namespace bcpp::network;


int main
(
    int,
    char **
)
{
    virtual_network_interface virtualNetworkInterface;
    return 0;
}
```

**Create a virtual network interface which is associated with a specific network interface: (In this example, the loopback interface)**
```
using namespace bcpp::network;


auto get_network_interface
(
    network_interface_name networkInterfaceName
) -> network_interface_configuration
{
    for (auto const & networkInterfaceConfiguration : get_available_network_interfaces())
        if (networkInterfaceConfiguration.name_ == networkInterfaceName)
            return networkInterfaceConfiguration;
    return {};
}


int main
(
    int,
    char **
)
{
    auto networkInterfaceConfiguration = get_network_interface("lo");
    virtual_network_interface virtualNetworkInterface({.networkInterfaceConfiguration_ = networkInterfaceConfiguration});
    if (!virtualNetworkInterface.is_valid())
    {
        std::cerr << "Failed to create virtual network interface\n";
        return -1;
    }
    return 0;
}
```

Sockets are then created via the virtual network interface.

**Create a TCP listener socket:**
```
auto bcpp::network::virtual_network_interface::create_tcp_socket
(
    tcp_listener_socket::configuration,
    tcp_listener_socket::event_handlers
) -> tcp_listener_socket;
```

**Create a TCP socket using an accepted file descriptor:**
```
auto bcpp::network::virtual_network_interface::accept_tcp_socket
(
    system::file_descriptor,
    tcp_socket::configuration,
    tcp_socket::event_handlers
) -> tcp_socket;
```

**Create a TCP socket and connect to the specified socket address:**
```
auto bcpp::network::virtual_network_interface::create_tcp_socket
(
    socket_address,
    tcp_socket::configuration,
    tcp_socket::event_handlers
) -> tcp_socket;
```

**Create a UDP socket and associate with the specified port:**
```
auto bcpp::network::virtual_network_interface::create_udp_socket
(
    port_id,
    udp_socket::configuration,
    udp_socket::event_handlers
) -> udp_socket;
```

**Create a UDP socket with an arbitrary port:**
```
auto bcpp::network::virtual_network_interface::create_udp_socket
(
    udp_socket::configuration,
    udp_socket::event_handlers
) -> udp_socket;
```

**Create a UDP socket and join the specified multicast address:**
```
auto bcpp::network::virtual_network_interface::multicast_join
(
    socket_address,
    udp_socket::configuration,
    udp_socket::event_handlers
) -> udp_socket;
```

# Sending and receiving data:

This networking library supports asynchronous send and receive.  To poll sockets created by any given `virtual_network_interface` is done by invoking `virtual_network_interface::poll()`. Any sockets which have packets to receive will be scheduled (see `work_contract` library for details) to receive data asynchronously.

Receiving packets on sockets which have been scheduled to receive data is done by invoking `virtual_network_interface::service_sockets()`. Packets are then returned via the socket's `receive_handler` callback (which is assigned during socket creation. See below.) 

Sending packets is also done asynchronously.  As such, invoking `socket::send(packet)` only queues the packet to be sent.  Packets are actually sent when `virtual_network_interface::service_sockets()` is invoked.

**Notes:**
Both `virtual_network_interface::poll()` and `virtual_network_interface::service_sockets()` are thread-safe functions.
The act of polling is intentionally decoupled from the act of sending/receiving packets. 
This architecture allows for many different usage cases.  

For general single threaded usage:
```
while (!done)
{
    virtualNetworkInterface.poll();
    virtualNetworkInterface.service_sockets();
}
```

For prioritizing polling on an issolated thread:
```
std::thread poller([&](std::stop_token const & stopToken)
        {
            while (!stopToken.stop_requested())
                virtualNetworkInterface.poll();
        });

std::thread worker([&](std::stop_token const & stopToken)
       {
            while (!stopToken.stop_requested())
                virtualNetworkInterface.service_sockets();
       });
```

For 'receive side scaling':
```
std::vector<std::thread> pollers(num_pollers);
for (auto & poller : pollers)
    poller = std::move(std::thread([&](std::stop_token const & stopToken)
        {
            while (!stopToken.stop_requested())
                virtualNetworkInterface.poll();
        }));

std::vector<std::thread> workers(num_workers);
for (auto & worker : workers)
    worker = std::move(std::thread worker([&](std::stop_token const & stopToken)
       {
            while (!stopToken.stop_requested())
                virtualNetworkInterface.service_sockets();
       }));
```

#Socket creation: `configuration` and `event_handlers`
Socket configuration is acheived by providing `socket::configuration` and `socket::event_handlers` when creating the socket via one of the `virtual_network_interface::create_***_socket()` functions listed above.

`event_handlers` (callbacks) are not only a design choice but are necessary because the library is asynchronous.  Therefore, receiving packets as well as receiving send completion notifications must be done via `event_handlers`.

**Creating and configuring a TCP listener socket and creating accepted sockets:**

```
struct bcpp::network::tcp_listener_socket::configuration
{
    port_id         portId_;                       // the port id for this socket
    std::uint32_t   backlog_{default_backlog};     // accept backlog capacity
};
```

```
struct bcpp::network::tcp_listener_socket::event_handlers
{
    using close_handler = std::function<void(socket_id)>;                           // optional socket close callback
    using poll_error_handler = std::function<void(socket_id)>;                      // optional error callback
    using accept_handler = std::function<void(socket_id, system::file_descriptor)>; // required accept callback

    close_handler           closeHandler_;
    poll_error_handler      pollErrorHandler_;
    accept_handler          acceptHandler_;
};
```

```
void on_accept
(
    auto socketId,
    auto fileDescriptor
)
{
    bcpp::network::tcp_socket::configuration acceptedSocketConfiguration;   // configured as needed 
    bcpp::network::tcp_socket::event_handlers acceptedSocketEventHandlers;  // configured as needed
    auto acceptedTcpSocket = virtualNetworkInterface.create_tcp_socket(std::move(fileDescriptor),
            acceptedSocketConfiguration, acceptedSocketEventHandlers);
}


auto tcpListenerSocket = virtualNetworkInterface.create_tcp_socket(
        {
            bcpp::network::tcp_listener_socket::configration{.portId_ = 10000_port},            // configuration
            bcpp::network::tcp_listener_socket::event_handlers{.acceptHandler_ = on_accept}     // event handlers
        });
```


**Creating and configuring a TCP sockets and connecting to listener sockets:**

```
struct bcpp::network::tcp_socket::configuration
{
    std::size_t socketReceiveBufferSize_{0};                // socket recv buffer size (0 = default)
    std::size_t socketSendBufferSize_{0};                   // socket send buffer size (0 = default)
    std::size_t readBufferSize_{0};                         // max bytes to receive per recv call (receive packet's capacity)
    std::size_t sendQueueSize_{0};                          // capacity of async send packet queue
    system::io_mode ioMode_{system::io_mode::read_write};   // socket read/write mode
};
```

```
struct bcpp::network::tcp_socket::event_handlers
{
    using close_handler = std::function<void(socket_id)>;                               
    using poll_error_handler = std::function<void(socket_id)>;                          
    using hang_up_handler = std::function<void(socket_id)>;                             
    using peer_hang_up_handler = std::function<void(socket_id)>;                        
    using receive_handler = std::function<void(socket_id, packet, socket_address)>;     
    using receive_error_handler = std::function<void(socket_id, std::int32_t)>;         
    using packet_allocation_handler = std::function<packet(socket_id, std::size_t)>;    

    close_handler               closeHandler_;              // optional close callback
    poll_error_handler          pollErrorHandler_;          // optional poll error callback
    receive_handler             receiveHandler_;            // optional hangup callback
    receive_error_handler       receiveErrorHandler_;       // optional peer hangup callback
    packet_allocation_handler   packetAllocationHandler_;   // requried packet receive callback
    hang_up_handler             hangUpHandler_;             // optional receive error callback
    peer_hang_up_handler        peerHangUpHandler_;         // optional packet allocator callback
};
```


#[WIP]
