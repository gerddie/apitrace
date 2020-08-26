#include "ft_texturestate.hpp"
#include "ft_globalstate.hpp"
#include <cstring>
#include <sstream>

#include <GL/glext.h>

namespace frametrim {

using std::stringstream;

TextureState::TextureState(GLint glID, PCall gen_call):
    SizedObjectState(glID, gen_call, texture),
    m_last_bind_call_dirty(true)
{

}

void TextureState::bind_unit(PCall b, PCall unit)
{
    if (m_last_unit_call && unit)
        m_last_unit_call_dirty = true;

    m_last_bind_call = b;
    m_last_unit_call = unit;
    m_last_bind_call_dirty = true;


}

void TextureState::data(PCall call)
{
    unsigned level = call->arg(1).toUInt();
    assert(level < 16);

    m_data_upload_set[level].clear();
    if (m_last_unit_call) {
        m_data_upload_set[level].insert(m_last_unit_call);
        m_last_unit_call_dirty = false;
    }

    m_data_upload_set[level].insert(m_last_bind_call);
    m_last_bind_call_dirty = false;

    m_data_upload_set[level].insert(call);

    if (!strcmp(call->name(), "glTexImage2D") ||
            !strcmp(call->name(), "glTexImage3D")) {
        auto w = call->arg(3).toUInt();
        auto h = call->arg(4).toUInt();
        set_size(level, w, h);
    } else if (!strcmp(call->name(), "glTexImage1D")) {
        auto w = call->arg(3).toUInt();
        set_size(level, w, 1);
    }
}

void TextureState::sub_data(PCall call)
{
    unsigned level = call->arg(1).toUInt();
    assert(level < 16);
    /* We should check like with buffers and remove old
     * sub data calls */
    m_data_upload_set[level].clear();
    if (m_last_unit_call && m_last_unit_call_dirty) {
        m_data_upload_set[level].insert(m_last_unit_call);
        m_last_unit_call_dirty = false;
    }

    if (m_last_bind_call_dirty ) {
        m_data_upload_set[level].insert(m_last_bind_call);
        m_last_bind_call_dirty = false;
    }
    m_data_upload_set[level].insert(call);
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
        for(unsigned i = 0; i < 16; ++i)
            list.insert(m_data_upload_set[i]);
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
        if (m_bound_texture[target_unit]) {
            m_unbind_calls[target_unit].clear();
            if (m_active_texture_unit_call)
                m_unbind_calls[target_unit].insert(m_active_texture_unit_call);
            m_unbind_calls[target_unit].insert(call);
        }
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

void TextureStateMap::set_sub_data(PCall call)
{
    /* This will need some better handling */
    auto texture = m_bound_texture[get_target_unit(call)];

    if (!texture) {
        std::cerr << "No texture found in call " << call->no
                  << " target:" << call->arg(0).toUInt()
                  << " U:" << m_active_texture_unit;
        assert(0);
    }
    texture->sub_data(call);
}

void TextureStateMap::gen_mipmap(PCall call)
{
    auto texture = m_bound_texture[get_target_unit(call)];

    if (!texture) {
        std::cerr << "No texture found in call " << call->no
                  << " target:" << call->arg(0).toUInt()
                  << " U:" << m_active_texture_unit;
        assert(0);
    }
    texture->append_call(call);
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
    for (auto& tex: m_bound_texture) {
        tex.second->emit_calls_to_list(list);
    }

    for (auto& clear_unit : m_unbind_calls) {
        list.insert(clear_unit.second);
    }
}


}

