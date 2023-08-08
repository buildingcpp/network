#include "./port_id.h"


//=============================================================================
bcpp::network::port_id::port_id
(
    std::string const & value
)
{
    // TODO: handle invalid value
    std::from_chars(value.data(), value.data() + value.size(), value_);
}
