#pragma once

#include <include/non_copyable.h>
#include <library/system.h>
#include <include/file_descriptor.h>

#include <memory>


namespace bcpp::network
{

    class poller;


    class poller_registration :
        non_copyable,
        non_movable
    {
    public:

        poller_registration
        (
            std::weak_ptr<poller>,
            system::file_descriptor const &
        );

        ~poller_registration();

        void release();

    private:

        std::weak_ptr<poller>   poller_;

        system::file_descriptor const & fileDescriptor_;

    }; // class poller_registration

} // namespace bcpp::network
