#ifndef TEXTURESTATE_HPP
#define TEXTURESTATE_HPP

#include "ft_framebuffer.hpp"
#include "ft_bufferstate.hpp"

namespace frametrim {

class TextureState : public SizedObjectState
{
public:
    using Pointer = std::shared_ptr<TextureState>;

    TextureState(GLint glID, PTraceCall gen_call);

    void bind_unit(PTraceCall unit);
    void post_bind(unsigned target, const PTraceCall& unit) override;
    void post_unbind(const PTraceCall& unit) override;

    PTraceCall data(const trace::Call& call);
    PTraceCall storage(const trace::Call& call);
    PTraceCall sub_data(const trace::Call& call);
    PTraceCall copy_sub_data(const trace::Call& call,
                             PFramebufferState fbo);
    void rendertarget_of(unsigned layer,
                         FramebufferState::Pointer fbo);

private:
    unsigned get_level_with_face_index(const trace::Call& call);

    bool is_active() const override;

    ObjectType type() const override {return bt_texture;}

    void do_emit_calls_to_list(CallSet& list) const override;
    void pass_state_cache(unsigned object_id, PCallSet cache) override;
    void emit_dependend_caches(CallSet& list) const override;


    bool  m_last_unit_call_dirty;
    PTraceCall m_last_unit_call;
    bool  m_last_bind_call_dirty;
    std::unordered_map<unsigned, CallSet> m_data_upload_set;
    CallSet m_data_use_set;
    int m_attach_count;
    std::unordered_map<unsigned, FramebufferState::Pointer> m_fbo;
    std::unordered_map<unsigned, BufferState> m_data_buffers;
    std::unordered_map<unsigned, PCallSet> m_creator_states;

};

using PTextureState = TextureState::Pointer;

class TextureStateMap : public TGenObjStateMap<TextureState>
{
public:
    TextureStateMap();

    PTraceCall active_texture(const trace::Call& call);

    PTraceCall set_data(const trace::Call& call);

    PTraceCall storage(const trace::Call& call);

    PTraceCall set_sub_data(const trace::Call& call);
    PTraceCall copy_sub_data(const trace::Call& call,
                             FramebufferState::Pointer read_fb);

    PTraceCall gen_mipmap(const trace::Call& call);

    PTraceCall bind_multitex(const trace::Call& call);

    PTraceCall set_state(const trace::Call& call, unsigned nparam_sel);

private:

    void post_bind(unsigned target, PTextureState obj) override;
    void post_unbind(unsigned target, PTraceCall call) override;
    unsigned composed_target_id(unsigned id) const override;

    unsigned compose_target_id_with_unit(unsigned target, unsigned unit) const;

    unsigned m_active_texture_unit;
    PTraceCall m_active_texture_unit_call;
};

}

#endif // TEXTURESTATE_HPP
