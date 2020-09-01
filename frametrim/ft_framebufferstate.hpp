#ifndef FRAMEBUFFERSTATE_HPP
#define FRAMEBUFFERSTATE_HPP

#include "ft_genobjectstate.hpp"
#include <unordered_map>
#include <unordered_set>

namespace frametrim {

class FramebufferStateBase : public GenObjectState
{
public:

    using Pointer = std::shared_ptr<FramebufferStateBase>;

    FramebufferStateBase(GLint glID, PCall gen_call);

    void draw(PCall call);

    void set_viewport(PCall call);

    void set_size(unsigned width, unsigned heigh);

    void clear(PCall call);

    CallSet& state_calls();

    void depends(PGenObjectState read_buffer);
protected:

    void set_attachment_types(unsigned buffer_types);

private:
    bool is_active() const override;

    virtual bool has_active_buffers() const;

    void do_emit_calls_to_list(CallSet& list) const override;
    virtual void emit_buffer_calls_to_list(CallSet& list) const;

    virtual void do_clear();

    PCall m_viewport_call;

    CallSet m_draw_prepare;

    unsigned m_width, m_height;
    bool m_viewport_full_size;

    unsigned m_attached_buffer_types;

    std::unordered_set<PGenObjectState> m_read_dependencies;

};

class FBOState : public FramebufferStateBase
{
public:
    using Pointer = std::shared_ptr<FBOState>;

    FBOState(GLint glID, PCall gen_call);

    void bind_read(PCall call);
    void bind_draw(PCall call);

    bool attach(unsigned attachment, PCall call, PSizedObjectState att);
private:

    void do_clear() override;
    bool has_active_buffers() const override;
    void emit_buffer_calls_to_list(CallSet& list) const override;

    unsigned m_attached_color_buffer_mask;

    PCall m_bind_read_call;
    PCall m_bind_draw_call;

    std::unordered_map<unsigned, CallSet> m_attach_calls;
    std::unordered_map<unsigned, PSizedObjectState> m_attachments;
};

using PFBOState = FBOState::Pointer;

using PFramebufferState = FramebufferStateBase::Pointer;

class RenderbufferMap;
class TextureStateMap;

class FramebufferMap : public TGenObjStateMap<FBOState>
{
public:
    using TGenObjStateMap<FBOState>::TGenObjStateMap;

    void bind(PCall call);
    void blit(PCall call);
    void renderbuffer(PCall call, RenderbufferMap& renderbuffers);
    void texture(PCall call, TextureStateMap& textures);

    PFramebufferState draw_fb();
    PFramebufferState read_fb();

private:
    void do_emit_calls_to_list(CallSet& list) const override;

    PFBOState m_draw_framebuffer;
    PFBOState m_read_framebuffer;

    PCall m_last_unbind_draw_fbo;
    PCall m_last_unbind_read_fbo;
};




class RenderbufferState : public SizedObjectState
{
public:

    using Pointer = std::shared_ptr<RenderbufferState>;

    RenderbufferState(GLint glID, PCall gen_call);

    void attach_as_rendertarget(PFramebufferState write_fb);
    void detach();

    void set_used_as_blit_source();

    void set_storage(PCall call);

private:

    bool is_active() const override;

    void do_emit_calls_to_list(CallSet& list) const override;

    PCall m_set_storage_call;

    /* If the renderbuffer is used in a draw framebuffer and later in
    * a read framebuffer for doing a blit, we have to keep the creation
    * of the data here */
    bool m_is_blit_source;
    PFramebufferState m_data_source;

    int m_attach_count;
};

using PRenderbufferState = RenderbufferState::Pointer;


class RenderbufferMap : public TGenObjStateMap<RenderbufferState>
{
public:
    using TGenObjStateMap<RenderbufferState>::TGenObjStateMap;

    void post_bind(PCall call, PRenderbufferState obj) override;
    void post_unbind(PCall call, PRenderbufferState obj) override;

    void storage(PCall call);

private:

    PRenderbufferState m_active_renderbuffer;
};

}

#endif // FRAMEBUFFERSTATE_HPP
