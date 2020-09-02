#include "ftr_tracecall.hpp"

namespace frametrim_reverse {

TraceCall::TraceCall(PCall call):
    m_trace_call_no(call->no)
{
}

TraceCallWithBinding::TraceCallWithBinding(PCall call, unsigned bind_id):
    TraceCall(call),
    m_bind_id(bind_id)
{
}

}
