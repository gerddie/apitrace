#include "ft_framebuffer.hpp"
#include "ft_texturestate.hpp"
#include "ft_programstate.hpp"
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

PTraceCall FramebufferStateMap::bind_fbo(const trace::Call& call,
                                         bool recording)
{
    unsigned target = call.arg(0).toUInt();
    unsigned id = call.arg(1).toUInt();
    auto c = trace2call(call);

    if (target == GL_FRAMEBUFFER ||
        target == GL_DRAW_FRAMEBUFFER) {
        bind_target(GL_DRAW_FRAMEBUFFER, id, c);
    }

    if (target == GL_FRAMEBUFFER ||
        target == GL_READ_FRAMEBUFFER) {
        bind_target(GL_READ_FRAMEBUFFER, id, c);
        m_read_buffer->set_readbuffer_bind_call(c);
    }

    if (recording && m_current_framebuffer->id() > 0)
        m_current_framebuffer->flush_state_cache(*m_default_framebuffer);

    return c;
}

void FramebufferStateMap::post_bind(unsigned target,
                                    FramebufferState::Pointer fbo)
{
    if (!fbo)
        fbo = m_default_framebuffer;

    if (target == GL_FRAMEBUFFER ||
        target == GL_DRAW_FRAMEBUFFER) {
        m_current_framebuffer = fbo;
    }

    if (target == GL_FRAMEBUFFER ||
        target == GL_READ_FRAMEBUFFER) {
        m_read_buffer = fbo;
    }

    if (m_current_framebuffer->id() != m_read_buffer->id())
        m_read_buffer->flush_state_cache(*m_current_framebuffer);
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
    m_default_framebuffer->bind(target, call);
}

void FramebufferStateMap::do_emit_calls_to_list(CallSet& list) const
{
    m_default_framebuffer->emit_calls_to_list(list);
    if (m_current_framebuffer->id())
        m_current_framebuffer->emit_calls_to_list(list);

    if (m_read_buffer->id() &&
        (m_read_buffer->id() != m_current_framebuffer->id()))
        m_read_buffer->emit_calls_to_list(list);
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

void FramebufferState::pass_state_cache(unsigned object_id, PCallSet cache)
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

    if (!m_draw_calls.has(CallSet::attach_calls)) {
        emit_attachment_calls_to_list(m_draw_calls);
        m_draw_calls.set(CallSet::attach_calls);
    }

    m_draw_calls.insert(c);
    dirty_cache();
    return c;
}

PTraceCall FramebufferState::draw_buffer(const trace::Call& call)
{
    m_drawbuffer_call = trace2call(call);
    if (call.arg(0).toUInt() == GL_BACK && id() != 0) {
        std::cerr << "state error in call " << m_drawbuffer_call->call_no()
                  << ": framebuffer ID is " << id() << "\n";
        assert(0);
    }
    m_drawbuffer_call->set_required_call(bind_call());


    dirty_cache();
    return m_drawbuffer_call;
}

void FramebufferState::set_readbuffer_bind_call(PTraceCall call)
{
    m_bind_as_readbuffer_call = call;
}


PTraceCall FramebufferState::read_buffer(const trace::Call& call)
{
    m_readbuffer_call = trace2call(call);
    m_readbuffer_call->set_required_call(m_bind_as_readbuffer_call);
    return m_readbuffer_call;
}

void FramebufferState::post_bind(unsigned target, const PTraceCall &call)
{
    (void)target;
    (void)call;
    m_bind_dirty = true;
}

PTraceCall FramebufferState::readbuffer_call(unsigned attach_id)
{
    (void)attach_id;
    return bind_call();
}

void
DefaultFramebufferState::attach(unsigned index, PSizedObjectState attachment,
                                unsigned layer, PTraceCall call)
{
    (void)index;
    (void)attachment;
    (void)layer;
    (void)call;

    std::cerr << "Default framebuffers can't have attachments in call " <<
                 call->call_no() << "\n";
    assert(0);
}

void DefaultFramebufferState::set_viewport_size(unsigned width, unsigned height)
{
    if (!m_initial_viewport_set) {
        set_size(width, height);
        m_initial_viewport_set = true;
    }
}

bool DefaultFramebufferState::clear_all_buffers(unsigned mask) const
{
    /* TODO: acquire the actual buffers from the creation call */
    /* We assume that clearing the depth buffer means that the draw
     * will overwrite the whole output */
    if (mask & GL_DEPTH_BUFFER_BIT)
        return true;
    return false;
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

    auto rb = renderbuffer_map.get_by_id(call.arg(3).toUInt());
    auto fbo = bound_to_call_target(call);

    assert(fbo);
    assert(fbo->id() > 0);

    PTraceCall c = make_shared<TraceCall>(call);
    if (rb) {
        rb->attach_as_rendertarget(c);
    }

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
    return m_read_buffer->read_buffer(call);
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

void DefaultFramebufferState::pass_state_cache(unsigned object_id, PCallSet cache)
{
    (void)object_id;
    if (cache) {
        cache->resolve();
        for (auto&& c: *cache)
            append_call(c);
    }
}

FramebufferState::FramebufferState(GLint glID, PTraceCall gen_call):
    GenObjectState(glID, gen_call),
    m_width(0),
    m_height(0),
    m_viewport_x(0),
    m_viewport_y(0),
    m_viewport_width(0),
    m_viewport_height(0),
    m_bind_dirty(false)
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
    auto c = trace2call(call);
    if (m_viewport_calls.empty() || (*m_viewport_calls.rbegin())->name_with_params() !=
        c->name_with_params() || m_bind_dirty) {

        if (m_bind_dirty)
            m_viewport_calls.push_back(bind_call());
        m_viewport_calls.push_back(c);
        m_bind_dirty = false;
    }

    dirty_cache();
    return c;
}

PTraceCall FramebufferState::clear(const trace::Call& call)
{
    assert(id() == 0 || (m_width > 0 &&  m_height > 0));

    if (m_width == m_viewport_width &&
        m_height == m_viewport_height &&
        clear_all_buffers(call.arg(0).toUInt())) {
        m_draw_calls.clear();
        m_draw_calls.insert(bind_call());
    }

    if (!m_draw_calls.has(CallSet::attach_calls)) {
        emit_attachment_calls_to_list(m_draw_calls);
        m_draw_calls.set(CallSet::attach_calls);
    }

    auto c = trace2call(call);
    m_draw_calls.insert(c);
    dirty_cache();

    return c;
}

void FramebufferState::draw(PTraceCall call)
{
    if (!m_draw_calls.has(CallSet::attach_calls)) {
        emit_attachment_calls_to_list(m_draw_calls);
        m_draw_calls.set(CallSet::attach_calls);
    }
    m_draw_calls.insert(call);
    dirty_cache();
}

void FramebufferState::do_emit_calls_to_list(CallSet& list) const
{
    for (auto&& vc : m_viewport_calls)
        list.insert(vc);
    emit_gen_call(list);
    emit_bind(list);
    list.insert(m_draw_calls);

    emit_attachment_calls_to_list(list);

    if (m_readbuffer_call)
        list.insert(m_readbuffer_call);

    if (m_drawbuffer_call)
        list.insert(m_drawbuffer_call);
}

void FramebufferState::set_dependend_state_cache(unsigned key, PCallSet sc)
{
    m_dependend_states[key] = sc;
}

void FramebufferState::emit_dependend_caches(CallSet& list) const
{
    for(auto&& deps : m_dependend_states)
        list.insert(deps.first, deps.second);
}

void FramebufferState::emit_attachment_calls_to_list(CallSet& list) const
{
    (void)list;
}

void FBOState::attach(unsigned index, PSizedObjectState attachment,
                      unsigned layer, PTraceCall call)
{
    (void)layer;

    if (m_attachments[index] &&
        (!attachment ||
        m_attachments[index]->global_id() != attachment->global_id())) {
        flush_state_cache(*m_attachments[index]);
        m_attachments[index]->unattach();
    }
    call->set_required_call(bind_call());
    m_attach_call[index] = call;

    if (attachment) {
        attachment->flush_state_cache(*this);

        set_size(attachment->width(),
                 attachment->height());
    }

    m_attachments[index] = attachment;
    call->set_required_call(bind_call());

    dirty_cache();
}

void FBOState::post_unbind(const PTraceCall& call)
{
    (void)call;

    for (auto&& a: m_attachments) {
        if (a.second)
            flush_state_cache(*a.second);
    }
}

void FBOState::emit_attachment_calls_to_list(CallSet& list) const
{
    for (auto&& a : m_attachments)
        if (a.second)
            a.second->emit_calls_to_list(list);

    for (auto&& a : m_attach_call) {
        if (a.second) {
            a.second->emit_required_calls(list);
            list.insert(a.second);
        }
    }
}

void FBOState::set_viewport_size(unsigned width, unsigned height)
{
    (void)width;
    (void)height;
}

bool FBOState::clear_all_buffers(unsigned mask) const
{
    unsigned attached_mask = 0;
    for (auto&& a : m_attachments) {
        if (a.second) {
            switch (a.first) {
            case GL_DEPTH_ATTACHMENT:
                attached_mask |= GL_DEPTH_BUFFER_BIT;
                break;
            case GL_STENCIL_ATTACHMENT:
                attached_mask |= GL_STENCIL_BUFFER_BIT;
                /* fallthrough */
            case GL_DEPTH_STENCIL_ATTACHMENT:
                attached_mask |= GL_DEPTH_BUFFER_BIT;
                break;
            default:
                attached_mask |= GL_COLOR_BUFFER_BIT;
            }
        }
    }
    return (attached_mask & ~(mask | GL_COLOR_BUFFER_BIT)) ? false : true;
}

PTraceCall FBOState::readbuffer_call(unsigned attach_id)
{
    return m_attach_call[attach_id];
}

}
