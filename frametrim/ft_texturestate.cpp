#include "ft_texturestate.hpp"
#include <cstring>
#include <sstream>

#include <GL/glext.h>

namespace frametrim {

using std::stringstream;

TextureState::TextureState(GLint glID, PCall gen_call):
    SizedObjectState(glID, gen_call, texture)
{

}

void TextureState::bind_unit(PCall b, PCall unit)
{
    m_last_bind_call = b;
    m_last_unit_call = unit;
}

void TextureState::data(PCall call)
{
    if (m_last_unit_call)
        m_data_upload_set.insert(m_last_unit_call);
    m_data_upload_set.insert(m_last_bind_call);
    m_data_upload_set.insert(call);

    if (!strcmp(call->name(), "glTexImage2D") ||
            !strcmp(call->name(), "glTexImage3D")) {
        auto level = call->arg(1).toUInt();
        auto w = call->arg(3).toUInt();
        auto h = call->arg(4).toUInt();
        set_size(level, w, h);
    } else if (!strcmp(call->name(), "glTexImage1D")) {
        auto level = call->arg(1).toUInt();
        auto w = call->arg(3).toUInt();
        set_size(level, w, 1);
    }
}

void TextureState::use(PCall call)
{
    m_data_use_set.clear();
    if (m_last_unit_call)
        m_data_use_set.insert(m_last_unit_call);
    m_data_use_set.insert(m_last_bind_call);
    m_data_use_set.insert(call);
}

void TextureState::rendertarget_of(unsigned layer, PFramebufferState fbo)
{
    m_fbo[layer] = fbo;
}

void TextureState::do_emit_calls_to_list(CallSet& list) const
{
    if (!m_data_use_set.empty() || m_last_bind_call) {
        emit_gen_call(list);
        list.insert(m_texture_state_set);
        list.insert(m_data_upload_set);
        list.insert(m_data_use_set);

        /* This is a somewhat lazy approach, but we assume that if the texture
       * was attached to a draw FBO then this was done to create the contents
       * of the texture, so we need to record all calls used in the fbo */
        for (auto& f : m_fbo)
            f.second->emit_calls_to_list(list);
    }
}

TextureStateMap::TextureStateMap(GlobalState *gs):
    TGenObjStateMap<TextureState>(gs),
    m_active_texture_unit(0)
{
}

void TextureStateMap::active_texture(PCall call)
{
    m_active_texture_unit = call->arg(0).toUInt() - GL_TEXTURE0;
    m_active_texture_unit_call = call;
}

void TextureStateMap::bind(PCall call)
{
    unsigned id = call->arg(1).toUInt();
    unsigned target_unit = get_target_unit(call);

    if (id) {
        if (!m_bound_texture[target_unit] ||
                m_bound_texture[target_unit]->id() != id) {

            auto tex = get_by_id(id);
            m_bound_texture[target_unit] = tex;
            tex->bind_unit(call, m_active_texture_unit_call);

            if (global_state().in_target_frame()) {
                tex->emit_calls_to_list(global_state().global_callset());
            }
            auto draw_fb = global_state().draw_framebuffer();
            if (draw_fb)
                tex->emit_calls_to_list(draw_fb->dependent_calls());
        }
    } else {
        if (m_bound_texture[target_unit])
            m_bound_texture[target_unit]->append_call(call);
        m_bound_texture.erase(target_unit);
    }
}

void TextureStateMap::set_data(PCall call)
{
    auto texture = m_bound_texture[get_target_unit(call)];

    if (!texture) {
        std::cerr << "No texture found in call " << call->no
                  << " target:" << call->arg(0).toUInt()
                  << " U:" << m_active_texture_unit;
        assert(0);
    }
    texture->data(call);
}

void TextureStateMap::set_state(PCall call, unsigned addr_params)
{
    auto texture = m_bound_texture[get_target_unit(call)];
    if (!texture) {
        std::cerr << "No texture found in call " << call->no
                  << " target:" << call->arg(0).toUInt()
                  << " U:" << m_active_texture_unit;
        assert(0);
    }
    texture->set_state_call(call, addr_params);
}


unsigned TextureStateMap::get_target_unit(PCall call) const
{
    auto target = call->arg(0).toUInt();
    switch (target) {
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        target = GL_TEXTURE_CUBE_MAP;
        break;
    default:
        ;
    }
    return (target << 8) | m_active_texture_unit;
}

void TextureStateMap::do_emit_calls_to_list(CallSet& list) const
{
    for (auto& tex: m_bound_texture)
        tex.second->emit_calls_to_list(list);
}


}