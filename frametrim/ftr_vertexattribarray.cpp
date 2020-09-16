#include "ftr_vertexattribarray.hpp"
#include "ftr_boundobject.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim_reverse {

using std::make_shared;

void VertexAttribArray::enable(PTraceCall call)
{
    m_enabled_timeline.push_front(call);
}

PTraceCall
VertexAttribArray::enable_call(unsigned call_no) const
{
    for (auto&& e : m_enabled_timeline) {
        if (e->call_no() < call_no)
            return e;
    }
    return nullptr;
}

void VertexAttribArray::pointer(unsigned  callno, PBufObject obj)
{
    m_buffer_timeline.push(callno, obj);
}

PTraceCall VertexAttribArrayMap::pointer(const trace::Call& call, BufObjectMap &buffers)
{
    auto buf = buffers.bound_to_target(GL_ARRAY_BUFFER);
    unsigned index = add_array(call);
    m_va[index]->pointer(call.no, buf);
    return buf ? make_shared<TraceCallOnBoundObj>(call, buf):
                 make_shared<TraceCall>(call);
}

PTraceCall VertexAttribArrayMap::enable(const trace::Call& call)
{
    unsigned index = add_array(call);
    auto c = make_shared<TraceCall>(call);
    m_va[index]->enable(c);
    return c;
}

unsigned VertexAttribArrayMap::add_array(const trace::Call& call)
{
    auto index = call.arg(0).toUInt();
    if (m_va.size() <= index)
        m_va.resize(index + 1);

    if(!m_va[index])
        m_va[index] = make_shared<VertexAttribArray>();

    return index;
}


}