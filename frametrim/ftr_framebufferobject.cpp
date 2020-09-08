#include "ftr_framebufferobject.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim_reverse {

using std::make_shared;

unsigned RenderbufferObject::evaluate_size(const trace::Call& call)
{
    unsigned w = call.arg(2).toUInt();
    unsigned h = call.arg(3).toUInt();
    set_size(0,w,h);
    return 0;
}

PTraceCall
RenderbufferObjectMap::storage(const trace::Call& call)
{
    auto rb = bound_to_call_target(call);
    assert(rb);
    rb->allocate(call);
    return make_shared<TraceCallOnBoundObj>(call, rb);
}

void
FramebufferObject::attach(unsigned attach_point, PAttachableObject obj, unsigned layer)
{
    unsigned idx = 0;
    switch (attach_point) {
    case GL_DEPTH_ATTACHMENT:
        idx = 0;
        break;
    case GL_STENCIL_ATTACHMENT:
        idx = 1;
        break;
    default:
        idx = attach_point - GL_COLOR_ATTACHMENT0 + 2;
    }

    if (m_attachments[idx])
        m_attachments[idx]->detach_from(id());

    if (obj) {
        m_width = obj->width(layer);
        m_height = obj->width(layer);
    }

    m_attachments[idx] = obj;
}

PTraceCall
FramebufferObjectMap::bind(const trace::Call& call)
{
    auto target = call.arg(0).toUInt();
    auto fbo = by_id(call.arg(1).toUInt());

    if (target == GL_FRAMEBUFFER ||
        target == GL_DRAW_FRAMEBUFFER) {
        m_draw_buffer = fbo;
    }

    if (target == GL_FRAMEBUFFER ||
        target == GL_READ_FRAMEBUFFER) {
        m_read_buffer = fbo;
    }
    return make_shared<TraceCall>(call);
}

PTraceCall
FramebufferObjectMap::blit(const trace::Call& call)
{
    assert(0 && "Blit not correctly implemented");
    return make_shared<TraceCall>(call);
}

PTraceCall
FramebufferObjectMap::attach_renderbuffer(const trace::Call& call, RenderbufferObjectMap& rb_map)
{
    auto fbo = bound_to_call_target(call);
    auto rb = rb_map.by_id(call.arg(3).toUInt());
    auto attach_point = call.arg(1).toUInt();

    fbo->attach(attach_point, rb, 0);
    rb->attach_to(fbo);
    return make_shared<TraceCallOnBoundObjWithDeps>(call, fbo, rb);
}

PTraceCall
FramebufferObjectMap::attach_texture(const trace::Call& call,
                                     TexObjectMap& tex_map,
                                     unsigned tex_param_idx,
                                     unsigned level_param_idx)
{
    auto attach_point = call.arg(1).toUInt();
    auto tex = tex_map.by_id(call.arg(tex_param_idx).toUInt());
    auto layer = call.arg(level_param_idx).toUInt();

    auto target = call.arg(0).toUInt();

    PFramebufferObject fbo;
    if (target == GL_FRAMEBUFFER || GL_DRAW_FRAMEBUFFER) {
        fbo = bound_to_call_target(call);
        fbo->attach(attach_point, tex, layer);
    } else {
        assert(GL_READ_FRAMEBUFFER);
        fbo = bound_to_call_target(call);
    }
    return make_shared<TraceCallOnBoundObjWithDeps>(call, fbo, tex);
}

PTraceCall FramebufferObjectMap::attach_texture3d(const trace::Call& call,
                                                  TexObjectMap& tex_map)
{
    return attach_texture(call, tex_map, 3, 4);
}



}
