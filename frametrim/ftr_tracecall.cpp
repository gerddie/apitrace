#include "ftr_tracecall.hpp"

namespace frametrim_reverse {

TraceCall::TraceCall(const trace::Call &call):
    m_trace_call_no(call.no),
    m_name(call.name()),
    m_required(false)
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
