#ifndef CFRAMEBUFFER_HPP
#define CFRAMEBUFFER_HPP

#include "trace_parser.hpp"

#include "ft_genobjectstate.hpp"
#include "ft_programstate.hpp"
#include "ft_vertexattribpointer.hpp"

#include <memory>

namespace frametrim {

class RenderbufferMap;

class FramebufferState : public GenObjectState {
public:
    using Pointer = std::shared_ptr<FramebufferState>;

    FramebufferState(GLint glID, PTraceCall gen_call);

    PTraceCall viewport(const trace::Call& call);

    PTraceCall clear(const trace::Call& call);

    void draw(PTraceCall call);

    PTraceCall draw_buffer(const trace::Call& call);

    PTraceCall read_buffer(const trace::Call& call);

    virtual void attach(unsigned index, PSizedObjectState attachment,
                        unsigned layer, PTraceCall call) = 0;

    PTraceCall blit(const trace::Call& call);

    void set_dependend_state_cache(unsigned key, PCallSet sc);

    void set_readbuffer_bind_call(PTraceCall call);

protected:
    void set_size(unsigned width, unsigned height);

private:
    ObjectType type() const override {return bt_framebuffer;}

    virtual bool clear_all_buffers(unsigned mask) const = 0;

    virtual void set_viewport_size(unsigned width, unsigned height);
    void do_emit_calls_to_list(CallSet& list) const override;
    virtual void emit_attachment_calls_to_list(CallSet& list) const;

    void pass_state_cache(unsigned object_id, PCallSet cache) override;
    void emit_dependend_caches(CallSet& list) const override;
    virtual PTraceCall readbuffer_call(unsigned attach_id);

    void post_bind(unsigned target, const PTraceCall& call) override;

    unsigned m_width;
    unsigned m_height;

    unsigned m_viewport_x;
    unsigned m_viewport_y;
    unsigned m_viewport_width;
    unsigned m_viewport_height;

    std::vector<PTraceCall> m_viewport_calls;
    PTraceCall m_drawbuffer_call;
    PTraceCall m_readbuffer_call;
    PTraceCall m_bind_as_readbuffer_call;
    CallSet m_draw_calls;
    bool m_bind_dirty;
protected:
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
    bool clear_all_buffers(unsigned mask) const override;
    void post_unbind(const PTraceCall& call) override;
    PTraceCall readbuffer_call(unsigned attach_id) override;

    std::unordered_map<unsigned, SizedObjectState::Pointer> m_attachments;
    std::unordered_map<unsigned, PTraceCall> m_attach_call;
};

class DefaultFramebufferState : public FramebufferState {
public:
    DefaultFramebufferState();

    virtual void attach(unsigned index, PSizedObjectState attachment,
                        unsigned layer, PTraceCall call) override;

private:
    void pass_state_cache(unsigned object_id, PCallSet cache) override;
    bool clear_all_buffers(unsigned mask) const override;
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

    PTraceCall draw_buffer(const trace::Call& call);

    PTraceCall read_buffer(const trace::Call& call);

    PTraceCall attach_texture(const trace::Call& call, TextureStateMap& texture_map,
                              unsigned tex_param_idx,
                              unsigned level_param_idx);

    PTraceCall attach_renderbuffer(const trace::Call& call,
                                   RenderbufferMap& renderbuffer_map);

    PTraceCall blit(const trace::Call& call);

    PTraceCall bind_fbo(const trace::Call& call, bool recording);

    FramebufferState& current_framebuffer() {
        assert(m_current_framebuffer);
        return *m_current_framebuffer;
    }

    FramebufferState::Pointer read_framebuffer() {
        return m_read_buffer;
    }

private:

    void post_bind(unsigned target, FramebufferState::Pointer fbo) override;
    void post_unbind(unsigned target, PTraceCall call) override;

    void do_emit_calls_to_list(CallSet& list) const override;

    FramebufferState::Pointer
    bound_to_call_target(const trace::Call& call) const;

    FramebufferState::Pointer m_default_framebuffer;
    FramebufferState::Pointer m_current_framebuffer;
    FramebufferState::Pointer m_read_buffer;

};

}

#endif // CFRAMEBUFFER_HPP
