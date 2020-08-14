#include "ft_objectstate.h"

namespace frametrim {

ObjectState::ObjectState(GLint glID):
   m_glID(glID),
   m_goid(g_next_object_id++)
{
}

unsigned ObjectState::id() const
{
   return m_glID;
}

void ObjectState::append_calls_to(CallSet& list) const
{
   do_append_calls_to(list);
   list.insert(m_calls.begin(), m_calls.end());
}

void ObjectState::append_call(PCall call)
{
   m_calls.push_back(call);
}

void ObjectState::set_required()
{
   m_required = true;
}

void ObjectState::set_active(bool active)
{
   m_active = active;
}

bool ObjectState::required() const
{
   return m_required;
}

bool ObjectState::active() const
{
   return m_active;
}

void ObjectState::do_append_calls_to(CallSet &list) const
{
   (void)list;
}

uint64_t ObjectState::g_next_object_id = 0;
}