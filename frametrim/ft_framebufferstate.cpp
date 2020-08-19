#include "ft_framebufferstate.hpp"
#include <GL/glext.h>
#include <cstring>
#include <iostream>

namespace frametrim {

FramebufferState::FramebufferState(GLint glID, PCall gen_call):
   GenObjectState(glID, gen_call),
   m_width(0),
   m_height(0),
   m_viewport_full_size(false),
   m_attached_buffer_types(0),
   m_attached_color_buffer_mask(0)
{
}


void FramebufferState::bind(PCall call)
{
   m_bind_call = call;
}

void FramebufferState::attach(unsigned attachment, PCall call,
                              PSizedObjectState att)
{
   if (m_attachments[attachment] &&
       (*m_attachments[attachment] == *att))
      return;

   m_attachments[attachment] = att;
   m_attachment_call[attachment] = call;
   m_attach_calls.insert(m_bind_call);
   m_attach_calls.insert(call);

   m_width = att->width();
   m_height = att->height();

   m_attached_buffer_types = 0;
   m_attached_color_buffer_mask = 0;

   for(auto& a: m_attachments) {
      if (a.second) {
         switch (a.first) {
         case GL_DEPTH_ATTACHMENT:
            m_attached_buffer_types |= GL_DEPTH_BUFFER_BIT;
            break;
         case GL_STENCIL_ATTACHMENT:
            m_attached_buffer_types |= GL_STENCIL_BUFFER_BIT;
            break;
         default:
            m_attached_buffer_types |= GL_COLOR_BUFFER_BIT;
            m_attached_color_buffer_mask |= 1 << (a.first - GL_COLOR_ATTACHMENT0);
         }
      }
   }
}

void FramebufferState::draw(PCall call)
{
   m_draw.insert(call);
}

CallSet& FramebufferState::state_calls()
{
   return m_draw_prepare;
}

void FramebufferState::set_viewport(PCall call)
{
   m_viewport_call = call;
   m_viewport_full_size =
         call->arg(0).toUInt() == 0 &&
         call->arg(1).toUInt() == 0 &&
         call->arg(2).toUInt() == m_width &&
         call->arg(3).toUInt() == m_height;
}

void FramebufferState::clear(PCall call)
{
   /* If the viewport covers the full size, and we clear all buffer tyoes
    * so we can forget all older draw calls
    * FIXME: Technically it is possible that some buffer is masked out and
    * its contents should be retained from earlier draw calls. */
   unsigned flags = m_attached_buffer_types & ~call->arg(0).toUInt();

   if (m_viewport_full_size && !flags)
      m_draw.clear();

	m_draw.insert(m_bind_call);

   if(m_viewport_call)
      m_draw.insert(m_viewport_call);
   m_draw.insert(call);
}


void FramebufferState::do_emit_calls_to_list(CallSet& list) const
{
   if (m_bind_call)  {
      emit_gen_call(list);

      list.insert(m_bind_call);
      list.insert(m_attach_calls.begin(), m_attach_calls.end());
      list.insert(m_draw_prepare.begin(), m_draw_prepare.end());
      list.insert(m_draw.begin(), m_draw.end());

      for(auto& a: m_attachments)
         if (a.second)
            a.second->emit_calls_to_list(list);
   }
}

RenderbufferState::RenderbufferState(GLint glID, PCall gen_call):
   SizedObjectState(glID, gen_call, renderbuffer)
{
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

void RenderbufferState::do_emit_calls_to_list(CallSet& list) const
{
   emit_gen_call(list);

   if (m_set_storage_call)
      list.insert(m_set_storage_call);

   if (m_attach_read_fb_call)
      list.insert(m_attach_read_fb_call);

   if (m_attach_write_fb_call)
      list.insert(m_attach_write_fb_call);

   if (m_is_blit_source) {
      assert(m_data_source);
      m_data_source->emit_calls_to_list(list);
   }
}


}
