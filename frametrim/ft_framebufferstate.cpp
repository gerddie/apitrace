#include "ft_framebufferstate.hpp"
#include "ft_texturestate.hpp"
#include <GL/glext.h>
#include <cstring>
#include <iostream>

namespace frametrim {

FramebufferState::FramebufferState(GLint glID, PCall gen_call):
    GenObjectState(glID, gen_call),
    m_width(0),
    m_height(0),
    m_viewport_full_size(false),
    m_attached_buffer_types(0),
    m_attached_color_buffer_mask(0)
{
}

void FramebufferState::bind(PCall call)
{
    m_bind_call = call;
}

void FramebufferState::attach(unsigned attachment, PCall call,
                              PSizedObjectState att)
{
    if (m_attachments[attachment] && att &&
            (*m_attachments[attachment] == *att))
        return;

    m_attachments[attachment] = att;
    m_attach_calls[attachment].insert(m_bind_call);
    m_attach_calls[attachment].insert(call);

    if (att) {
        m_width = att->width();
        m_height = att->height();
    } else {
        m_attach_calls[attachment].clear();
    }

    m_attached_buffer_types = 0;
    m_attached_color_buffer_mask = 0;

    for(auto& a: m_attachments) {
        if (a.second) {
            switch (a.first) {
            case GL_DEPTH_ATTACHMENT:
                m_attached_buffer_types |= GL_DEPTH_BUFFER_BIT;
                break;
            case GL_STENCIL_ATTACHMENT:
                m_attached_buffer_types |= GL_STENCIL_BUFFER_BIT;
                break;
            default:
                m_attached_buffer_types |= GL_COLOR_BUFFER_BIT;
                m_attached_color_buffer_mask |= 1 << (a.first - GL_COLOR_ATTACHMENT0);
            }
        }
    }
}

void FramebufferState::draw(PCall call)
{
    append_call(call);
}

CallSet& FramebufferState::state_calls()
{
    return m_draw_prepare;
}

void FramebufferState::set_viewport(PCall call)
{
    m_viewport_call = call;
    m_viewport_full_size =
            call->arg(0).toUInt() == 0 &&
            call->arg(1).toUInt() == 0 &&
            call->arg(2).toUInt() == m_width &&
            call->arg(3).toUInt() == m_height;
}

void FramebufferState::clear(PCall call)
{
    /* If the viewport covers the full size, and we clear all buffer tyoes
    * so we can forget all older draw calls
    * FIXME: Technically it is possible that some buffer is masked out and
    * its contents should be retained from earlier draw calls. */
    unsigned flags = m_attached_buffer_types & ~call->arg(0).toUInt();

    if (m_viewport_full_size && !flags)
        reset_callset();

    append_call(m_bind_call);

    append_call(call);
}

void FramebufferState::depends(PGenObjectState read_buffer)
{
    // TODO: we should check whether the drawn region is covering the
    // whole target so that we can remove un-needed calls
    m_read_dependencies.insert(read_buffer);
}

void FramebufferState::do_emit_calls_to_list(CallSet& list) const
{
    if (m_bind_call)  {
        if (list.m_debug)
            std::cerr << "Emit calls for FBO " << id() << "\n";

        emit_gen_call(list);

        list.insert(m_bind_call);

        for (auto& a : m_attach_calls)
            list.insert(a.second);

        list.insert(m_draw_prepare);

        for (auto& d : m_read_dependencies) {
            if (d)
                d->emit_calls_to_list(list);
        }

        for(auto& a: m_attachments)
            if (a.second)
                a.second->emit_calls_to_list(list);

        if (list.m_debug)
            std::cerr << "Done FBO " << id() << "\n";
    }

}


void FramebufferMap::bind(PCall call)
{
    unsigned target = call->arg(0).toUInt();
    unsigned id = call->arg(1).toUInt();

    if (id)  {

        if ((target == GL_DRAW_FRAMEBUFFER ||
             target == GL_FRAMEBUFFER) &&
                (!m_draw_framebuffer ||
                 m_draw_framebuffer->id() != id)) {
            m_draw_framebuffer = get_by_id(id);
            m_draw_framebuffer->bind(call);
            m_last_unbind_draw_fbo = nullptr;

            /* TODO: Create a copy of the current state and record all
          * calls in the framebuffer until it is un-bound
          * attach the framebuffer to any texture that might be
          * attached as render target */

            auto& calls = m_draw_framebuffer->state_calls();
            calls.clear();
            global_state().collect_state_calls(calls);
        }

        if ((target == GL_READ_FRAMEBUFFER ||
             target == GL_FRAMEBUFFER) &&
                (!m_read_framebuffer ||
                 m_read_framebuffer->id() != id)) {
            m_read_framebuffer = get_by_id(id);
            m_read_framebuffer->bind(call);
            m_last_unbind_read_fbo = nullptr;
        }

    } else {

        if (target == GL_DRAW_FRAMEBUFFER ||
                target == GL_FRAMEBUFFER) {
            m_draw_framebuffer = nullptr;
            m_last_unbind_draw_fbo  = call;
        }

        if (target == GL_READ_FRAMEBUFFER ||
                target == GL_FRAMEBUFFER) {
            m_read_framebuffer = nullptr;
            m_last_unbind_read_fbo  = call;
        }
    }
}

void FramebufferMap::blit(PCall call)
{
    (void)call;
    assert(m_read_framebuffer);

    if (m_draw_framebuffer) {
        m_draw_framebuffer->depends(m_read_framebuffer);
        m_draw_framebuffer->append_call(call);
    }
}

void FramebufferMap::renderbuffer(PCall call, RenderbufferMap& renderbuffers)
{
    unsigned target = call->arg(0).toUInt();
    unsigned attachment = call->arg(1).toUInt();
    unsigned rb_id = call->arg(3).toUInt();

    auto rb = rb_id ? renderbuffers.get_by_id(rb_id) : nullptr;

    PFramebufferState  draw_fb;
    bool read_rb = false;

    if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
        m_draw_framebuffer->attach(attachment, call, rb);
        draw_fb = m_draw_framebuffer;
    }

    if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
        m_read_framebuffer->attach(attachment, call, rb);
        read_rb = true;
    }

    if (rb)
        rb->attach(call, read_rb, draw_fb);
}

void FramebufferMap::texture(PCall call, TextureStateMap& textures)
{
    unsigned layer = 0;
    unsigned textarget = 0;
    unsigned has_tex_target = strcmp("glFramebufferTexture", call->name()) != 0;

    unsigned target = call->arg(0).toUInt();
    unsigned attachment = call->arg(1).toUInt();

    if (has_tex_target)
        textarget = call->arg(2).toUInt();

    unsigned texid = call->arg(2 + has_tex_target).toUInt();
    unsigned level = call->arg(3 + has_tex_target).toUInt();

    if (!strcmp(call->name(), "glFramebufferTexture3D"))
        layer = call->arg(5).toUInt();

    assert(level < 16);
    assert(layer < 1 << 12);

    unsigned textargetid = (textarget << 16) | level << 12 | level;

    auto texture = textures.get_by_id(texid);

    if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
        m_draw_framebuffer->attach(attachment, call, texture);
        texture->rendertarget_of(textargetid, m_draw_framebuffer);
    }

    if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
        m_read_framebuffer->attach(attachment, call, texture);
    }
}

PFramebufferState FramebufferMap::draw_fb()
{
    return m_draw_framebuffer;
}

PFramebufferState FramebufferMap::read_fb()
{
    return m_read_framebuffer;
}

void FramebufferMap::emit_calls_to_list(CallSet& list) const
{
    if (m_last_unbind_draw_fbo)
        list.insert(m_last_unbind_draw_fbo);

    if (m_last_unbind_read_fbo)
        list.insert(m_last_unbind_read_fbo);
}

RenderbufferState::RenderbufferState(GLint glID, PCall gen_call):
    SizedObjectState(glID, gen_call, renderbuffer)
{
}

void RenderbufferState::attach(PCall call, bool read, PFramebufferState write_fb)
{
    if (read)
        m_attach_read_fb_call = call;

    if (write_fb) {
        m_attach_write_fb_call = call;
        m_data_source = write_fb;
    }
}

void RenderbufferState::set_used_as_blit_source()
{
    m_is_blit_source = true;
}

void RenderbufferState::set_storage(PCall call)
{
    m_set_storage_call = call;
    unsigned w = call->arg(2).toUInt();
    unsigned h = call->arg(2).toUInt();
    set_size(0, w, h);
}

void RenderbufferState::do_emit_calls_to_list(CallSet& list) const
{
    emit_gen_call(list);

    if (m_set_storage_call)
        list.insert(m_set_storage_call);

    if (m_attach_read_fb_call)
        list.insert(m_attach_read_fb_call);

    if (m_attach_write_fb_call)
        list.insert(m_attach_write_fb_call);

    if (m_is_blit_source) {
        assert(m_data_source);
        m_data_source->emit_calls_to_list(list);
    }
}

void RenderbufferMap::bind(PCall call)
{
    assert(call->arg(0).toUInt() == GL_RENDERBUFFER);

    auto id = call->arg(1).toUInt();

    if (id) {
        if (!m_active_renderbuffer || m_active_renderbuffer->id() != id) {
            m_active_renderbuffer = get_by_id(id);
            if (!m_active_renderbuffer) {
                std::cerr << "No renderbuffer in " << __func__
                          << " with call " << call->no
                          << " " << call->name() << "\n";
                assert(0);
            }
            m_active_renderbuffer->append_call(call);
        }
        m_last_unbind_call = nullptr;
    } else {
        /* Need to keep track when id == 0 */
        m_last_unbind_call = call;
    }
}

void RenderbufferMap::storage(PCall call)
{
    assert(m_active_renderbuffer);
    m_active_renderbuffer->set_storage(call);
}

void RenderbufferMap::emit_calls_to_list(CallSet& list) const
{
    /* Renderbuffers are only of interest if they are bound, so no
     * need to emit the related calls here, because the framebuffer will take
     * care of this, but we have to unbind any renderbuffer if that's
     * what the trace did before the current state is recorded. */
    if (m_last_unbind_call)
       list.insert(m_last_unbind_call);
}

}
