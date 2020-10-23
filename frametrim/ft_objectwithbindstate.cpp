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

void ObjectWithBindState::bind(unsigned target, PTraceCall call)
{
    m_bind_call = call;
    m_bound = true;
    m_bound_dirty = true;
    dirty_cache();
    post_bind(target, m_bind_call);

    auto gen = gen_call();
    if (call && gen)
        call->set_required_call(gen);
}

void ObjectWithBindState::unbind(PTraceCall call)
{
    dirty_cache();
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

void ObjectWithBindState::post_set_state_call(PTraceCall call)
{
    if (m_bind_call)
        call->set_required_call(m_bind_call);
}

void ObjectWithBindState::do_emit_calls_to_list(CallSet &list) const
{
    list.insert(m_bind_call);
}

bool ObjectWithBindState::is_active() const
{
    return bound();
}

void ObjectWithBindState::post_bind(unsigned target, const PTraceCall& call)
{
    (void)target;
    (void)call;
    // pseudoabstract method
}

void ObjectWithBindState::post_unbind(const PTraceCall& call)
{
    (void)call;
    // pseudoabstract method
}

}