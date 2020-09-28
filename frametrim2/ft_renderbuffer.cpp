#include "ft_renderbuffer.hpp"

namespace frametrim {

RenderBuffer::RenderBuffer(GLint glID, PTraceCall gen_call):
    SizedObjectState(glID, gen_call, renderbuffer),
    m_creator_id(0),
    m_is_blit_source(false)
{

}

void RenderBuffer::attach_as_rendertarget(PTraceCall attach_call)
{
    m_attach_call = attach_call;
    attach();
}

void RenderBuffer::detach()
{
    unattach();
}

void RenderBuffer::set_used_as_blit_source()
{
    m_is_blit_source = true;
}

PTraceCall
RenderBuffer::set_storage(const trace::Call& call)
{
    m_set_storage_call = trace2call(call);
    unsigned w = call.arg(2).toUInt();
    unsigned h = call.arg(2).toUInt();
    set_size(0, w, h);
    m_set_storage_call->set_required_call(bind_call());
    dirty_cache();
    return m_set_storage_call;
}

void RenderBuffer::do_emit_calls_to_list(CallSet& list) const
{
    if (is_attached()) {
        emit_gen_call(list);
        emit_bind(list);
        list.insert(m_attach_call);

        if (m_set_storage_call)
            list.insert(m_set_storage_call);
    }
}

bool RenderBuffer::is_active() const
{
    return bound() || is_attached();
}


void RenderBuffer::pass_state_cache(unsigned object_id, PCallSet cache)
{
    m_creator_id = object_id;
    m_creator_state = cache;
}

void RenderBuffer::emit_dependend_caches(CallSet& list) const
{
    if (m_creator_state)
        list.insert(m_creator_id, m_creator_state);
}

PTraceCall
RenderbufferMap::storage(const trace::Call& call)
{
    assert(m_active_renderbuffer);
    return m_active_renderbuffer->set_storage(call);
}


void RenderbufferMap::post_bind(unsigned target, RenderBuffer::Pointer obj)
{
    (void)target;
    m_active_renderbuffer = obj;
}

void RenderbufferMap::post_unbind(unsigned target, PTraceCall call)
{
    (void)target;
    (void)call;
    m_active_renderbuffer = nullptr;
}

}