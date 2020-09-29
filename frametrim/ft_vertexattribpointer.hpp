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

    unsigned get_cache_id_helper() const override;

    ObjectType type() const override;

    bool is_active() const override;

    void do_emit_calls_to_list(CallSet& list) const override;

    void pass_state_cache(unsigned object_id, PCallSet cache) override;

    void emit_dependend_caches(CallSet& list) const override;

    PTraceCall m_set_call;
    PTraceCall m_enable_call;

    unsigned m_buffer_id;
    PCallSet m_buffer_state;

    bool m_enabled;
};
using PVertexAttribPointer = VertexAttribPointer::Pointer;

class VertexAttribPointerMap : public TObjStateMap<VertexAttribPointer> {
public:
    VertexAttribPointerMap();
    PTraceCall enable(const trace::Call& call, bool enable);
    PTraceCall set_data(const trace::Call& call, BufferStateMap& buffers);
    PCallSet state_cache() const;
private:
    PVertexAttribPointer get_or_create(const trace::Call& call);
    void do_emit_calls_to_list(CallSet& list) const override;

    mutable bool m_state_cache_dirty;
    mutable PCallSet m_state_cache;
};

}

#endif // VERTEXATTRIBPOINTER_HPP
