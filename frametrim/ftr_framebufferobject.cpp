#include "ftr_framebufferobject.hpp"

namespace frametrim_reverse {

void RenderbufferObject::evaluate_size(const trace::Call& call)
{
    unsigned w = call.arg(2).toUInt();
    unsigned h = call.arg(3).toUInt();
    set_size(0,w,h);
}

PTraceCall
RenderbufferObjectMap::storage(const trace::Call& call)
{

}

void
FramebufferObject::attach(unsigned index, PAttachableObject obj)
{
    m_attachments[index] = obj;
}

PTraceCall
FramebufferObjectMap::blit(const trace::Call& call)
{

}

PTraceCall
FramebufferObjectMap::attach_renderbuffer(const trace::Call& call, RenderbufferObjectMap& rb_map)
{

}

PTraceCall
FramebufferObjectMap::attach_texture(const trace::Call& call, TexObjectMap& tex_map)
{

}



}
