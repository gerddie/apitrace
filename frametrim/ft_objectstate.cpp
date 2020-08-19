#include "ft_objectstate.hpp"

namespace frametrim {

ObjectState::ObjectState(GLint glID):
   m_glID(glID),
   m_goid(g_next_object_id++),
   m_submitted(false)
{
}

unsigned ObjectState::id() const
{
   return m_glID;
}

void ObjectState::emit_calls_to_list(CallSet& list) const
{
   if (!m_submitted) {
      m_submitted = true;
      do_emit_calls_to_list(list);
      list.insert(m_gen_calls.begin(), m_gen_calls.end());
      list.insert(m_calls.begin(), m_calls.end());
      m_submitted = false;
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

CallSet& ObjectState::calls()
{
   return m_calls;
}

void ObjectState::do_emit_calls_to_list(CallSet &list) const
{
   (void)list;
}

uint64_t ObjectState::g_next_object_id = 0;
}