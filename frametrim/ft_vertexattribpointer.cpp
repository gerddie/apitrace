#include "ft_vertexattribpointer.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim {

VertexAttribPointer::VertexAttribPointer(GLint glID):
    ObjectState(glID)
{

}

PTraceCall
VertexAttribPointer::enable(const trace::Call &call, bool enable)
{
    m_enable_call = trace2call(call);
    m_enabled = enable;
    return m_enable_call;
}

PTraceCall
VertexAttribPointer::set_data(const trace::Call &call, PBufferState buf)
{
    m_set_call = trace2call(call);
    m_buf = buf;
    m_buf->flush_state_cache(*this);
    return m_set_call;
}

ObjectType VertexAttribPointer::type() const
{
    return bt_vertex_pointer;
}

bool VertexAttribPointer::is_active() const
{
    return m_enabled;
}

void VertexAttribPointer::do_emit_calls_to_list(CallSet& list) const
{
    list.insert(m_enable_call);
    if (m_enabled && m_set_call) {
        /* Enable can be called before setting the actual pointer,
         * so we might have to skip the set call */
        list.insert(m_set_call);
    }
}

PTraceCall VertexAttribPointerMap::enable(const trace::Call& call, bool enable)
{
    auto vap = get_or_create(call);
    return vap->enable(call, enable);
}


PTraceCall VertexAttribPointerMap::set_data(const trace::Call& call, BufferStateMap& buffers)
{
    auto vap = get_or_create(call);
    auto buf = buffers.bound_to(GL_ARRAY_BUFFER);
    return vap->set_data(call, buf);
}

PVertexAttribPointer
VertexAttribPointerMap::get_or_create(const trace::Call& call)
{
    unsigned id = call.arg(0).toUInt();
    auto vap = get_by_id(id);
    if (!vap) {
        vap = std::make_shared<VertexAttribPointer>(id);
        set(id, vap);
    }
    return vap;
}

void VertexAttribPointerMap::do_emit_calls_to_list(CallSet& list) const
{
    /* TODO> Why isn't this done in the base class */
    emit_all_states(list);
}

}