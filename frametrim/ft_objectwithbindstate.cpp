#include "ft_objectwithbindstate.hpp"

namespace frametrim {

ObjectWithBindState::ObjectWithBindState(GLint glID, PCall call):
    ObjectState(glID, call),
    m_bound(false)
{
}

void ObjectWithBindState::bind(PCall call)
{
    m_bind_call = call;
    m_bound = true;
    do_bind(call);
}

void ObjectWithBindState::unbind(PCall call)
{
    m_bind_call = call;
    m_bound = false;
    do_unbind(call);
}

bool ObjectWithBindState::bound() const
{
    return m_bound;
}

void ObjectWithBindState::emit_bind(CallSet& out_list)
{
    if (m_bind_call)
        out_list.insert(m_bind_call);
}

void ObjectWithBindState::do_bind(PCall call)
{
    (void)call;
    // pseudoabstract method
}

void ObjectWithBindState::do_unbind(PCall call)
{
    (void)call;
    // pseudoabstract method
}

}