#include "ftr_attachableobject.hpp"

namespace frametrim_reverse {

void AttachableObject::allocate(const trace::Call& call)
{
    unsigned level = evaluate_size(call);

    if (m_allocation_call.size() <= level)
        m_allocation_call.resize(level + 1);

    m_allocation_call[level] = call.no;
}

void AttachableObject::collect_allocation_call(CallIdSet& calls)
{
    for (auto&& c: m_allocation_call) {
        calls.insert(c);
        collect_last_call_before(calls, m_bind_calls, c);
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

void AttachableObject::attach_to(PGenObject obj)
{
    assert(m_attached_to.find(obj->id()) == m_attached_to.end() &&
           "Attacheing to the same FBO twice is not handled");
    m_attached_to[obj->id()] = obj;
}

void AttachableObject::detach_from(unsigned fbo_id)
{
    m_attached_to.erase(fbo_id);
}

}
