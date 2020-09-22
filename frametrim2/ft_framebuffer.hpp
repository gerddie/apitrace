#ifndef CFRAMEBUFFER_HPP
#define CFRAMEBUFFER_HPP

#include "trace_parser.hpp"

#include "ft_genobjectstate.hpp"

#include <memory>

namespace frametrim {

class FramebufferState : public GenObjectState {
public:
    using Pointer = std::shared_ptr<FramebufferState>;

    FramebufferState(GLint glID, PTraceCall gen_call);

    PTraceCall viewport(const trace::Call& call);

    PTraceCall clear(const trace::Call& call);

    PTraceCall draw(const trace::Call& call);

    void append_state_cache(PCallSet cache);

    virtual void attach(unsigned index, PSizedObjectState attachment,
                        unsigned layer, PTraceCall call) = 0;

protected:
    void set_size(unsigned width, unsigned height);

private:

    virtual void set_viewport_size(unsigned width, unsigned height);
    void do_emit_calls_to_list(CallSet& list) const override;

    unsigned m_width;
    unsigned m_height;

    unsigned m_viewport_x;
    unsigned m_viewport_y;
    unsigned m_viewport_width;
    unsigned m_viewport_height;

    PTraceCall m_viewport_call;
    CallSet m_draw_calls;

    std::vector<PCallSet> m_dependend_states;
};

using PFramebufferState = FramebufferState::Pointer;

class FBOState : public FramebufferState {
public:
    using FramebufferState::FramebufferState;

    virtual void attach(unsigned index, PSizedObjectState attachment,
                        unsigned layer, PTraceCall call) override;


private:
    void set_viewport_size(unsigned width, unsigned height) override;

    std::unordered_map<unsigned, SizedObjectState::Pointer> m_attachments;
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

    FramebufferState& current_framebuffer() {
        assert(m_current_framebuffer);
        return *m_current_framebuffer;
    }

private:

    void post_bind(unsigned target, FramebufferState::Pointer fbo) override;

    FramebufferState::Pointer
    bound_to_call_target(const trace::Call& call) const;

    FramebufferState::Pointer m_default_framebuffer;
    FramebufferState::Pointer m_current_framebuffer;
    FramebufferState::Pointer m_read_buffer;

};

}

#endif // CFRAMEBUFFER_HPP
