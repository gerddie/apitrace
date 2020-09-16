#include "ftr_traceobject.hpp"
#include "ftr_tracecall.hpp"

namespace frametrim_reverse {

TraceObject::TraceObject():
    m_visited(std::numeric_limits<unsigned>::max())
{
}

void TraceObject::collect_objects_of_type(Queue& objects, unsigned call,
                                          TypeFlags  typemask)
{
    (void)objects;
    (void)call;
    (void)typemask;
}

void TraceObject::collect_calls(CallSet& calls, unsigned call_before)
{
    if (m_visited > call_before) {
        m_visited = call_before;
        collect_generate_call(calls);
        collect_allocation_call(calls);
        collect_data_calls(calls, call_before);
        collect_bind_calls(calls, call_before);
        collect_state_calls(calls, call_before);
    }
}

void TraceObject::collect_generate_call(CallSet& calls)
{
    (void)calls;
}

void TraceObject::collect_allocation_call(CallSet& calls)
{
    (void)calls;
}

void TraceObject::collect_data_calls(CallSet& calls, unsigned call_before)
{
    (void)calls;
    (void)call_before;
}

void TraceObject::collect_state_calls(CallSet& calls, unsigned call_before)
{
    (void)calls;
    (void)call_before;
}

unsigned TraceObject::collect_last_call_before(CallSet& calls,
                                         const std::list<PTraceCall>& call_list,
                                         unsigned call_before)
{
    for (auto&& c : call_list)
        if (c->call_no() < call_before) {
            calls.insert(c);
            return c->call_no();
        }
    return call_before;
}

void TraceObject::collect_all_calls_before(CallSet& calls,
                                         const std::list<PTraceCall> &call_list,
                                         unsigned call_before)
{
    for (auto& c : call_list)
        if (c->call_no() < call_before) {
            calls.insert(c);
        }
}


void TraceObject::collect_bind_calls(CallSet& calls, unsigned call_before)
{
    (void)calls;
    (void)call_before;
}

}