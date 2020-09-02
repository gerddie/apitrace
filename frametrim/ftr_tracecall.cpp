#include "ftr_tracecall.hpp"

namespace frametrim_reverse {

TraceCall::TraceCall(const trace::Call &call):
    m_trace_call_no(call.no),
    m_required(false)
{
}

void TraceCall::set_required()
{
    m_required = true;
}


TraceCallOnBoundObj::TraceCallOnBoundObj(const trace::Call& call, PGenObject obj):
    TraceCall(call),
    m_object(obj)
{

}

}
