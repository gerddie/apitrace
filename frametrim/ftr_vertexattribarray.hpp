#ifndef VERTEXATTRIBARRAY_HPP
#define VERTEXATTRIBARRAY_HPP

#include "ftr_bufobject.hpp"
#include "ftr_bufobject.hpp"

namespace frametrim_reverse {

class VertexAttribArray : public TraceObject
{
public:
    using Pointer = std::shared_ptr<VertexAttribArray>;

    void enable(PTraceCall call);
    PTraceCall enable_call(unsigned call_no) const;
    void pointer(unsigned  callno, PBufObject obj);
    void accept(ObjectVisitor& visitor) override {visitor.visit(*this);};
private:
    std::list<PTraceCall> m_enabled_timeline;
    BindTimeline<PBufObject> m_buffer_timeline;
};
using PVertexAttribArray = VertexAttribArray::Pointer;

class VertexAttribArrayMap  {
public:
    PTraceCall pointer(const trace::Call& call, BufObjectMap& buffers);
    PTraceCall enable(const trace::Call& call);

private:
    unsigned add_array(const trace::Call& call);

    std::vector<PVertexAttribArray> m_va;
};

}

#endif // VERTEXATTRIBARRAY_HPP
