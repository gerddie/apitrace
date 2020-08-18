#ifndef FRAMEBUFFERSTATE_HPP
#define FRAMEBUFFERSTATE_HPP

#include "ft_genobjectstate.hpp"
#include <unordered_map>

namespace frametrim {

class FramebufferState : public GenObjectState
{
public:
   using GenObjectState::GenObjectState;

   void bind(PCall call);

   void attach(unsigned attachemnt, PCall call, PGenObjectState att);



   void draw(PCall call);

private:

   void do_append_calls_to(CallSet& list) const override;

   PCall m_bind_call;

   CallSet m_draw_prepare;
   CallSet m_draw;

   unsigned m_sizex, m_sizey;

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

   void do_append_calls_to(CallSet& list) const override;

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
