#ifndef FRAMEBUFFERSTATE_HPP
#define FRAMEBUFFERSTATE_HPP

#include "ft_genobjectstate.hpp"
#include <unordered_map>

namespace frametrim {

class FramebufferState : public GenObjectState
{
public:
   FramebufferState(GLint glID, PCall gen_call);

   void bind(PCall call);

   void attach(unsigned attachemnt, PCall call, PSizedObjectState att);

   void draw(PCall call);

   void set_viewport(PCall call);

   void clear(PCall call);

   CallSet& state_calls();

private:

   void do_emit_calls_to_list(CallSet& list) const override;

   PCall m_bind_call;
   PCall m_viewport_call;

   CallSet m_attach_calls;
   CallSet m_draw_prepare;
   CallSet m_draw;

   unsigned m_width, m_height;
   bool m_viewport_full_size;

   unsigned m_attached_buffer_types;
   unsigned m_attached_color_buffer_mask;

   std::unordered_map<unsigned, PCall> m_attachment_call;
   std::unordered_map<unsigned, PGenObjectState> m_attachments;

};

using PFramebufferState = std::shared_ptr<FramebufferState>;

class RenderbufferState : public SizedObjectState
{
public:
   using SizedObjectState::SizedObjectState;

   void attach(PCall call, bool read, PFramebufferState write_fb);

   void set_used_as_blit_source();

   void set_storage(PCall call);

private:

   void do_emit_calls_to_list(CallSet& list) const override;

   PCall m_set_storage_call;

   PCall m_attach_read_fb_call;
   PCall m_attach_write_fb_call;

   /* If the renderbuffer is used in a draw framebuffer and later in
    * a read framebuffer for doing a blit, we have to keep the creation
    * of the data here */
   bool m_is_blit_source;
   PFramebufferState m_data_source;
};

using PRenderbufferState = std::shared_ptr<RenderbufferState>;


}

#endif // FRAMEBUFFERSTATE_HPP
