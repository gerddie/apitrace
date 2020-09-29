#ifndef VERTEXATTRIBPOINTER_HPP
#define VERTEXATTRIBPOINTER_HPP

#include "ft_bufferstate.hpp"

namespace frametrim {

class VertexAttribPointer : public ObjectState
{
public:
    using Pointer = std::shared_ptr<VertexAttribPointer>;

    VertexAttribPointer(GLint glID);

    PTraceCall enable(const trace::Call& call, bool enable);

    PTraceCall set_data(const trace::Call &call, PBufferState buf);

private:

    ObjectType type() const override;

    bool is_active() const override;

    void do_emit_calls_to_list(CallSet& list) const override;

    PBufferState m_buf;
    PTraceCall m_set_call;
    PTraceCall m_enable_call;
    bool m_enabled;
};
using PVertexAttribPointer = VertexAttribPointer::Pointer;

class VertexAttribPointerMap : public TObjStateMap<VertexAttribPointer> {
public:

    PTraceCall enable(const trace::Call& call, bool enable);
    PTraceCall set_data(const trace::Call& call, BufferStateMap& buffers);
private:
    PVertexAttribPointer get_or_create(const trace::Call& call);
    void do_emit_calls_to_list(CallSet& list) const override;
};

}

#endif // VERTEXATTRIBPOINTER_HPP
