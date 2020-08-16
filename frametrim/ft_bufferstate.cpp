#include "ft_bufferstate.hpp"

namespace frametrim {

BufferState::BufferState(GLint glID, PCall gen_call):
   ObjectState(glID),
   m_gen_call(gen_call)
{
   assert(m_gen_call);
}

void BufferState::bind(PCall call)
{
   m_last_bind_call = call;
}

void BufferState::data(PCall call)
{
   m_data_upload_set.clear();
   m_data_upload_set.insert(m_last_bind_call);
   m_data_upload_set.insert(call);
}

void BufferState::use(PCall call)
{
   m_data_use_set.clear();
   m_data_use_set.insert(m_last_bind_call);
   m_data_use_set.insert(call);
}

void BufferState::do_append_calls_to(CallSet& list) const
{
   if (!m_data_use_set.empty()) {
      list.insert(m_gen_call);
      list.insert(m_data_upload_set.begin(), m_data_upload_set.end());
      list.insert(m_data_use_set.begin(), m_data_use_set.end());
   }
}


}
