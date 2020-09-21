#ifndef BUFFERSTATE_HPP
#define BUFFERSTATE_HPP

#include "ft_genobjectstate.hpp"

namespace frametrim {

class BufferState : public GenObjectState
{
public:

    using Pointer = std::shared_ptr<BufferState>;

    BufferState(GLint glID, const trace::Call& gen_call);
    ~BufferState();

    void data(const trace::Call& call);
    void append_data(const trace::Call& call);

    void map(const trace::Call& call);
    void map_range(const trace::Call& call);
    void memcopy(const trace::Call& call);
    void unmap(const trace::Call& call);
    bool in_mapped_range(uint64_t address) const;

    void use(const trace::Call& call);

private:
    void post_bind(const trace::Call& call) override;
    void post_unbind(const trace::Call& call) override;
    void do_emit_calls_to_list(CallSet& list) const override;

    friend struct BufferStateImpl;
    struct BufferStateImpl *impl;
};

using PBufferState = BufferState::Pointer;

class BufferStateMap : public TGenObjStateMap<BufferState> {
  public:
    using TGenObjStateMap<BufferState>::TGenObjStateMap;

    void data(const trace::Call& call);
    void sub_data(const trace::Call& call);

    void map(const trace::Call& call);
    void map_range(const trace::Call& call);
    void memcpy(const trace::Call& call);
    void unmap(const trace::Call& call);

private:
    using BufferMap = std::unordered_map<GLint, PBufferState>;

    std::unordered_map<GLint, BufferMap> m_mapped_buffers;
};

}

#endif // BUFFERSTATE_HPP
