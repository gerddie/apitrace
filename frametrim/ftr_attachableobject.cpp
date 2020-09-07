#include "ftr_attachableobject.hpp"

namespace frametrim_reverse {

void AttachableObject::allocate(const trace::Call& call)
{
    evaluate_size(call);
    m_allocation_call = call.no;
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
