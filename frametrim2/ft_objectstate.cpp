#include "ft_objectstate.hpp"
#include "ft_framebuffer.hpp"
#include <sstream>

namespace frametrim {

using std::stringstream;

ObjectState::ObjectState(GLint glID, PTraceCall call):
    m_glID(glID),
    m_emitting(false),
    m_callset_dirty(true)
{
    m_gen_calls.insert(call);
}

ObjectState::ObjectState(GLint glID):
    m_glID(glID),
    m_emitting(false),
    m_callset_dirty(true)
{
}

ObjectState::~ObjectState()
{
    // only needed to be virtual
}

unsigned ObjectState::id() const
{
    return m_glID;
}

unsigned ObjectState::global_id() const
{
    return type() + bt_last * id();
}

void ObjectState::emit_calls_to_list(CallSet& list) const
{
    /* Because of circular references (e.g. FBO and texture attachments that
    * gets used later) we need to make sure we only emit this series once,
    * otherwise we might end up with a stack overflow.
    */
    if (!m_emitting && is_active()) {
        m_emitting = true;

        list.insert(m_gen_calls);
        list.insert(m_state_calls);
        list.insert(m_calls);

        do_emit_calls_to_list(list);
        emit_dependend_caches(list);

        m_emitting = false;
    }
}

PTraceCall
ObjectState::append_call(PTraceCall call)
{
    m_callset_dirty = true;
    m_calls.insert(call);
    dirty_cache();
    return call;
}

void ObjectState::reset_callset()
{
    m_calls.clear();
}

void ObjectState::flush_state_cache(ObjectState& fbo) const
{
    if (m_callset_dirty) {
        m_state_cache = std::make_shared<CallSet>();
        emit_calls_to_list(*m_state_cache);
        m_callset_dirty = false;
    }
    if (!m_state_cache->empty())
        fbo.pass_state_cache(global_id(), m_state_cache);
}

void ObjectState::pass_state_cache(unsigned object_id, PCallSet cache)
{
    (void)object_id;
    (void)cache;
    assert(0 && "object type doesn't support dependend states");
}

PTraceCall
ObjectState::set_state_call(const trace::Call& call, unsigned nstate_id_params)
{
    m_callset_dirty = true;
    auto c = std::make_shared<TraceCall>(call, nstate_id_params);
    m_state_calls[c->name()] = c;
    post_set_state_call(c);
    dirty_cache();
    return c;
}

bool ObjectState::is_active() const
{
    return true;
}

void ObjectState::do_emit_calls_to_list(CallSet &list) const
{
    (void)list;
}

void ObjectState::emit_dependend_caches(CallSet& list) const
{
    (void)list;
}

}