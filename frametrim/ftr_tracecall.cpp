#include "ftr_tracecall.hpp"

namespace frametrim_reverse {

TraceCall::TraceCall(unsigned callno, const std::string& name):
    m_trace_call_no(callno),
    m_name(name),
    m_required(false)
{

}

TraceCall::TraceCall(const trace::Call &call):
    TraceCall(call.no, call.name())
{
}

void TraceCall::set_required()
{
    m_required = true;
}

void TraceCall::add_object_to_set(ObjectSet& out_set) const
{
    add_owned_object(out_set);
    add_dependend_objects(out_set);
}


void TraceCall::add_owned_object(ObjectSet& out_set) const
{

}

void TraceCall::add_dependend_objects(ObjectSet& out_set) const
{

}
}
