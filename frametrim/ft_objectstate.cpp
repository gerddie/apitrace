#include "ft_objectstate.hpp"
#include "ft_framebuffer.hpp"
#include <sstream>

namespace frametrim {

using std::stringstream;

ObjectState::ObjectState(GLint glID, PTraceCall call):
    m_glID(glID),
    m_emitting(false),
    m_callset_submit_fence(0),
    m_callset_fence(1)
{
    m_gen_calls.insert(call);
}

ObjectState::ObjectState(GLint glID):
    m_glID(glID),
    m_emitting(false),
    m_callset_submit_fence(0),
    m_callset_fence(1)
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

const char *
ObjectState::type_name() const
{
    const char *typenames[bt_last] = {
        "Buffer",
        "Displaylist",
        "Framebuffer",
        "Program",
        "Legacyprogram",
        "Matrix",
        "Renderbuffer",
        "Shader",
        "Sampler",
        "SyncObject",
        "Texture",
        "VertexArray",
        "VertexPointer"
    };

    return type() < bt_last ? typenames[type()] : "Unknown";
}

unsigned ObjectState::global_id() const
{
    return type() + bt_last * id() + sc_last;
}

void ObjectState::dirty_cache()
{
    if (m_callset_fence <= m_callset_submit_fence)
        m_callset_fence = m_callset_submit_fence + 1;
}

bool ObjectState::state_cache_dirty() const
{
    return m_callset_fence > m_callset_submit_fence;
}

unsigned ObjectState::cache_id() const
{
    return (global_id() << 16) + get_cache_id_helper();
}

unsigned ObjectState::get_cache_id_helper() const
{
    return 0;
}

unsigned ObjectState::fence_id() const
{
    return m_callset_submit_fence;
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
    m_calls.insert(call);
    dirty_cache();
    return call;
}

PTraceCall
ObjectState::append_call(const trace::Call& call)
{
    return append_call(trace2call(call));
}

void ObjectState::reset_callset()
{
    m_calls.clear();
    dirty_cache();
}

void ObjectState::flush_state_cache(ObjectState& fbo) const
{
    auto state_cache = get_state_cache();
    if (!state_cache->empty())
        fbo.pass_state_cache(cache_id(), state_cache);
}

PCallSet ObjectState::get_state_cache() const
{
    if (m_callset_fence > m_callset_submit_fence) {
        m_state_cache = std::make_shared<CallSet>();
        emit_calls_to_list(*m_state_cache);
        m_callset_submit_fence = m_callset_fence;
    }
    return m_state_cache;
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