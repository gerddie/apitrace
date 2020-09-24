#include "ft_texturestate.hpp"
#include <cstring>
#include <sstream>
#include <algorithm>

#include <GL/glext.h>

namespace frametrim {

using std::stringstream;

TextureState::TextureState(GLint glID, PTraceCall gen_call):
    SizedObjectState(glID, gen_call, texture),
    m_last_bind_call_dirty(true),
    m_attach_count(0)
{

}

void TextureState::bind_unit(PTraceCall unit)
{
    if (m_last_unit_call && unit)
        m_last_unit_call_dirty = true;

    if (unit)
        m_last_unit_call = unit;
}

void TextureState::post_bind(const PTraceCall& call)
{
    (void)call;
    m_last_bind_call_dirty = true;
}

void TextureState::post_unbind(const PTraceCall& call)
{
    (void)call;
    m_last_bind_call_dirty = true;
}

PTraceCall
TextureState::data(const trace::Call& call)
{
    unsigned level = call.arg(1).toUInt();
    assert(level < 16);

    dirty_cache();
    m_data_upload_set[level].clear();
    if (m_last_unit_call) {
        m_data_upload_set[level].insert(m_last_unit_call);
        m_last_unit_call_dirty = false;
    }

    if (m_last_bind_call_dirty) {
        emit_bind(m_data_upload_set[level]);
        m_last_bind_call_dirty = false;
    }

    auto c = trace2call(call);
    m_data_upload_set[level].insert(c);

    if (!strcmp(call.name(), "glTexImage2D") ||
            !strcmp(call.name(), "glTexImage3D")) {
        auto w = call.arg(3).toUInt();
        auto h = call.arg(4).toUInt();
        set_size(level, w, h);
    } else if (!strcmp(call.name(), "glTexImage1D")) {
        auto w = call.arg(3).toUInt();
        set_size(level, w, 1);
    }
    return c;
}

PTraceCall TextureState::storage(const trace::Call& call)
{
    unsigned levels = call.arg(1).toUInt();

    unsigned w0 = call.arg(3).toUInt();

    unsigned h0 = strcmp(call.name(), "glTexStorage1D") ?
                      call.arg(4).toUInt() : 1;

    auto c = trace2call(call);
    for (unsigned l = 0; l <= levels; ++l) {
        unsigned w = w0 >> l;
        unsigned h = h0 >> l;
        set_size(l, std::max(w, 1u), std::max(h, 1u));
        m_data_upload_set[l].insert(c);
    }
    return c;
}

PTraceCall
TextureState::sub_data(const trace::Call& call)
{
    unsigned level = call.arg(1).toUInt();
    assert(level < 16);
    /* We should check like with buffers and remove old
     * sub data calls */
    if (m_last_unit_call && m_last_unit_call_dirty) {
        m_data_upload_set[level].insert(m_last_unit_call);
        m_last_unit_call_dirty = false;
    }

    if (m_last_bind_call_dirty ) {
        emit_bind(m_data_upload_set[level]);
        m_last_bind_call_dirty = false;
    }
    auto c = trace2call(call);
    m_data_upload_set[level].insert(c);
    dirty_cache();
    return c;
}

PTraceCall
TextureState::copy_sub_data(const trace::Call& call,
                            FramebufferState::Pointer read_buffer)
{
    auto c = trace2call(call);
    auto level = call.arg(1).toUInt();
    m_data_upload_set[level].insert(c);
    m_fbo[level] = read_buffer;
    dirty_cache();
    return c;
}

void TextureState::rendertarget_of(unsigned layer,
                                   FramebufferState::Pointer fbo)
{
    m_fbo[layer] = fbo;
}

void TextureState::do_emit_calls_to_list(CallSet& list) const
{
    emit_gen_call(list);
    emit_bind(list);
    for(unsigned i = 0; i < 16; ++i)
        list.insert(m_data_upload_set[i]);
    list.insert(m_data_use_set);

    for(auto&& f: m_fbo) {
        if (f.second)
            f.second->emit_calls_to_list(list);
    }
}

bool TextureState::is_active() const
{
    return bound() || is_attached();
}

TextureStateMap::TextureStateMap():
    m_active_texture_unit(0)
{
}

PTraceCall TextureStateMap::active_texture(const trace::Call& call)
{
    m_active_texture_unit = call.arg(0).toUInt() - GL_TEXTURE0;
    m_active_texture_unit_call = trace2call(call);
    return m_active_texture_unit_call;
}

PTraceCall
TextureStateMap::bind_multitex(const trace::Call& call)
{
    auto unit = call.arg(0).toUInt() - GL_TEXTURE0;
    auto target =  call.arg(1).toUInt();
    auto id = call.arg(2).toUInt();

    auto target_id = compose_target_id_with_unit(target, unit);

    auto c = trace2call(call);
    bind_target(target_id, id, c);
    return c;
}

void
TextureStateMap::post_bind(unsigned target, PTextureState obj)
{
    (void)target;
    obj->bind_unit(m_active_texture_unit_call);
}

void TextureStateMap::post_unbind(unsigned target, PTraceCall call)
{
    (void)target;
    (void)call;
}

PTraceCall
TextureStateMap::set_data(const trace::Call& call)
{
    auto texture = bound_in_call(call);

    if (!texture) {
        std::cerr << "No texture bound in call " << call.no
                  << " target:" << call.arg(0).toUInt()
                  << " unit:" << m_active_texture_unit
                  << "\n";
        return trace2call(call);
    }
    return texture->data(call);
}

PTraceCall TextureStateMap::storage(const trace::Call& call)
{
    auto texture = bound_in_call(call);
    if (!texture) {
        std::cerr << "No texture bound in call " << call.no
                  << " target:" << call.arg(0).toUInt()
                  << " unit:" << m_active_texture_unit
                  << "\n";
        return trace2call(call);
    }
    return texture->storage(call);
}

PTraceCall
TextureStateMap::set_state(const trace::Call& call, unsigned nparam_sel)
{
    auto texture = bound_in_call(call);

    if (!texture) {
        std::cerr << "No texture bound in call " << call.no
                  << " target:" << call.arg(0).toUInt()
                  << " unit:" << m_active_texture_unit
                  << "\n";
        return trace2call(call);
    }
    return texture->set_state_call(call, nparam_sel);
}

PTraceCall
TextureStateMap::set_sub_data(const trace::Call& call)
{
    /* This will need some better handling */
    auto texture = bound_in_call(call);

    if (!texture) {
        std::cerr << __func__ << ": No texture bound in call " << call.no
                  << " target:" << call.arg(0).toUInt()
                  << " unit:" << m_active_texture_unit
                  << "\n";
        return trace2call(call);
    }
    return texture->sub_data(call);
}

PTraceCall
TextureStateMap::copy_sub_data(const trace::Call& call,
                                    FramebufferState::Pointer read_fb)
{
    auto texture = bound_in_call(call);
    if (!texture) {
        std::cerr << "No texture found in call " << call.no
                  << " target:" << call.arg(0).toUInt()
                  << " U:" << m_active_texture_unit;
        assert(0);
    }
    if (!read_fb) {
        std::cerr << "TODO: Handle if the read buffer "
                     "is the default framebuffer\n";
    }
    return texture->copy_sub_data(call, read_fb);

}

PTraceCall
TextureStateMap::gen_mipmap(const trace::Call& call)
{
    auto texture = bound_in_call(call);

    if (!texture) {
        std::cerr << "No texture found in call " << call.no
                  << " target:" << call.arg(0).toUInt()
                  << " U:" << m_active_texture_unit;
        assert(0);
    }
    return texture->append_call(trace2call(call));
}

unsigned TextureStateMap::composed_target_id(unsigned target) const
{
    return compose_target_id_with_unit(target, m_active_texture_unit);
}

unsigned TextureStateMap::compose_target_id_with_unit(unsigned target,
                                                      unsigned unit) const

{

    switch (target) {
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        target = GL_TEXTURE_CUBE_MAP;
        break;
    case GL_PROXY_TEXTURE_1D:
        target = GL_TEXTURE_1D;
        break;
    case GL_PROXY_TEXTURE_2D:
        target = GL_TEXTURE_2D;
        break;
    case GL_PROXY_TEXTURE_3D:
        target = GL_TEXTURE_3D;
        break;
    default:
        ;
    }
    return (target << 8) | unit;
}

}

