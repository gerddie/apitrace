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
    m_emitting(false)
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

        m_emitting = false;
    }
}

PTraceCall
ObjectState::append_call(PTraceCall call)
{
    m_calls.insert(call);
    return call;
}

void ObjectState::reset_callset()
{
    m_calls.clear();
}

void ObjectState::flush_state_cache(FramebufferState& fbo) const
{
    if (m_callset_dirty) {
        m_state_cache = std::make_shared<CallSet>();
        emit_calls_to_list(*m_state_cache);
    }
    fbo.append_state_cache(m_state_cache);
}

PTraceCall
ObjectState::set_state_call(const trace::Call& call, unsigned nstate_id_params)
{
    stringstream state_call_name;
    state_call_name << call.name();
    for(unsigned i = 0; i < nstate_id_params; ++i)
        state_call_name << "_" << call.arg(i).toUInt();

    m_state_calls[state_call_name.str()] = trace2call(call);
    return m_state_calls[state_call_name.str()];
}

bool ObjectState::is_active() const
{
    return true;
}

void ObjectState::do_emit_calls_to_list(CallSet &list) const
{
    (void)list;
}

}