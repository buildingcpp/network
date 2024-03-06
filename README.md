<img src="https://github.com/buildingcpp/network/actions/workflows/network.yml/badge.svg?branch=main">


# network - a simple, easy to use async network library using work contracts
lots of logging, documentation and error handling to be added

# quick start

# Virtual Network Interfaces
All sockets are created and managed through a `virtual_network_interface`.  A Virtual network interface
is associated with one of the local host's physical network interfaces and all sockets created via that
virtual network interface will be associated with that same physical network interface.

For example, to create a `virtual_network_interface` on the loopback interface:

```
   bcpp::network::virtual_network_interface virtualNetworkInterface({.physicalNetworkInterfaceName_ = "lo"});
   if (!virtualNetworkInterface.is_valid())
   {
      sdt::cerr << "Failed to create virtual network interface\n";
      return -1; // bad interface
   }
   std::cout << "virtual network interface successfully created\n";
```

Once a `virtual_network_interface` has been created, it can be used to create TCP listener sockets, TCP sockets,
and UDP sockets.


        tcp_listener_socket tcp_listen
        (
            port_id,
            tcp_listener_socket::configuration,
            tcp_listener_socket::event_handlers
        );

        tcp_socket tcp_accept
        (
            system::file_descriptor,
            tcp_socket::configuration,
            tcp_socket::event_handlers
        );

        tcp_socket tcp_connect
        (
            socket_address,
            tcp_socket::configuration,
            tcp_socket::event_handlers
        );

        udp_socket udp_connect
        (
            port_id,
            socket_address,
            udp_socket::configuration,
            udp_socket::event_handlers
        );

        udp_socket udp_connect
        (
            socket_address,
            udp_socket::configuration,
            udp_socket::event_handlers
        );

        udp_socket udp_connectionless
        (
            port_id,
            udp_socket::configuration,
            udp_socket::event_handlers
        );

        udp_socket udp_connectionless
        (
            udp_socket::configuration,
            udp_socket::event_handlers
        );

        udp_socket multicast_join
        (
            socket_address,
            udp_socket::configuration,
            udp_socket::event_handlers
        );

To create a TCP socket on this virtual network interface and connect it to port 5000 on the local host:

```
    using namespace std::string_literals;
    auto tcpSocket = virtualNetworkInterface.tcp_connect("127.0.0.1:5000"s, {}, {});
    if (!tcpSocket.is_valid())
    {
        std::cerr << "Failed to create TCP socket\n";
        return -1;
    }
    std::cout << "TCP socket created\n";
```

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
