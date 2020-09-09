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

FramebufferObject::FramebufferObject(unsigned gl_id, PTraceCall gen_call):
    GenObject(gl_id, gen_call)
{

}

void FramebufferObject::set_state(PTraceCall call)
{
    m_state_calls.push_back(call);
}

void FramebufferObject::viewport(const trace::Call& call)
{
    m_viewport_x = call.arg(0).toUInt();
    m_viewport_y = call.arg(1).toUInt();
    m_viewport_width = call.arg(2).toUInt();
    m_viewport_height = call.arg(3).toUInt();

    m_state_calls.push_back(make_shared<StateCall>(call, 0));
}

void FramebufferObject::collect_data_calls(CallIdSet& calls, unsigned call_before)
{
    unsigned start_redraw_call = std::numeric_limits<unsigned>::max();
    for (auto&& c : m_draw_calls) {
        if (c->call_no() >= call_before)
            continue;
        calls.insert(c->call_no());
        if (c->test_flag(TraceCall::full_viewport_redraw)) {
            start_redraw_call = c->call_no();
            break;
        }
    }

    /* all state changes during the draw must be recorded */
    std::unordered_set<std::string> singular_states;

    for (auto&& c : m_state_calls) {
        if (c->call_no() >= call_before)
            continue;
        if (c->call_no() > start_redraw_call)
            calls.insert(c->call_no());
        auto state = c->name();
        if (singular_states.find(state) == singular_states.end()) {
            singular_states.insert(state);
            calls.insert(c->call_no());
        }
    }
}

void FramebufferObject::collect_dependend_obj(Queue& objects)
{
    for (auto&& a : m_attachments)
        if (a && !a-visited())
            objects.push(a);
}


void FramebufferObject::clear(const trace::Call& call)
{
    bool full_clear = true;
    if (m_viewport_width != m_width ||
        m_viewport_height != m_height)
        full_clear = false;

    /* TODO: Check attachments vs. draw buffers */

    auto c = make_shared<TraceCall>(call);
    if (full_clear)
        c->set_flag(TraceCall::full_viewport_redraw);
    m_draw_calls.push_front(c);
}

void FramebufferObject::draw(PTraceCall call)
{
    m_draw_calls.push_front(call);
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

    if (idx >= m_attachments.size())
        m_attachments.resize(idx + 1);

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

    PTraceCall retval;
    if (target == GL_FRAMEBUFFER ||
        target == GL_DRAW_FRAMEBUFFER) {
        m_draw_buffer = fbo;
    }


    if (target == GL_FRAMEBUFFER ||
        target == GL_READ_FRAMEBUFFER) {
        m_read_buffer = fbo;
    }
    return fbo ? make_shared<TraceCallOnBoundObj>(call, fbo) :
                 make_shared<TraceCall>(call);
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
    assert(0 && "Blit not correctly implemented");
    return make_shared<TraceCall>(call);
}

PTraceCall
FramebufferObjectMap::attach_renderbuffer(const trace::Call& call, RenderbufferObjectMap& rb_map)
{
    PFramebufferObject fbo = bound_to_call_target(call);
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

    PFramebufferObject fbo = bound_to_call_target(call);
    fbo->attach(attach_point, tex, layer);

    return make_shared<TraceCallOnBoundObjWithDeps>(call, fbo, tex);
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
