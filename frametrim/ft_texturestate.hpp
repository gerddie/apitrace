#ifndef TEXTURESTATE_HPP
#define TEXTURESTATE_HPP

#include "ft_framebufferstate.hpp"

namespace frametrim {

class TextureState : public SizedObjectState
{
public:
    using Pointer = std::shared_ptr<TextureState>;

    TextureState(GLint glID, PCall gen_call);

    void bind_unit(PCall unit);
    void post_bind(PCall unit) override;
    void post_unbind(PCall unit) override;

    void data(PCall call);
    void sub_data(PCall call);
    void copy_sub_data(PCall call, PFramebufferState fbo);

    void rendertarget_of(unsigned layer, PFramebufferState fbo);

private:
    bool is_active() const override;

    void do_emit_calls_to_list(CallSet& list) const override;

    bool  m_last_unit_call_dirty;
    PCall m_last_unit_call;
    bool  m_last_bind_call_dirty;
    CallSet m_data_upload_set[16];
    CallSet m_data_use_set;
    int m_attach_count;

    std::unordered_map<unsigned, PFramebufferState> m_fbo;
};

using PTextureState = TextureState::Pointer;

class TextureStateMap : public TGenObjStateMap<TextureState>
{
public:
    TextureStateMap(GlobalState *gs);

    void active_texture(PCall call);

    void set_data(PCall call);

    void set_sub_data(PCall call);
    void copy_sub_data(PCall call);

    void gen_mipmap(PCall call);

    void bind_multitex(PCall call);

private:

    void post_bind(PCall call, PTextureState obj) override;
    void post_unbind(PCall call, PTextureState obj) override;
    unsigned composed_target_id(unsigned id) const override;

    unsigned compose_target_id_with_unit(unsigned target, unsigned unit) const;

    unsigned m_active_texture_unit;
    PCall m_active_texture_unit_call;
};

}

#endif // TEXTURESTATE_HPP
