#include "ftr_attachableobject.hpp"

namespace frametrim_reverse {

using std::make_shared;

PTraceCall
AttachableObject::allocate(const trace::Call& call)
{
    unsigned level = evaluate_size(call);

    if (m_allocation_call.size() <= level)
        m_allocation_call.resize(level + 1);

    auto c = make_shared<TraceCall>(call);
    m_allocation_call[level] = c;
    return c;
}

void AttachableObject::collect_allocation_call(CallSet& calls)
{
    for (auto&& c: m_allocation_call) {
        calls.insert(c);
        collect_last_call_before(calls, m_bind_calls, c->call_no());
    }
}

void AttachableObject::set_size(unsigned level, unsigned w, unsigned h)
{
    if (m_width.size() <= level)
        m_width.resize(level +1);
    if (m_heigth.size() <= level)
        m_heigth.resize(level +1);

    m_width[level] = w;
    m_heigth[level] = h;
}


void AttachableObject::attach_to(PGenObject obj, unsigned att_point, unsigned call_no)
{
    auto& timeline = m_bindings[64 * obj->id() + att_point];
    timeline.push(call_no, obj);
}

void AttachableObject::detach_from(unsigned fbo_id, unsigned att_point, unsigned call_no)
{
    auto& timeline = m_bindings[64 * fbo_id + att_point];
    timeline.unbind_last(call_no);
}

}
