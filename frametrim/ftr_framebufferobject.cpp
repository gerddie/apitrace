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
    return rb->allocate(call);
}

FramebufferObject::FramebufferObject(unsigned gl_id, PTraceCall gen_call):
    GenObject(gl_id, gen_call),
    m_viewport_width(0),
    m_viewport_height(0),
    m_width(0),
    m_height(0)
{
}

void FramebufferObject::set_state(PTraceCall call)
{
    m_state_calls.push_back(call);
}

void FramebufferObject::bind(PTraceCall call)
{
    m_bind_calls.push_front(call);
}

PTraceCall FramebufferObject::viewport(const trace::Call& call)
{
    m_viewport_x = call.arg(0).toUInt();
    m_viewport_y = call.arg(1).toUInt();
    m_viewport_width = call.arg(2).toUInt();
    m_viewport_height = call.arg(3).toUInt();

    auto c = make_shared<StateCall>(call, 0);
    m_state_calls.push_back(c);
    return c;
}

void FramebufferObject::collect_data_calls(CallIdSet& calls, unsigned call_before)
{
    unsigned start_draw_call = std::numeric_limits<unsigned>::max();
    for (auto&& c : m_draw_calls) {
        if (c->call_no() >= call_before)
            continue;
        calls.insert(c);
        start_draw_call = c->call_no();
        if (c->test_flag(TraceCall::full_viewport_redraw)) {
            break;
        }
    }
    collect_bind_calls(calls, start_draw_call);

    /* all state changes during the draw must be recorded */
    std::unordered_set<std::string> singular_states;

    for (auto&& c : m_state_calls) {
        if (c->call_no() >= call_before)
            continue;
        if (c->call_no() > start_draw_call)
            calls.insert(c);
        auto state = c->name();
        if (singular_states.find(state) == singular_states.end()) {
            singular_states.insert(state);
            calls.insert(c);
        }
    }

    unsigned need_bind_before = start_draw_call;
    for(auto&& attach: m_attachment_calls) {
        unsigned call_no = collect_last_call_before(calls, attach.second, start_draw_call);
        if (call_no < need_bind_before)
            need_bind_before = call_no;

        auto attach_timeline = m_attachments[attach.first];
        for (auto&& a : attach_timeline) {
            if (a.bind_call_no <= call_no &&
                a.unbind_call_no > call_no) {
                a.obj->collect_calls(calls, call_no);
            }
        }
    }

    collect_bind_calls(calls, need_bind_before);
}

void FramebufferObject::collect_bind_calls(CallIdSet& calls, unsigned call_before)
{
    collect_last_call_before(calls, m_bind_calls, call_before);
}

void FramebufferObject::collect_dependend_obj(Queue& objects, const TraceCallRange &call_range)
{
    for (auto&& timeline : m_attachments) {
        for (auto&& binding : timeline.second) {
            if (binding.bind_call_no <= call_range.second &&
                binding.unbind_call_no >= call_range.first &&
                binding.obj && !binding.obj->visited())
                objects.push(binding.obj);
        }
    }
}

PTraceCall FramebufferObject::clear(const trace::Call& call)
{
    bool full_clear = true;
    if (m_viewport_width != m_width ||
        m_viewport_height != m_height) {
        full_clear = false;
    }

    /* TODO: Check attachments vs. draw buffers */

    auto c = make_shared<TraceCall>(call);
    if (full_clear) {
        c->set_flag(TraceCall::full_viewport_redraw);
    }
    m_draw_calls.push_front(c);
    return c;
}

void FramebufferObject::draw(PTraceCall call)
{
    m_draw_calls.push_front(call);
}

void
FramebufferObject::attach(unsigned attach_point, PAttachableObject obj,
                          unsigned layer, PTraceCall call)
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

    if (obj) {
        m_width = obj->width(layer);
        m_height = obj->heigth(layer);
    }

    auto& timeline = m_attachments[idx];
    if (!timeline.empty()) {
        timeline.front().unbind_call_no = call->call_no();
        AttachableObject& o = static_cast<AttachableObject&>(*timeline.front().obj);
        o.detach_from(id(),attach_point, call->call_no());
    }

    timeline.push_front(BindTimePoint(obj, call->call_no()));

    m_attachment_calls[idx].push_front(call);
}

PTraceCall
FramebufferObjectMap::bind(const trace::Call& call)
{
    auto target = call.arg(0).toUInt();
    auto fbo = by_id(call.arg(1).toUInt());

    PTraceCall retval;
    if (target == GL_FRAMEBUFFER ||
        target == GL_DRAW_FRAMEBUFFER) {
        m_draw_buffer = fbo;
    }


    if (target == GL_FRAMEBUFFER ||
        target == GL_READ_FRAMEBUFFER) {
        m_read_buffer = fbo;
    }
    if (fbo) {
        auto c = make_shared<TraceCallOnBoundObj>(call, fbo);
        fbo->bind(c);
        return c;
    } else
        return make_shared<TraceCall>(call);
}

PFramebufferObject FramebufferObjectMap::draw_buffer() const
{
    return m_draw_buffer;
}

PFramebufferObject FramebufferObjectMap::read_buffer() const
{
    return m_read_buffer;
}

PTraceCall
FramebufferObjectMap::blit(const trace::Call& call)
{
    if (m_draw_buffer && m_read_buffer)
        return make_shared<TraceCallOnBoundObj>(call,
                                                m_draw_buffer,
                                                m_read_buffer);

    if (m_draw_buffer)
        return make_shared<TraceCallOnBoundObj>(call, m_draw_buffer);

    if (m_read_buffer)
        return make_shared<TraceCallOnBoundObj>(call, m_read_buffer);

    return make_shared<TraceCall>(call);
}

PTraceCall
FramebufferObjectMap::attach_renderbuffer(const trace::Call& call, RenderbufferObjectMap& rb_map)
{
    PFramebufferObject fbo = bound_to_call_target(call);
    auto rb = rb_map.by_id(call.arg(3).toUInt());
    auto attach_point = call.arg(1).toUInt();

    PTraceCall c;
    if (rb) {
        c = make_shared<TraceCallOnBoundObj>(call, fbo, rb);
        rb->attach_to(fbo, attach_point, call.no);
    } else {
        c = make_shared<TraceCallOnBoundObj>(call, fbo);
    }

    fbo->attach(attach_point, rb, 0, c);

    return c;
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

    PFramebufferObject fbo = bound_to_call_target(call);

    PTraceCall c;
    if (tex) {
        c = make_shared<TraceCallOnBoundObj>(call, fbo, tex);
        tex->attach_to(fbo, attach_point, call.no);
    } else {
        c = make_shared<TraceCallOnBoundObj>(call, fbo);
    }
    fbo->attach(attach_point, tex, layer, c);

    return c;
}

PFramebufferObject
FramebufferObjectMap::bound_to_call_target(const trace::Call& call) const
{
    auto target = call.arg(0).toUInt();

    if (target == GL_FRAMEBUFFER ||
            target == GL_DRAW_FRAMEBUFFER) {
        return m_draw_buffer;
    } else
        return m_read_buffer;
}

PTraceCall FramebufferObjectMap::attach_texture3d(const trace::Call& call,
                                                  TexObjectMap& tex_map)
{
    return attach_texture(call, tex_map, 3, 4);
}



}
