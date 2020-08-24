#ifndef BUFFERSTATE_HPP
#define BUFFERSTATE_HPP

#include "ft_genobjectstate.hpp"

namespace frametrim {

class BufferState : public GenObjectState
{
public:

    using Pointer = std::shared_ptr<BufferState>;

    BufferState(GLint glID, PCall gen_call);
    ~BufferState();

    void bind(PCall call);
    void data(PCall call);
    void append_data(PCall call);

    void map(PCall call);
    void map_range(PCall call);
    void memcopy(PCall call);
    void unmap(PCall call);
    bool in_mapped_range(uint64_t address) const;

    void use(PCall call = nullptr);

private:
    void do_emit_calls_to_list(CallSet& list) const override;

    struct BufferStateImpl *impl;
};

using PBufferState = BufferState::Pointer;

class BufferStateMap : public TStateMap<BufferState> {
  public:
    using TStateMap<BufferState>::TStateMap;

    PBufferState bound_to(unsigned target);

    void bind(PCall call);
    void data(PCall call);
    void sub_data(PCall call);

    void map(PCall call);
    void map_range(PCall call);
    void memcpy(PCall call);
    void unmap(PCall call);

    void emit_calls_to_list(CallSet& list) const;

private:

    using BufferMap = std::unordered_map<GLint, PBufferState>;

    BufferMap m_bound_buffers;
    std::unordered_map<GLint, BufferMap> m_mapped_buffers;
};

}

#endif // BUFFERSTATE_HPP
