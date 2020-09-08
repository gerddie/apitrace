#include "ftr_texobject.hpp"
#include "ftr_tracecallext.hpp"
#include "ftr_boundobject.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim_reverse {

using std::make_shared;

unsigned TexObject::evaluate_size(const trace::Call& call)
{
    unsigned level = call.arg(1).toUInt();
    unsigned w = call.arg(3).toUInt();
    unsigned h = 1;

    if (!strcmp(call.name(), "glTexImage2D")||
        !strcmp(call.name(), "glCompressedTexImage2D")||
        !strcmp(call.name(), "glTexImage3D")) {
        h = call.arg(4).toUInt();
    }
    set_size(level, w, h);
    return level;
}

TexObjectMap::TexObjectMap():
    m_active_texture_unit(0)
{

}

PTraceCall TexObjectMap::active_texture(const trace::Call& call)
{
    m_active_texture_unit = call.arg(0).toUInt();
    return make_shared<TraceCall>(call);
}

PTraceCall TexObjectMap::allocation(const trace::Call& call)
{
    auto texture = bound_to_call_target(call);
    texture->allocate(call);
    return make_shared<TraceCallOnBoundObj>(call, texture);
}

PTraceCall TexObjectMap::bind_multitex(const trace::Call& call)
{
    auto unit = call.arg(0).toUInt() - GL_TEXTURE0;
    auto target =  call.arg(1).toUInt();
    auto id = call.arg(2).toUInt();

    auto target_id = compose_target_id_with_unit(target, unit);

    auto obj = bind_target(target_id, id);
    return make_shared<TraceCallOnBoundObj>(call, obj);
}

unsigned TexObjectMap::target_id_from_call(const trace::Call& call) const
{
    unsigned target = call.arg(0).toUInt();
    return compose_target_id_with_unit(target, m_active_texture_unit);
}

unsigned TexObjectMap::compose_target_id_with_unit(unsigned target,
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

