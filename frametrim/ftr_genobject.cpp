#include "ftr_genobject.hpp"
#include "ftr_tracecall.hpp"
namespace frametrim_reverse {

using std::make_shared;

TraceObject::TraceObject():
    m_visited(false)
{
}

void TraceObject::collect_objects(Queue& objects)
{
    collect_owned_obj(objects);
    collect_dependend_obj(objects);
}

void TraceObject::collect_calls(CallIdSet& calls, unsigned call_before)
{
    collect_generate_call(calls);
    collect_allocation_call(calls);
    collect_data_calls(calls, call_before);
    collect_bind_calls(calls, call_before);
    collect_state_calls(calls, call_before);
}

void TraceObject::collect_generate_call(CallIdSet& calls)
{
    (void)calls;
}

void TraceObject::collect_allocation_call(CallIdSet& calls)
{
    (void)calls;
}

void TraceObject::collect_data_calls(CallIdSet& calls, unsigned call_before)
{
    (void)calls;
    (void)call_before;
}

void TraceObject::collect_state_calls(CallIdSet& calls, unsigned call_before)
{
    (void)calls;
    (void)call_before;
}

void TraceObject::collect_dependend_obj(Queue& objects)
{
    (void)objects;
}

void TraceObject::collect_owned_obj(Queue& objects)
{
    (void)objects;
}

void TraceObject::collect_last_call_before(CallIdSet& calls,
                                         const std::vector<unsigned>& call_list,
                                         unsigned call_before)
{
    for (auto c = call_list.rbegin(); c != call_list.rend(); ++c)
        if (*c < call_before) {
            calls.insert(*c);
            return;
        }
}

void TraceObject::collect_all_calls_before(CallIdSet& calls,
                                         const std::vector<unsigned>& call_list,
                                         unsigned call_before)
{
    for (auto& c : call_list)
        if (c < call_before)
            calls.insert(c);
}

void TraceObject::collect_bind_calls(CallIdSet& calls, unsigned call_before)
{
    (void)calls;
    (void)call_before;
}

GenObject::GenObject(unsigned id, PTraceCall gen_call):
    m_id(id),
    m_gen_call(gen_call)
{
}

void GenObject::collect_generate_call(CallIdSet& calls)
{
    calls.insert(m_gen_call->call_no());
}


void BoundObject::collect_bind_calls(CallIdSet& calls, unsigned call_before)
{
    collect_last_call_before(calls, m_bind_calls, call_before);
}

}