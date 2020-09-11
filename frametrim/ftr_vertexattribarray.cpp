#include "ftr_vertexattribarray.hpp"

#include <GL/gl.h>


namespace frametrim_reverse {

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

}

PTraceCall VertexAttribArrayMap::pointer(const trace::Call& call, BufObjectMap &buffers)
{
    auto buf = buffers.bound_to_target(GL_ARRAY_BUFFER);
    if (m_va)

}

PTraceCall VertexAttribArrayMap::enable(const trace::Call& call, bool do_enable)
{

}



}