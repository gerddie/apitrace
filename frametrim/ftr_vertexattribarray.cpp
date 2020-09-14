#include "ftr_vertexattribarray.hpp"
#include "ftr_boundobject.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim_reverse {

using std::make_shared;

void VertexAttribArray::enable(unsigned  callno, bool enable)
{
    const unsigned ifinite = std::numeric_limits<unsigned>::max();

    if (enable) {
        if (m_enabled.empty() || m_enabled.front().second != ifinite)
            m_enabled.push_front(std::make_pair(callno, ifinite));
    } else {
        if (!m_enabled.empty())
            m_enabled.front().second = callno;
    }
}

bool VertexAttribArray::is_enabled(const TraceCallRange& range)
{
    for (auto&& e : m_enabled) {
        if (e.first < range.second &&
            e.second > range.first)
            return true;
    }
    return false;
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
    return make_shared<TraceCallOnBoundObj>(call, buf);
}

PTraceCall VertexAttribArrayMap::enable(const trace::Call& call, bool do_enable)
{
    unsigned index = add_array(call);
    m_va[index]->enable(call.no, do_enable);
    return make_shared<TraceCall>(call);
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