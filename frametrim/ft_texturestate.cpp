#include "ft_texturestate.hpp"
#include <cstring>
#include <sstream>
#include <algorithm>

#include <GL/glext.h>

namespace frametrim {

using std::stringstream;

enum TexTypes {
    gl_texture_buffer,
    gl_texture_1d,
    gl_texture_2d,
    gl_texture_3d,
    gl_texture_cube,
    gl_texture_1d_array,
    gl_texture_2d_array,
    gl_texture_cube_array,
    gl_texture_2dms,
    gl_texture_2dms_array,
    gl_texture_last
};

TextureState::TextureState(GLint glID, PTraceCall gen_call):
    SizedObjectState(glID, gen_call, texture),
    m_last_unit_call_dirty(true),
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

void TextureState::post_bind(unsigned target, const PTraceCall& call)
{
    (void)target;
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
    unsigned sized_level = call.arg(1).toUInt();
    unsigned level = get_level_with_face_index(call);

    m_data_upload_set[level].clear();
    if (m_last_unit_call) {
        m_data_upload_set[level].insert(m_last_unit_call);
        m_last_unit_call_dirty = false;
    }

    emit_bind(m_data_upload_set[level]);
    m_last_bind_call_dirty = false;

    auto c = trace2call(call);
    m_data_upload_set[level].insert(c);

    if (!strcmp(call.name(), "glTexImage2D") ||
        !strcmp(call.name(), "glCompressedTexImage2D") ||
            !strcmp(call.name(), "glTexImage3D")) {
        auto w = call.arg(3).toUInt();
        auto h = call.arg(4).toUInt();
        set_size(sized_level, w, h);
    } else if (!strcmp(call.name(), "glTexImage1D")) {
        auto w = call.arg(3).toUInt();
        set_size(sized_level, w, 1);
    }
    dirty_cache();
    return c;
}

PTraceCall TextureState::storage(const trace::Call& call)
{
    unsigned layers = call.arg(0).toUInt() == GL_TEXTURE_CUBE_MAP ? 6 : 1;
    unsigned levels = call.arg(1).toUInt();
    unsigned w0 = call.arg(3).toUInt();

    unsigned h0 = strcmp(call.name(), "glTexStorage1D") ?
                      call.arg(4).toUInt() : 1;

    auto c = trace2call(call);

    c->set_required_call(bind_call());

    for (unsigned l = 0; l <= levels; ++l) {
        unsigned w = w0 >> l;
        unsigned h = h0 >> l;
        set_size(l, std::max(w, 1u), std::max(h, 1u));
        for (unsigned i = 0; i < layers; ++i)
            m_data_upload_set[layers * l + i].insert(c);
    }
    dirty_cache();
    return c;
}

PTraceCall
TextureState::sub_data(const trace::Call& call)
{
    unsigned level = get_level_with_face_index(call);

    /* Not exhaustive */
    if (!strcmp(call.name(), "glTexImage2D"))
        assert(call.arg(4).toUInt() <= width());

    /* We should check like with buffers and remove old
     * sub data calls */
    if (m_last_unit_call && m_last_unit_call_dirty) {
        m_data_upload_set[level].insert(m_last_unit_call);
        m_last_unit_call_dirty = false;
    }

    if (m_last_bind_call_dirty) {
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

unsigned TextureState::get_level_with_face_index(const trace::Call& call)
{
    unsigned level = call.arg(1).toUInt();
    unsigned scale = 1;
    unsigned shift = 0;
    switch (call.arg(0).toUInt()) {
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        ++shift;
        /* fallthrough */
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        ++shift;
        /* fallthrough */
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        ++shift;
        /* fallthrough */
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        ++shift;
        /* fallthrough */
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        ++shift;
        /* fallthrough */
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        scale = 6;
    default:
        ;
    }
    level = scale * level + shift;
    return level;
}

void TextureState::rendertarget_of(unsigned layer,
                                   FramebufferState::Pointer fbo)
{
    m_fbo[layer] = fbo;
    attach();
    flush_state_cache(*fbo);
}

void TextureState::do_emit_calls_to_list(CallSet& list) const
{
    emit_gen_call(list);
    for(auto&& [layer, data]: m_data_upload_set) {
        list.insert(data);
    }
    list.insert(m_data_use_set);

    for(auto&& f: m_fbo) {
        if (f.second)
            f.second->emit_calls_to_list(list);
    }
    if (m_last_bind_call_dirty)
        list.insert(bind_call());
    if (m_last_unit_call_dirty)
        list.insert(m_last_unit_call);
}

bool TextureState::is_active() const
{
    return bound() || is_attached();
}

void TextureState::pass_state_cache(unsigned object_id, PCallSet cache)
{
    m_creator_states[object_id] = cache;
    dirty_cache();
}

void TextureState::emit_dependend_caches(CallSet& list) const
{
    for(auto&& cs: m_creator_states) {
        if (cs.second) {
            list.insert(cs.first, cs.second);
        }
    }
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
    if (obj)
        obj->bind_unit(m_active_texture_unit_call);
}

void TextureStateMap::post_unbind(unsigned target, PTraceCall call)
{
    (void)target;
    if (m_active_texture_unit_call)
        call->set_required_call(m_active_texture_unit_call);

}

PTraceCall
TextureStateMap::set_data(const trace::Call& call)
{
    auto texture = bound_in_call(call);

    if (!texture)
        return trace2call(call);
    return texture->data(call);
}

PTraceCall TextureStateMap::storage(const trace::Call& call)
{
    auto texture = bound_in_call(call);
    if (!texture)
        return trace2call(call);
    return texture->storage(call);
}

PTraceCall
TextureStateMap::set_state(const trace::Call& call, unsigned nparam_sel)
{
    auto texture = bound_in_call(call);

    if (!texture)
        return trace2call(call);
    return texture->set_state_call(call, nparam_sel);
}

PTraceCall
TextureStateMap::set_sub_data(const trace::Call& call)
{
    /* This will need some better handling */
    auto texture = bound_in_call(call);

    if (!texture)
        return trace2call(call);

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
    case GL_TEXTURE_CUBE_MAP:
        target = gl_texture_cube;
        break;
    case GL_PROXY_TEXTURE_1D:
    case GL_TEXTURE_1D:
        target = gl_texture_1d;
        break;
    case GL_PROXY_TEXTURE_2D:
    case GL_TEXTURE_2D:
        target = gl_texture_2d;
        break;
    case GL_PROXY_TEXTURE_3D:
    case GL_TEXTURE_3D:
        target = gl_texture_3d;
        break;
    case GL_TEXTURE_CUBE_MAP_ARRAY:
        target = gl_texture_cube_array;
        break;
    case GL_TEXTURE_1D_ARRAY:
        target = gl_texture_1d_array;
        break;
    case GL_TEXTURE_2D_ARRAY:
        target = gl_texture_2d_array;
        break;
    case GL_TEXTURE_2D_MULTISAMPLE:
        target = gl_texture_2dms;
        break;
    case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
        target = gl_texture_2dms_array;
        break;
    case GL_TEXTURE_BUFFER:
        target = gl_texture_buffer;
        break;
    default:
        std::cerr << "target=" << target << "not supported\n";
        assert(0);
        ;
    }

    return target + unit * gl_texture_last;
}

}

