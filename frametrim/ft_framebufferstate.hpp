#ifndef FRAMEBUFFERSTATE_HPP
#define FRAMEBUFFERSTATE_HPP

#include "ft_genobjectstate.hpp"
#include <unordered_map>
#include <unordered_set>

namespace frametrim {

class FramebufferState : public GenObjectState
{
public:

    using Pointer = std::shared_ptr<FramebufferState>;

    FramebufferState(GLint glID, PCall gen_call);

    void bind(PCall call);

    void attach(unsigned attachment, PCall call, PSizedObjectState att);

    void draw(PCall call);

    void set_viewport(PCall call);

    void clear(PCall call);

    CallSet& state_calls();

    void depends(PGenObjectState read_buffer);

private:

    void do_emit_calls_to_list(CallSet& list) const override;

    PCall m_bind_call;
    PCall m_viewport_call;

    std::unordered_map<unsigned, CallSet> m_attach_calls;
    CallSet m_draw_prepare;

    unsigned m_width, m_height;
    bool m_viewport_full_size;

    unsigned m_attached_buffer_types;
    unsigned m_attached_color_buffer_mask;

    std::unordered_map<unsigned, PSizedObjectState> m_attachments;

    std::unordered_set<PGenObjectState> m_read_dependencies;

};

using PFramebufferState = FramebufferState::Pointer;

class RenderbufferMap;
class TextureStateMap;

class FramebufferMap : public TStateMap<FramebufferState>
{
public:
    using TStateMap<FramebufferState>::TStateMap;

    void bind(PCall call);
    void blit(PCall call);
    void renderbuffer(PCall call, RenderbufferMap& renderbuffers);
    void texture(PCall call, TextureStateMap& textures);

    PFramebufferState draw_fb();
    PFramebufferState read_fb();

    void emit_calls_to_list(CallSet& list) const;

private:

    PFramebufferState m_draw_framebuffer;
    PFramebufferState m_read_framebuffer;

    PCall m_last_unbind_draw_fbo;
    PCall m_last_unbind_read_fbo;
};




class RenderbufferState : public SizedObjectState
{
public:

    using Pointer = std::shared_ptr<RenderbufferState>;

    RenderbufferState(GLint glID, PCall gen_call);

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

using PRenderbufferState = RenderbufferState::Pointer;


class RenderbufferMap : public TStateMap<RenderbufferState>
{
public:
    using TStateMap<RenderbufferState>::TStateMap;

    void bind(PCall call);

    void storage(PCall call);

    void emit_calls_to_list(CallSet& list) const;

private:
    PRenderbufferState m_active_renderbuffer;
    PCall m_last_unbind_call;

};

}

#endif // FRAMEBUFFERSTATE_HPP
