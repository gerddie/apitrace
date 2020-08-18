#include "ft_texturestate.hpp"
#include <cstring>

namespace frametrim {

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

   if (!strcmp(call->name(), "glTexImage2D") ||
       !strcmp(call->name(), "glTexImage3D")) {
      auto level = call->arg(1).toUInt();
      auto w = call->arg(3).toUInt();
      auto h = call->arg(4).toUInt();
      set_size(level, w, h);
   } else if (!strcmp(call->name(), "glTexImage1D")) {
      auto level = call->arg(1).toUInt();
      auto w = call->arg(3).toUInt();
      set_size(level, w, 1);
   }
}

void TextureState::use(PCall call)
{
   m_data_use_set.clear();
   if (m_last_unit_call)
      m_data_use_set.insert(m_last_unit_call);
   m_data_use_set.insert(m_last_bind_call);
   m_data_use_set.insert(call);
}

void TextureState::rendertarget_of(PFramebufferState fbo)
{
   m_fbo = fbo;
}

void TextureState::do_append_calls_to(CallSet& list) const
{
   if (!m_data_use_set.empty() || m_last_bind_call) {
      emit_gen_call(list);
      list.insert(m_data_upload_set.begin(), m_data_upload_set.end());
      list.insert(m_data_use_set.begin(), m_data_use_set.end());

      /* This is a somewhat lazy approach, but we assume that if the texture
       * was attached to a draw FBO then this was done to create the contents
       * of the texture, so we need to record all calls used in the fbo */
      if (m_fbo)
         m_fbo->append_calls_to(list);
   }
}


}