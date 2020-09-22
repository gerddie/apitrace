#include "ft_framebuffer.hpp"
#include "ft_texturestate.hpp"


#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim {

using std::make_shared;

FramebufferStateMap::FramebufferStateMap()
{
    m_current_framebuffer = make_shared<DefaultFramebufferState>();
    set(0, m_current_framebuffer);
}

PTraceCall FramebufferStateMap::viewport(const trace::Call& call)
{
    return m_current_framebuffer->viewport(call);
}

PTraceCall FramebufferStateMap::clear(const trace::Call& call)
{
    return m_current_framebuffer->clear(call);
}

PTraceCall FramebufferStateMap::draw(const trace::Call& call)
{
    return m_current_framebuffer->draw(call);
}

void FramebufferState::set_size(unsigned width, unsigned height)
{
    m_width = width;
    m_height = height;
}

void FramebufferState::set_viewport_size(unsigned width, unsigned height)
{
    (void)width;
    (void)height;
}

void
DefaultFramebufferState::attach(unsigned index, PSizedObjectState attachment,
                    unsigned layer, PTraceCall call)
{
    (void)index;
    (void)attachment;
    (void)layer;
    (void)call;

    assert(0 && "Default framebuffers can't have attachments");
}

void DefaultFramebufferState::set_viewport_size(unsigned width, unsigned height)
{
    if (!m_initial_viewport_set) {
        set_size(width, height);
        m_initial_viewport_set = true;
    }
}

PTraceCall
FramebufferStateMap::attach_texture(const trace::Call &call,
                                    TextureStateMap &texture_map,
                                    unsigned tex_param_idx,
                                    unsigned level_param_idx)
{
    auto attach_point = call.arg(1).toUInt();
    auto tex = texture_map.get_by_id(call.arg(tex_param_idx).toUInt());
    auto layer = call.arg(level_param_idx).toUInt();

    auto fbo = bound_to_call_target(call);

    PTraceCall c = make_shared<TraceCall>(call);
    if (tex)
        tex->rendertarget_of(layer, fbo);

    fbo->attach(attach_point, tex, layer, c);
    return c;
}

FramebufferState::Pointer
FramebufferStateMap::bound_to_call_target(const trace::Call& call) const
{
    auto target = call.arg(0).toUInt();

    if (target == GL_FRAMEBUFFER ||
            target == GL_DRAW_FRAMEBUFFER) {
        return m_current_framebuffer;
    } else
        return m_read_buffer;
}

DefaultFramebufferState::DefaultFramebufferState():
    FramebufferState(0, nullptr),
    m_initial_viewport_set(false)
{

}

FramebufferState::FramebufferState(GLint glID, PTraceCall gen_call):
    GenObjectState(glID, gen_call),
    m_width(0),
    m_height(0),
    m_viewport_x(0),
    m_viewport_y(0),
    m_viewport_width(0),
    m_viewport_height(0)
{
}

PTraceCall FramebufferState::viewport(const trace::Call& call)
{
    m_viewport_x = call.arg(0).toUInt();
    m_viewport_y = call.arg(1).toUInt();
    m_viewport_width = call.arg(2).toUInt();
    m_viewport_height = call.arg(3).toUInt();

    set_viewport_size(m_viewport_width, m_viewport_height);
    return trace2call(call);
}

PTraceCall FramebufferState::clear(const trace::Call& call)
{
    return trace2call(call);
}

PTraceCall FramebufferState::draw(const trace::Call& call)
{
    return trace2call(call);
}


}
