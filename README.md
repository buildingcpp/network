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
