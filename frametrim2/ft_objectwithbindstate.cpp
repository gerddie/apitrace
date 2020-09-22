#include "ft_objectwithbindstate.hpp"

namespace frametrim {

ObjectWithBindState::ObjectWithBindState(GLint glID,
                                         PTraceCall call):
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

void ObjectWithBindState::bind(PTraceCall call)
{
    m_bind_call = call;
    m_bound = true;
    m_bound_dirty = true;

    post_bind(m_bind_call);
}

void ObjectWithBindState::unbind(PTraceCall call)
{
    m_bind_call = call;
    m_bound = false;
    post_unbind(m_bind_call);
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

void ObjectWithBindState::post_bind(const PTraceCall& call)
{
    (void)call;
    // pseudoabstract method
}

void ObjectWithBindState::post_unbind(const PTraceCall& call)
{
    (void)call;
    // pseudoabstract method
}

}