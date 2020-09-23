#include "ft_framebuffer.hpp"
#include "ft_texturestate.hpp"
#include "ft_renderbuffer.hpp"


#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim {

using std::make_shared;

FramebufferStateMap::FramebufferStateMap()
{
    m_default_framebuffer = make_shared<DefaultFramebufferState>();
    set(0, m_default_framebuffer);
    m_read_buffer = m_current_framebuffer = m_default_framebuffer;
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



void FramebufferStateMap::post_bind(unsigned target,
                                    FramebufferState::Pointer fbo)
{
    if (target == GL_FRAMEBUFFER ||
        target == GL_DRAW_FRAMEBUFFER) {
        m_current_framebuffer = fbo;
    }

    if (target == GL_FRAMEBUFFER ||
        target == GL_READ_FRAMEBUFFER) {
        m_read_buffer = fbo;
    }
}

void FramebufferStateMap::post_unbind(unsigned target,
                                      PTraceCall call)
{
    (void)call;
    if (target == GL_FRAMEBUFFER ||
        target == GL_DRAW_FRAMEBUFFER) {
        m_current_framebuffer = m_default_framebuffer;
    }

    if (target == GL_FRAMEBUFFER ||
        target == GL_READ_FRAMEBUFFER) {
        m_read_buffer = m_default_framebuffer;
    }
    m_default_framebuffer->bind(call);
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

void FramebufferState::append_state_cache(unsigned object_id, PCallSet cache)
{
    assert(cache);
    m_dependend_states[object_id] = cache;
}

PTraceCall FramebufferState::blit(const trace::Call& call)
{
    auto c = trace2call(call);

    unsigned dest_x = call.arg(4).toUInt();
    unsigned dest_y = call.arg(5).toUInt();
    unsigned dest_w = call.arg(6).toUInt();
    unsigned dest_h = call.arg(7).toUInt();

    if (dest_x == 0 && dest_y == 0 &&
        dest_w == m_width && dest_h == m_height) {
        m_draw_calls.clear();
        m_draw_calls.insert(bind_call());
    }
    m_draw_calls.insert(c);
    return c;
}

PTraceCall FramebufferState::draw_buffer(const trace::Call& call)
{
    m_drawbuffer_call = trace2call(call);
    m_drawbuffer_call->set_required_call(bind_call());
    return m_drawbuffer_call;
}

PTraceCall FramebufferState::read_buffer(const trace::Call& call)
{
    m_readbuffer_call = trace2call(call);
    m_readbuffer_call->set_required_call(bind_call());
    return m_readbuffer_call;
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

PTraceCall
FramebufferStateMap::attach_renderbuffer(const trace::Call& call,
                                         RenderbufferMap& renderbuffer_map)
{
    auto attach_point = call.arg(1).toUInt();
    std::cerr << "Get Renderbuffer with ID " << call.arg(3).toUInt() << "\n";

    auto rb = renderbuffer_map.get_by_id(call.arg(3).toUInt());
    auto fbo = bound_to_call_target(call);

    assert(fbo);
    assert(fbo->id() > 0);

    PTraceCall c = make_shared<TraceCall>(call);
    if (rb)
        rb->attach_as_rendertarget(fbo);
    fbo->attach(attach_point, rb, 0, c);
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

PTraceCall FramebufferStateMap::blit(const trace::Call& call)
{
    auto c = m_current_framebuffer->blit(call);
    m_read_buffer->flush_state_cache(*m_current_framebuffer);
    return c;
}

PTraceCall FramebufferStateMap::draw_buffer(const trace::Call& call)
{
    return m_current_framebuffer->draw_buffer(call);
}

PTraceCall FramebufferStateMap::read_buffer(const trace::Call& call)
{
    return m_read_buffer->draw_buffer(call);
}

DefaultFramebufferState::DefaultFramebufferState():
    FramebufferState(0, nullptr),
    m_initial_viewport_set(false)
{

}

bool DefaultFramebufferState::is_active() const
{
    return true;
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

    /* There may be cases where the viewport is not full sized and
     * we have to track more than one call */
    m_viewport_call = trace2call(call);
    return m_viewport_call;
}

PTraceCall FramebufferState::clear(const trace::Call& call)
{
    if (m_width == m_viewport_width &&
        m_height == m_viewport_height)
        m_draw_calls.clear();

    auto c = trace2call(call);
    m_draw_calls.insert(c);
    return c;
}

PTraceCall FramebufferState::draw(const trace::Call& call)
{
    auto c = trace2call(call);
    m_draw_calls.insert(c);
    return c;
}

void FramebufferState::do_emit_calls_to_list(CallSet& list) const
{
    list.insert(m_viewport_call);
    emit_bind(list);
    list.insert(m_draw_calls);

    for(auto&& deps : m_dependend_states)
        list.insert(*deps.second);

    emit_attachment_calls_to_list(list);

    if (m_readbuffer_call)
        list.insert(m_readbuffer_call);

    if (m_drawbuffer_call)
        list.insert(m_drawbuffer_call);
}

void FramebufferState::emit_attachment_calls_to_list(CallSet& list) const
{
    (void)list;
}

void FBOState::attach(unsigned index, PSizedObjectState attachment,
                      unsigned layer, PTraceCall call)
{
    (void)layer;
    (void)call;
    m_attachments[index] = attachment;
    m_attach_call[index] = std::make_pair(bind_call(), call);

    if (attachment)
        attachment->flush_state_cache(*this);
}

void FBOState::emit_attachment_calls_to_list(CallSet& list) const
{
    for (auto&& a : m_attachments)
        if (a.second)
            a.second->emit_calls_to_list(list);

    for (auto&& a : m_attach_call) {
        if (a.second.first)
            list.insert(a.second.first);
        if (a.second.second)
            list.insert(a.second.second);
    }

}

void FBOState::set_viewport_size(unsigned width, unsigned height)
{
    (void)width;
    (void)height;
}


}
