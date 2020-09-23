#ifndef CFRAMEBUFFER_HPP
#define CFRAMEBUFFER_HPP

#include "trace_parser.hpp"

#include "ft_genobjectstate.hpp"

#include <memory>

namespace frametrim {

class RenderbufferMap;

class FramebufferState : public GenObjectState {
public:
    using Pointer = std::shared_ptr<FramebufferState>;

    FramebufferState(GLint glID, PTraceCall gen_call);

    PTraceCall viewport(const trace::Call& call);

    PTraceCall clear(const trace::Call& call);

    PTraceCall draw(const trace::Call& call);

    void append_state_cache(unsigned object_id, PCallSet cache);

    virtual void attach(unsigned index, PSizedObjectState attachment,
                        unsigned layer, PTraceCall call) = 0;

protected:
    void set_size(unsigned width, unsigned height);

private:
    ObjectType type() const override {return bt_framebuffer;}

    virtual void set_viewport_size(unsigned width, unsigned height);
    void do_emit_calls_to_list(CallSet& list) const override;
    virtual void emit_attachment_calls_to_list(CallSet& list) const;

    unsigned m_width;
    unsigned m_height;

    unsigned m_viewport_x;
    unsigned m_viewport_y;
    unsigned m_viewport_width;
    unsigned m_viewport_height;

    PTraceCall m_viewport_call;
    CallSet m_draw_calls;

    std::unordered_map<unsigned, PCallSet> m_dependend_states;
};

using PFramebufferState = FramebufferState::Pointer;

class FBOState : public FramebufferState {
public:
    using FramebufferState::FramebufferState;

    virtual void attach(unsigned index, PSizedObjectState attachment,
                        unsigned layer, PTraceCall call) override;


private:
    void set_viewport_size(unsigned width, unsigned height) override;
    void emit_attachment_calls_to_list(CallSet& list) const override;

    std::unordered_map<unsigned, SizedObjectState::Pointer> m_attachments;
    std::unordered_map<unsigned, std::pair<PTraceCall, PTraceCall>> m_attach_call;
};

class DefaultFramebufferState : public FramebufferState {
public:
    DefaultFramebufferState();

    virtual void attach(unsigned index, PSizedObjectState attachment,
                        unsigned layer, PTraceCall call) override;

private:
    void set_viewport_size(unsigned width, unsigned height) override;
    bool is_active() const override;

    bool m_initial_viewport_set;

};


class TextureStateMap;

class FramebufferStateMap : public TGenObjStateMap<FBOState> {
public:

    FramebufferStateMap();

    PTraceCall viewport(const trace::Call& call);

    PTraceCall clear(const trace::Call& call);

    PTraceCall draw(const trace::Call& call);

    PTraceCall attach_texture(const trace::Call& call, TextureStateMap& texture_map,
                              unsigned tex_param_idx,
                              unsigned level_param_idx);

    PTraceCall attach_renderbuffer(const trace::Call& call,
                                   RenderbufferMap& renderbuffer_map);

    FramebufferState& current_framebuffer() {
        assert(m_current_framebuffer);
        return *m_current_framebuffer;
    }

private:

    void post_bind(unsigned target, FramebufferState::Pointer fbo) override;
    void post_unbind(unsigned target, FramebufferState::Pointer fbo) override;

    FramebufferState::Pointer
    bound_to_call_target(const trace::Call& call) const;

    FramebufferState::Pointer m_default_framebuffer;
    FramebufferState::Pointer m_current_framebuffer;
    FramebufferState::Pointer m_read_buffer;

};

}

#endif // CFRAMEBUFFER_HPP
