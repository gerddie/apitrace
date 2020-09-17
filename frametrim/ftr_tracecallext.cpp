#include "ftr_tracecallext.hpp"

#include <sstream>
#include <iostream>

namespace frametrim_reverse {

StateCall::StateCall(const trace::Call& call, unsigned num_param_ids):
    TraceCall(call, combined_name(call, num_param_ids), true)
{

}

std::string
StateCall::combined_name(const trace::Call& call,
                         unsigned num_param_ids)
{
    std::stringstream s;

    s << call.name();
    for (auto i = 0u; i < num_param_ids; ++i)
        s << "_" << call.arg(i).toUInt();
    return s.str();
}

StateEnableCall::StateEnableCall(const trace::Call& call, const char *basename):
    TraceCall(call, combined_name(call, basename), true)
{

}

std::string
StateEnableCall::combined_name(const trace::Call& call, const char *basename)
{
    std::stringstream s;
    s << basename << "_" << call.arg(0).toUInt();
    return s.str();
}

BufferSubrangeCall::BufferSubrangeCall(const trace::Call& call, uint64_t start,
                                       uint64_t end):
    TraceCall(call),
    m_start(start),
    m_end(end)
{
}


}