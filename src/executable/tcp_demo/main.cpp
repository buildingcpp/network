#include <library/network.h>

#include <include/non_movable.h>
#include <include/non_copyable.h>

#include <iostream>
#include <memory>
#include <map>

using namespace bcpp;
using namespace bcpp::system;
using namespace bcpp::network;

static ip_address constexpr ipAddress{loop_back};
static port_id constexpr portId{3000};
static socket_address constexpr listenerSocketAddress{ipAddress, portId};


//=============================================================================
class server : non_movable, non_copyable
{
public:

    struct session;

    server()
    {
        // we will use two threads for our example.  one will be responsible for polling the network interface
        pollerThread_ = std::jthread([this](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface_.poll();});
        // the other will be responsible for all of the async socket recv and tcp accepts
        workerThread_ = std::jthread([this](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface_.service_sockets();});
        // create a tcp listener socket
        socket_ = networkInterface_.tcp_listen(     // create a tcp listener socket
                listenerSocketAddress,              // address of this socket
                {                                   
                    .backlog_ = 8                   // configuration of this socket (or use the defaults)
                },
                {
                    .acceptHandler_ = [this]        // set event handlers
                            (
                                auto socketId,      // id of the socket making the callback (the listener socket)
                                auto fileDescriptor // the file descriptor of the newly accepted socket
                            )
                            {
                                on_accept_socket(std::move(fileDescriptor));
                            }
                });
    }

    ~server()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        networkInterface_.stop();
    }


    void on_accept_socket
    (
        auto fileDescriptor // the file descriptor of the newly accepted socket
    )
    {
        // create a new session object.  it will create the tcp socket using the newly accepted file descriptor
        auto s = std::make_unique<session>(networkInterface_, std::move(fileDescriptor), [this](auto const & s){on_session_close(s);});
        std::lock_guard lockGuard(mutex_);
        std::cout << "session " << s->get_id() << " added\n";
        sessions_[s->get_id()] = std::move(s);
    }


    void on_session_close
    (
        session const & s
    )
    {
        std::lock_guard lockGuard(mutex_);
        if (auto iter = sessions_.find(s.get_id()); iter != sessions_.end())
        {
            std::cout << "session " << s.get_id() << " removed\n";
            sessions_.erase(iter);
        }
    }


    struct session : non_movable, non_copyable
    {
        session
        (
            network_interface & networkInterface,
            file_descriptor fileDescriptor,
            std::function<void(session const &)> endSessionHandler 
        ) : endSessionHandler_(endSessionHandler)
        {
            tcpSocket_ = networkInterface.tcp_accept(   // create a new tcp socket instance
                    std::move(fileDescriptor),          // move the newly accepted file descriptor into it
                    tcp_socket::configuration{
                                                        // configure this new tcp socket
                    }, 
                    tcp_socket::event_handlers{
                            .closeHandler_ = [this]          // configure the close handler
                                    (
                                        auto                // id of this socket
                                    )
                                    {
                                        if (endSessionHandler_)
                                            std::exchange(endSessionHandler_, nullptr)(*this);
                                    },
                            .receiveHandler_ = [this]               // configure the packet receive handler
                                    (
                                        auto,                       // id of this new socket
                                        auto receivedPacket,        // the received packet
                                        auto sendersSocketAddress   // the socket address of the partner
                                    )
                                    {
                                        on_receive_packet(std::move(receivedPacket), sendersSocketAddress);
                                    }
                    });
        }

        ~session(){std::cout << "session deleted\n";}

        void on_receive_packet
        (
            packet receivedPacket,
            socket_address sendersSocketAddress
        )
        {
            std::cout << "session " << get_id() << " received message from " << sendersSocketAddress <<
                    ".  Message is \"" << std::string_view((char const *)receivedPacket.data(), receivedPacket.size()) << "\"\n";
        }

        socket_id get_id() const{return tcpSocket_.get_id();}

        tcp_socket                              tcpSocket_;
        std::function<void(session const &)>    endSessionHandler_;
    };


    std::function<void(session const &)>            closeHandler_;
    network_interface                               networkInterface_;
    tcp_listener_socket                             socket_;
    std::mutex                                      mutex_;
    std::map<socket_id, std::unique_ptr<session>>   sessions_;
    std::jthread                                    pollerThread_;
    std::jthread                                    workerThread_;
};


//=============================================================================
struct client : non_movable, non_copyable
{
    client(socket_address serverAddress):
        socket_(networkInterface_.tcp_connect(
                loop_back, serverAddress, {}, 
                {.receiveHandler_ = [&](auto, auto packet, auto){std::cout << std::string(packet.data(), packet.size()) << std::flush;}})),
        pollerThread_([this](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface_.poll();}),
        workerThread_([this](std::stop_token const & stopToken){while (!stopToken.stop_requested()) networkInterface_.service_sockets();}){}

    ~client()
    {
        networkInterface_.stop();
    }

    void send(std::span<char const> data){socket_.send(data);}

    network_interface   networkInterface_;
    tcp_socket          socket_;
    std::jthread        pollerThread_;
    std::jthread        workerThread_;
};


//=============================================================================
int main
(
    int,
    char **
)
{

    server myServer;
    client myClient(listenerSocketAddress);
    myClient.send("this is the message");
    std::this_thread::sleep_for(std::chrono::seconds(1)); // demo is async so give it a moment to complete
    return 0;
}
