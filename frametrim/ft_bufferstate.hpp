#ifndef BUFFERSTATE_HPP
#define BUFFERSTATE_HPP

#include "ft_genobjectstate.hpp"

namespace frametrim {

class BufferState : public GenObjectState
{
public:

    using Pointer = std::shared_ptr<BufferState>;

    BufferState(GLint glID, PTraceCall gen_call);
    ~BufferState();

    PTraceCall data(const trace::Call& call);
    PTraceCall append_data(const trace::Call& call);

    PTraceCall map(const trace::Call& call);
    PTraceCall map_range(const trace::Call& call);
    PTraceCall memcopy(const trace::Call& call);
    PTraceCall unmap(const trace::Call& call);
    bool in_mapped_range(uint64_t address) const;
    PTraceCall flush(const trace::Call& call);
    PTraceCall use(const trace::Call& call);

    PTraceCall bind_target_call(unsigned target) const;

private:
    ObjectType type() const override {return bt_buffer;}
    void post_bind(unsigned target, const PTraceCall& call) override;
    void post_unbind(const PTraceCall& call) override;
    void do_emit_calls_to_list(CallSet& list) const override;

    friend struct BufferStateImpl;
    struct BufferStateImpl *impl;
};

using PBufferState = BufferState::Pointer;

class BufferStateMap : public TGenObjStateMap<BufferState> {
  public:
    using TGenObjStateMap<BufferState>::TGenObjStateMap;

    PTraceCall data(const trace::Call& call);
    PTraceCall sub_data(const trace::Call& call);

    PTraceCall map(const trace::Call& call);
    PTraceCall map_range(const trace::Call& call);
    PTraceCall memcpy(const trace::Call& call);
    PTraceCall unmap(const trace::Call& call);
    PTraceCall flush(const trace::Call& call);

    static unsigned composed_target_id(unsigned target, unsigned index);
private:
    unsigned target_id_from_call(const trace::Call& call) const override;


    using BufferMap = std::unordered_map<GLint, PBufferState>;

    std::unordered_map<GLint, BufferMap> m_mapped_buffers;
};

}

#endif // BUFFERSTATE_HPP
