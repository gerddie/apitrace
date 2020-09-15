#include "ftr_genobject.hpp"
#include "ftr_tracecall.hpp"
namespace frametrim_reverse {

using std::make_shared;

TraceObject::TraceObject():
    m_visited(std::numeric_limits<unsigned>::max())
{
}

void TraceObject::collect_objects(Queue& objects, const TraceCallRange& call_range)
{
    collect_owned_obj(objects, call_range);
    collect_dependend_obj(objects, call_range);
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

void TraceObject::collect_dependend_obj(Queue& objects, const TraceCallRange &call_range)
{
    (void)objects;
    (void)call_range;
}

void TraceObject::collect_owned_obj(Queue& objects, const TraceCallRange &call_range)
{
    (void)objects;
    (void)call_range;
}

unsigned TraceObject::collect_last_call_before(CallIdSet& calls,
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

void TraceObject::collect_all_calls_before(CallIdSet& calls,
                                         const std::list<PTraceCall> &call_list,
                                         unsigned call_before)
{
    for (auto& c : call_list)
        if (c->call_no() < call_before)
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
    if (m_gen_call)
        calls.insert(m_gen_call);
}

void BoundObject::collect_bind_calls(CallIdSet& calls, unsigned call_before)
{
    collect_last_call_before(calls, m_bind_calls, call_before);
}

PTraceObject BindTimeline::push(unsigned callno, PTraceObject obj)
{
    PTraceObject last = nullptr;
    if (!m_timeline.empty()) {
        m_timeline.front().unbind_call_no = callno;
        last = m_timeline.front().obj;
    }

    m_timeline.push_front(BindTimePoint(obj, callno));
    return last;
}

PTraceObject BindTimeline::unbind_last(unsigned callno)
{
    if (!m_timeline.empty()) {
        m_timeline.front().unbind_call_no = callno;
        return m_timeline.front().obj;
    }
    return nullptr;
}

PTraceObject BindTimeline::active_in_call_range(const TraceCallRange &call_range) const
{
    for (auto&& tp: m_timeline) {
        if (tp.bind_call_no < call_range.second &&
            tp.unbind_call_no >call_range.first)
            return tp.obj;
    }
    return nullptr;
}

PTraceObject BindTimeline::active_at_call(unsigned no) const
{
    for (auto&& tp: m_timeline) {
        if (tp.bind_call_no <= no  &&
            tp.unbind_call_no > no)
            return tp.obj;
    }
    return nullptr;
}

void BindTimeline::collect_active_in_call_range(ObjectSet& objects, const TraceCallRange &call_range) const
{
    for (auto&& tp: m_timeline) {
        if (tp.bind_call_no < call_range.second &&
            tp.unbind_call_no > call_range.first &&
            tp.obj)
            if (!tp.obj->visited(call_range.first))
                objects.push(tp.obj);
    }
}

}