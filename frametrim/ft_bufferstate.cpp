#include "ft_bufferstate.hpp"

namespace frametrim {

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

void BufferState::do_emit_calls_to_list(CallSet& list) const
{
   if (!m_data_use_set.empty()) {
      emit_gen_call(list);
      list.insert(m_data_upload_set);
      list.insert(m_data_use_set);
   }
}

}
