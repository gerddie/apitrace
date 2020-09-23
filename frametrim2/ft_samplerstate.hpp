#ifndef SAMPLERSTATE_HPP
#define SAMPLERSTATE_HPP

#include "ft_genobjectstate.hpp"

namespace frametrim {


class SamplerState : public GenObjectState
{
public:
    using Pointer = std::shared_ptr<SamplerState>;
    using GenObjectState::GenObjectState;

private:
    ObjectType type() const override {return bt_sampler;}

    void do_emit_calls_to_list(CallSet& list) const override;
};

using PSamplwerState = SamplerState::Pointer;

class SamplerStateMap : public TGenObjStateMap<SamplerState>
{
public:
    using TGenObjStateMap<SamplerState>::TGenObjStateMap;

    PTraceCall set_state(const trace::Call& call, unsigned addr_params);
private:

    void do_emit_calls_to_list(CallSet& list) const override;
};


}

#endif // SAMPLERSTATE_HPP
