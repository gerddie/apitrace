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

    void set_state(PCall call);
    void data(PCall call);
    void use(PCall call);

    void rendertarget_of(unsigned layer, PFramebufferState fbo);

private:
    virtual void do_emit_calls_to_list(CallSet& list) const;

    PCall m_last_unit_call;
    PCall m_last_bind_call;
    StateCallMap m_texture_state_set;
    CallSet m_data_upload_set;
    CallSet m_data_use_set;

    std::unordered_map<unsigned, PFramebufferState> m_fbo;
};

using PTextureState = TextureState::Pointer;

class TextureStateMap : public TStateMap<TextureState>
{
public:
    using TStateMap<TextureState>::TStateMap;

    void bind(PCall call);

private:
    unsigned m_active_texture_unit;
    std::unordered_map<unsigned, PTextureState> m_bound_textures;


};

}

#endif // TEXTURESTATE_HPP
