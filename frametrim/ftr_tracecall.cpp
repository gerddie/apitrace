#include "ftr_tracecall.hpp"

namespace frametrim_reverse {

TraceCall::TraceCall(unsigned callno, const std::string& name,
                     bool is_state_call):
    m_trace_call_no(callno),
    m_name(name)
{
    if (is_state_call)
        m_flags.set(single_state);
}

TraceCall::TraceCall(const trace::Call &call):
    TraceCall(call.no, call.name(), false)
{
}

void TraceCall::add_object_to_set(ObjectSet& out_set) const
{
    add_owned_object(out_set);
    add_dependend_objects(out_set);
}


void TraceCall::add_owned_object(ObjectSet& out_set) const
{
    (void)out_set;
}

void TraceCall::add_dependend_objects(ObjectSet& out_set) const
{
    (void)out_set;
}

}
