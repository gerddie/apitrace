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

    post_bind(call);
}

void ObjectWithBindState::unbind(PCall call)
{
    m_bind_call = call;
    m_bound = false;
    post_unbind(call);
}

bool ObjectWithBindState::bound() const
{
    return m_bound;
}

void ObjectWithBindState::emit_bind(CallSet& out_list) const
{
    if (m_bind_call)
        out_list.insert(m_bind_call);
    else
        std::cerr << "Want to emit bind call for "
                  << id() << " but don't have one\n";
}

bool ObjectWithBindState::is_active() const
{
    return bound();
}

void ObjectWithBindState::post_bind(PCall call)
{
    (void)call;
    // pseudoabstract method
}

void ObjectWithBindState::post_unbind(PCall call)
{
    (void)call;
    // pseudoabstract method
}

}