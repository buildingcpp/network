<img src="https://github.com/buildingcpp/network/actions/workflows/network.yml/badge.svg?branch=main">


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


