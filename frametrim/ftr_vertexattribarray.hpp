#ifndef VERTEXATTRIBARRAY_HPP
#define VERTEXATTRIBARRAY_HPP

#include "ftr_bufobject.hpp"
#include "ftr_bufobject.hpp"

namespace frametrim_reverse {

class VertexAttribArray : public TraceObject
{
public:
    using Pointer = std::shared_ptr<VertexAttribArray>;

    void enable(unsigned  callno, bool enable);
    bool is_enabled(const TraceCallRange& range);
    void pointer(unsigned  callno, PBufObject obj);

private:
    std::list<std::pair<unsigned, unsigned>> m_enabled;
    BindTimeline m_buffer_timeline;
};
using PVertexAttribArray = VertexAttribArray::Pointer;

class VertexAttribArrayMap  {
public:
    PTraceCall pointer(const trace::Call& call, BufObjectMap& buffers);
    PTraceCall enable(const trace::Call& call, bool do_enable);
private:
    unsigned add_array(const trace::Call& call);

    std::vector<PVertexAttribArray> m_va;
};

}

#endif // VERTEXATTRIBARRAY_HPP
