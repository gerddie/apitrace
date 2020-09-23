#ifndef RENDERBUFFER_HPP
#define RENDERBUFFER_HPP

#include "ft_framebuffer.hpp"
#include "ft_genobjectstate.hpp"

namespace frametrim {

class RenderBuffer : public SizedObjectState
{
public:
    using Pointer = std::shared_ptr<RenderBuffer>;

    RenderBuffer(GLint glID, PTraceCall gen_call);

    void attach_as_rendertarget(PFramebufferState write_fb);

    void detach();

    void set_used_as_blit_source();

    PTraceCall set_storage(const trace::Call& call);

private:

    ObjectType type() const override {return bt_renderbuffer;}

    bool is_active() const override;

    void do_emit_calls_to_list(CallSet& list) const override;

    PTraceCall m_set_storage_call;

    /* If the renderbuffer is used in a draw framebuffer and later in
    * a read framebuffer for doing a blit, we have to keep the creation
    * of the data here */
    bool m_is_blit_source;
    PFramebufferState m_data_source;

    int m_attach_count;
};

class RenderbufferMap : public TGenObjStateMap<RenderBuffer>
{
public:
    using TGenObjStateMap<RenderBuffer>::TGenObjStateMap;

    void post_bind(unsigned target, RenderBuffer::Pointer obj) override;
    void post_unbind(unsigned target, RenderBuffer::Pointer obj) override;

    PTraceCall storage(const trace::Call& call);

private:

    RenderBuffer::Pointer m_active_renderbuffer;
};


}

#endif // RENDERBUFFER_HPP
