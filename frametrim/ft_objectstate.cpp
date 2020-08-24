#include "ft_objectstate.hpp"
#include <sstream>

namespace frametrim {

using std::stringstream;

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

void ObjectState::emit_calls_to_list(CallSet& list) const
{
    /* Because of circular references (e.g. FBO and texture attachments that
    * gets used later) we need to make sure we only emit this series once,
    * otherwise we might end up with a stack overflow.
    */
    if (!m_emitting) {
        m_emitting = true;

        list.insert(m_gen_calls);
        list.insert(m_state_calls);
        list.insert(m_calls);

        do_emit_calls_to_list(list);

        m_emitting = false;
    }
}

void ObjectState::append_gen_call(PCall call)
{
    m_gen_calls.insert(call);
}

void ObjectState::append_call(PCall call)
{
    m_calls.insert(call);
}

void ObjectState::reset_callset()
{
    m_calls.clear();
}

void ObjectState::set_state_call(PCall call, unsigned nstate_id_params)
{
    stringstream state_call_name;
    state_call_name << call->name();
    for(unsigned i = 0; i < nstate_id_params; ++i)
        state_call_name << "_" << call->arg(i).toUInt();

    m_state_calls[state_call_name.str()] = call;
}

void ObjectState::do_emit_calls_to_list(CallSet &list) const
{
    (void)list;
}

}