#ifndef BUFFERSTATE_HPP
#define BUFFERSTATE_HPP

#include "ft_genobjectstate.hpp"

namespace frametrim {

class BufferState : public GenObjectState
{
public:
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

using PBufferState = std::shared_ptr<BufferState>;

}

#endif // BUFFERSTATE_HPP
