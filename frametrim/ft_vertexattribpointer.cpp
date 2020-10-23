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
    dirty_cache();
    return m_enable_call;
}

PTraceCall
VertexAttribPointer::set_data(const trace::Call &call, PBufferState buf)
{
    m_set_call = trace2call(call);
    if (buf) {
        auto bind_call = buf->bind_target_call(GL_ARRAY_BUFFER);
        m_set_call->set_required_call(bind_call);
        buf->flush_state_cache(*this);
    } else {
        auto offset = (uint64_t)call.arg(5).toPointer();
        if (offset && offset < 1000) {
            std::cerr << "\nW: Call:" << call.no << ": "
                      << call.name() << " suspicious, pointer argument seems "
                                        "to be an offset ("
                      << offset << ") but no buffer bound\n";
        }
    }
    dirty_cache();
    return m_set_call;
}

ObjectType VertexAttribPointer::type() const
{
    return bt_vertex_pointer;
}

bool VertexAttribPointer::is_active() const
{
    return true;
}

unsigned VertexAttribPointer::get_cache_id_helper() const
{
    return fence_id();
}

void VertexAttribPointer::do_emit_calls_to_list(CallSet& list) const
{
    if (m_enable_call)
        list.insert(m_enable_call);

    if (m_set_call) {
        /* Enable can be called before setting the actual pointer,
         * so we might have to skip the set call */
        list.insert(m_set_call);
    }
}

void VertexAttribPointer::emit_dependend_caches(CallSet& list) const
{
    if (m_buffer_state)
        list.insert(m_buffer_id, m_buffer_state);
}

void VertexAttribPointer::pass_state_cache(unsigned object_id, PCallSet cache)
{
    m_buffer_id = object_id;
    m_buffer_state = cache;
    dirty_cache();
}

VertexAttribPointerMap::VertexAttribPointerMap():
    m_state_cache_dirty(true)
{
}

PTraceCall VertexAttribPointerMap::enable(const trace::Call& call, bool enable)
{
    auto vap = get_or_create(call);
    m_state_cache_dirty = true;
    return vap->enable(call, enable);
}


PTraceCall VertexAttribPointerMap::set_data(const trace::Call& call, BufferStateMap& buffers)
{
    auto vap = get_or_create(call);
    unsigned target = buffers.composed_target_id(GL_ARRAY_BUFFER, 0);
    auto buf = buffers.bound_to(target);
    m_state_cache_dirty = true;
    return vap->set_data(call, buf);
}

PVertexAttribPointer
VertexAttribPointerMap::get_or_create(const trace::Call& call)
{
    unsigned id = call.arg(0).toUInt();
    auto vap = get_by_id(id, false);
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

PCallSet VertexAttribPointerMap::state_cache() const
{
    if (m_state_cache_dirty) {
        m_state_cache = std::make_shared<CallSet>();
        emit_calls_to_list(*m_state_cache);
        m_state_cache_dirty = false;
    }
    return m_state_cache;
}

}