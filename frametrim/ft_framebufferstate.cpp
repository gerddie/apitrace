#include "ft_framebufferstate.hpp"
#include "ft_texturestate.hpp"
#include "ft_globalstate.hpp"
#include <GL/glext.h>
#include <cstring>
#include <iostream>

namespace frametrim {

FramebufferStateBase::FramebufferStateBase(GLint glID, PCall gen_call):
    GenObjectState(glID, gen_call),
    m_width(0),
    m_height(0),
    m_viewport_full_size(false),
    m_attached_buffer_types(0)
{
}

void FramebufferStateBase::set_size(unsigned width, unsigned heigh)
{
    m_width = width;
    m_height = heigh;
}

void FramebufferStateBase::set_attachment_types(unsigned buffer_types)
{
    m_attached_buffer_types = buffer_types;
}

void FramebufferStateBase::draw(PCall call)
{
    append_call(call);
}

CallSet& FramebufferStateBase::state_calls()
{
    return m_draw_prepare;
}

void FramebufferStateBase::set_viewport(PCall call)
{
    m_viewport_call = call;
    m_viewport_full_size =
            call->arg(0).toUInt() == 0 &&
            call->arg(1).toUInt() == 0 &&
            call->arg(2).toUInt() == m_width &&
            call->arg(3).toUInt() == m_height;
}

void FramebufferStateBase::clear(PCall call)
{
    /* If the viewport covers the full size, and we clear all buffer tyoes
    * so we can forget all older draw calls
    * FIXME: Technically it is possible that some buffer is masked out and
    * its contents should be retained from earlier draw calls. */
    unsigned flags = m_attached_buffer_types & ~call->arg(0).toUInt();

    if (m_viewport_full_size && !flags) {
        m_read_dependencies.clear();
        reset_callset();
    }

    append_call(call);
}

void FramebufferStateBase::emit_buffer_calls_to_list(CallSet& list) const
{
    (void)list;
}

void FramebufferStateBase::do_clear()
{
}

void FramebufferStateBase::depends(PGenObjectState read_buffer)
{
    // TODO: we should check whether the drawn region is covering the
    // whole target so that we can remove un-needed calls
    m_read_dependencies.insert(read_buffer);
}

bool FramebufferStateBase::is_active() const
{
    return bound() ||  has_active_buffers();
}

bool FramebufferStateBase::has_active_buffers() const
{
    return true;
}

void FramebufferStateBase::do_emit_calls_to_list(CallSet& list) const
{
    emit_gen_call(list);
    emit_buffer_calls_to_list(list);

    list.insert(m_draw_prepare);

    for (auto& d : m_read_dependencies) {
        if (d)
            d->emit_calls_to_list(list);
    }

    emit_buffer_calls_to_list(list);
}

FBOState::FBOState(GLint glID, PCall gen_call):
    FramebufferStateBase(glID, gen_call),
    m_attached_color_buffer_mask(0)
{

}

void FBOState::bind_read(PCall call)
{
    m_bind_read_call = call;
}

void FBOState::bind_draw(PCall call)
{
    m_bind_draw_call = call;
}

void FBOState::emit_buffer_calls_to_list(CallSet& list) const
{
    if (m_bind_read_call)
        list.insert(m_bind_read_call);
    if (m_bind_draw_call)
        list.insert(m_bind_draw_call);

    for (auto& a : m_attach_calls)
        list.insert(a.second);

    for(auto& a: m_attachments)
        if (a.second)
            a.second->emit_calls_to_list(list);
}

bool FBOState::attach(unsigned attachment, PCall call,
                              PSizedObjectState att)
{
    if (m_attachments[attachment] && att &&
            (*m_attachments[attachment] == *att))
        return false;

    if (m_attachments[attachment]) {
        m_attachments[attachment]->unattach();
    }

    m_attachments[attachment] = att;
    m_attach_calls[attachment].clear();

    if (att) {
        set_size(att->width(), att->height());

        if (m_bind_read_call)
            m_attach_calls[attachment].insert(m_bind_read_call);
        if (m_bind_draw_call)
            m_attach_calls[attachment].insert(m_bind_draw_call);

        m_attach_calls[attachment].insert(call);
        att->attach();
    }

    unsigned attached_buffer_types = 0;
    m_attached_color_buffer_mask = 0;

    for(auto& a: m_attachments) {
        if (a.second) {
            switch (a.first) {
            case GL_DEPTH_ATTACHMENT:
                attached_buffer_types |= GL_DEPTH_BUFFER_BIT;
                break;
            case GL_STENCIL_ATTACHMENT:
                attached_buffer_types |= GL_STENCIL_BUFFER_BIT;
                break;
            default:
                attached_buffer_types |= GL_COLOR_BUFFER_BIT;
                m_attached_color_buffer_mask |= 1 << (a.first - GL_COLOR_ATTACHMENT0);
            }
        }
    }
    set_attachment_types(attached_buffer_types);
    return true;
}

void FBOState::do_clear()
{
    append_call(m_bind_draw_call);
    append_call(m_bind_read_call);
}

bool FBOState::has_active_buffers() const
{
    return m_bind_draw_call || m_bind_read_call;
}

void FramebufferMap::bind(PCall call)
{
    unsigned target = call->arg(0).toUInt();
    unsigned id = call->arg(1).toUInt();
    auto& gs = global_state();

    if (id)  {

        if ((target == GL_DRAW_FRAMEBUFFER ||
             target == GL_FRAMEBUFFER) &&
                (!m_draw_framebuffer ||
                 m_draw_framebuffer->id() != id)) {
            m_draw_framebuffer = get_by_id(id);
            m_draw_framebuffer->bind_draw(call);
            m_last_unbind_draw_fbo = nullptr;

            /* TODO: Create a copy of the current state and record all
          * calls in the framebuffer until it is un-bound
          * attach the framebuffer to any texture that might be
          * attached as render target */

            auto& calls = m_draw_framebuffer->state_calls();
            calls.clear();

            gs.collect_state_calls(calls);

            if (gs.in_target_frame())
                m_draw_framebuffer->emit_calls_to_list(gs.global_callset());
            //std::cerr << call->no <<  ": Currnet number of state calls:" << calls.size() << "\nco";
        }

        if ((target == GL_READ_FRAMEBUFFER ||
             target == GL_FRAMEBUFFER) &&
                (!m_read_framebuffer ||
                 m_read_framebuffer->id() != id)) {
            m_read_framebuffer = get_by_id(id);
            m_read_framebuffer->bind_read(call);
            m_last_unbind_read_fbo = nullptr;
            if (gs.in_target_frame())
                m_read_framebuffer->emit_calls_to_list(gs.global_callset());
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

    bool attachment_changed = false;

    if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
        attachment_changed |= m_draw_framebuffer->attach(attachment, call, rb);
        draw_fb = m_draw_framebuffer;
        if (rb && attachment_changed)
            rb->attach_as_rendertarget(draw_fb);
    }

    if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
        attachment_changed |= m_read_framebuffer->attach(attachment, call, rb);
    }

    if (rb && attachment_changed) {
        if(global_state().in_target_frame())
            rb->emit_calls_to_list(global_state().global_callset());
    }
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
    else if (!strcmp(call->name(), "glFramebufferTexture2D"))
        textarget = GL_TEXTURE_2D;
    else if (!strcmp(call->name(), "glFramebufferTexture1D"))
        textarget = GL_TEXTURE_2D;
    else if (!strcmp(call->name(), "glFramebufferTexture3D"))  {
        textarget = GL_TEXTURE_3D;
        layer = call->arg(5).toUInt();
    }

    unsigned texid = call->arg(2 + has_tex_target).toUInt();
    unsigned level = call->arg(3 + has_tex_target).toUInt();

    assert(level < 16);
    assert(layer < (1 << 12));

    unsigned textargetid = (textarget << 16) | level << 12 | level;

    auto texture = texid ? textures.get_by_id(texid) : nullptr;

    if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
        assert(m_draw_framebuffer);
        m_draw_framebuffer->attach(attachment, call, texture);
        if (texture)
            texture->rendertarget_of(textargetid, m_draw_framebuffer);
    }

    if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
        assert(m_read_framebuffer);
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

void FramebufferMap::glx_init_default_framebuffer(PCall call)
{

}

void FramebufferMap::do_emit_calls_to_list(CallSet& list) const
{
    if (m_last_unbind_draw_fbo)
        list.insert(m_last_unbind_draw_fbo);

    if (m_last_unbind_read_fbo)
        list.insert(m_last_unbind_read_fbo);
}

RenderbufferState::RenderbufferState(GLint glID, PCall gen_call):
    SizedObjectState(glID, gen_call, renderbuffer),
    m_attach_count(0)
{
}

void RenderbufferState::attach_as_rendertarget(PFramebufferState write_fb)
{
    ++m_attach_count;
    m_data_source = write_fb;
}

void RenderbufferState::detach()
{
    if (m_attach_count > 0)
        --m_attach_count;
    else
        std::cerr << "Try to detach unattached renderbuffer\n";
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
    if (is_attached()) {
        emit_gen_call(list);
        emit_bind(list);

        if (m_set_storage_call)
            list.insert(m_set_storage_call);

        if (m_is_blit_source) {
            assert(m_data_source);
            m_data_source->emit_calls_to_list(list);
        }
    }
}

void RenderbufferMap::storage(PCall call)
{
    assert(m_active_renderbuffer);
    m_active_renderbuffer->set_storage(call);
}

bool RenderbufferState::is_active() const
{
    return bound() || m_attach_count > 0;
}

void RenderbufferMap::post_bind(PCall call, PRenderbufferState obj)
{
    (void)call;
    m_active_renderbuffer = obj;
}

void RenderbufferMap::post_unbind(PCall call, PRenderbufferState obj)
{
    (void)call;
    (void)obj;
    m_active_renderbuffer = nullptr;
}

}
