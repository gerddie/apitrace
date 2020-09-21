#include "ft_objectwithbindstate.hpp"

namespace frametrim {

ObjectWithBindState::ObjectWithBindState(GLint glID,
                                         const trace::Call& call):
    ObjectState(glID, call),
    m_bound(false),
    m_bound_dirty(false)
{
}

ObjectWithBindState::ObjectWithBindState(GLint glID):
    ObjectState(glID),
    m_bound(false),
    m_bound_dirty(false)
{
}

void ObjectWithBindState::bind(const trace::Call& call)
{
    m_bind_call = trace2call(call);
    m_bound = true;
    m_bound_dirty = true;

    post_bind(call);
}

void ObjectWithBindState::unbind(const trace::Call& call)
{
    m_bind_call = trace2call(call);
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

void ObjectWithBindState::post_bind(const trace::Call& call)
{
    (void)call;
    // pseudoabstract method
}

void ObjectWithBindState::post_unbind(const trace::Call& call)
{
    (void)call;
    // pseudoabstract method
}

}