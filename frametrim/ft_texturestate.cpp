#include "ft_texturestate.hpp"

namespace frametrim {

TextureState::TextureState(GLint glID, PCall gen_call):
   ObjectState(glID),
   m_gen_call(gen_call)
{
}

void TextureState::bind_unit(PCall b, PCall unit)
{
   m_last_bind_call = b;
   m_last_unit_call = unit;
}

void TextureState::data(PCall call)
{
   if (m_last_unit_call)
      m_data_upload_set.insert(m_last_unit_call);
   m_data_upload_set.insert(m_last_bind_call);
   m_data_upload_set.insert(call);
}

void TextureState::use(PCall call)
{
   m_data_use_set.clear();
   if (m_last_unit_call)
      m_data_use_set.insert(m_last_unit_call);
   m_data_use_set.insert(m_last_bind_call);
   m_data_use_set.insert(call);
}

void TextureState::do_append_calls_to(CallSet& list) const
{
   if (!m_data_use_set.empty() || m_last_bind_call) {
      list.insert(m_gen_call);
      list.insert(m_data_upload_set.begin(), m_data_upload_set.end());
      list.insert(m_data_use_set.begin(), m_data_use_set.end());
   }
}


}