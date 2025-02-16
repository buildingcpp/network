#include "./passive_socket_impl.h"


//=============================================================================
bcpp::network::passive_socket_impl::socket_impl
(
    socket_address socketAddress,
    configuration const & config,
    event_handlers const & eventHandlers,
    work_contract_group & workContractGroup,
    std::shared_ptr<poller> & p
) :    
    socket_base_impl(socketAddress, {.ioMode_ = config.ioMode_}, eventHandlers, ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP),
            workContractGroup.create_contract([this](){this->accept();}, [this](){this->destroy();})),
    poller_(p),
    acceptHandler_(eventHandlers.acceptHandler_)
{
    p->register_socket(*this);
    ::listen(fileDescriptor_.get(), config.backlog_);
}


//=============================================================================
void bcpp::network::passive_socket_impl::accept
(
)
{
    ::sockaddr address;
    socklen_t addressLength = sizeof(address);
    system::file_descriptor fileDescriptor(::accept(fileDescriptor_.get(), &address, &addressLength));
    if (fileDescriptor.is_valid())
    {
        if (acceptHandler_)
            acceptHandler_(id_, std::move(fileDescriptor));
        on_polled(); // could be more
    }
}


//=============================================================================
void bcpp::network::passive_socket_impl::destroy
(
    // use the work contract to asynchronously delete 'this'.
    // doing it this way ensures that the work contract's primary
    // work can not be executed any longer just prior to deleting
    // this.  This allows the primary work contract function to 
    // use a raw 'this' 
)
{
    if (receiveContract_.is_valid())
    {
        // remove this socket from the poller
        if (auto poller = poller_.lock(); poller)
            poller->unregister_socket(*this);
        receiveContract_.release();
    }
    else
    {
        delete this;
    }
}
