#include "ftr_genobject.hpp"
#include "ftr_tracecall.hpp"
namespace frametrim_reverse {

using std::make_shared;


GenObject::GenObject(unsigned id, PTraceCall gen_call):
    m_id(id),
    m_gen_call(gen_call)
{
}

void GenObject::collect_generate_call(CallSet& calls)
{
    if (m_gen_call)
        calls.insert(m_gen_call);
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

void BindTimeline::collect_currently_active(ObjectSet& objects) const
{
    if (!m_timeline.empty() && m_timeline.front().obj)
        objects.push(m_timeline.front().obj);
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