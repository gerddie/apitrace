#ifndef TEXTURESTATE_HPP
#define TEXTURESTATE_HPP

#include "ft_framebufferstate.hpp"

namespace frametrim {

class TextureState : public SizedObjectState
{
public:
    using Pointer = std::shared_ptr<TextureState>;

    TextureState(GLint glID, PCall gen_call);

    void bind_unit(PCall bind, PCall unit);

    void data(PCall call);
    void sub_data(PCall call);
    void use(PCall call);

    void rendertarget_of(unsigned layer, PFramebufferState fbo);

private:
    virtual void do_emit_calls_to_list(CallSet& list) const;

    bool  m_last_unit_call_dirty;
    PCall m_last_unit_call;
    bool  m_last_bind_call_dirty;
    PCall m_last_bind_call;
    CallSet m_data_upload_set[16];
    CallSet m_data_use_set;

    std::unordered_map<unsigned, PFramebufferState> m_fbo;
};

using PTextureState = TextureState::Pointer;

class TextureStateMap : public TGenObjStateMap<TextureState>
{
public:
    TextureStateMap(GlobalState *gs);

    void active_texture(PCall call);

    void bind(PCall call);

    void set_data(PCall call);

    void set_sub_data(PCall call);

    void gen_mipmap(PCall call);

    void set_state(PCall call, unsigned addr_params);

private:
    unsigned get_target_unit(PCall call) const;

    void do_emit_calls_to_list(CallSet& list) const override;

    unsigned m_active_texture_unit;
    std::unordered_map<unsigned, PTextureState> m_bound_texture;
    PCall m_active_texture_unit_call;

    std::unordered_map<unsigned, CallSet> m_unbind_calls;

};

}

#endif // TEXTURESTATE_HPP
