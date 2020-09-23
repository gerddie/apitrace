#include "ft_renderbuffer.hpp"

namespace frametrim {

RenderBuffer::RenderBuffer(GLint glID, PTraceCall gen_call):
    SizedObjectState(glID, gen_call, renderbuffer),
    m_is_blit_source(false)
{

}

void RenderBuffer::attach_as_rendertarget(PFramebufferState write_fb)
{
    attach();
    m_data_source = write_fb;
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

        if (m_set_storage_call)
            list.insert(m_set_storage_call);

        if (m_is_blit_source) {
            assert(m_data_source);
            m_data_source->emit_calls_to_list(list);
        }
    }
}

bool RenderBuffer::is_active() const
{
    return bound() || is_attached();
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