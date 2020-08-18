#include "ft_framebufferstate.hpp"

namespace frametrim {


void FramebufferState::bind(PCall call)
{
   m_bind_call = call;
}

void FramebufferState::attach(unsigned attachemnt, PCall call,
                              PGenObjectState att)
{
   m_attachments[attachemnt] = att;
   m_attachment_call[attachemnt] = call;
}

void FramebufferState::do_append_calls_to(CallSet& list) const
{
   if (m_bind_call)  {

      for(auto& a: m_attachments)
         if (a.second)
            a.second->append_calls_to(list);

   }
}

void RenderbufferState::attach(PCall call, bool read, PFramebufferState write_fb)
{
   if (read)
      m_attach_read_fb_call = call;

   if (write_fb) {
      m_attach_write_fb_call = call;
      m_data_source = write_fb;
   }
}

void RenderbufferState::set_used_as_blit_source()
{
   m_is_blit_source = true;
}

void RenderbufferState::set_storage(PCall call)
{
   m_set_storage_call = call;
   unsigned w = call->arg(2).toUInt();
   unsigned h = call->arg(2).toUInt();
   set_size(0, w, h);
}

void RenderbufferState::do_append_calls_to(CallSet& list) const
{
   if (m_set_storage_call)
      list.insert(m_set_storage_call);

   if (m_attach_read_fb_call)
      list.insert(m_attach_read_fb_call);

   if (m_attach_write_fb_call)
      list.insert(m_attach_write_fb_call);

   if (m_is_blit_source) {
      assert(m_data_source);
      m_data_source->append_calls_to(list);
   }
}


}
