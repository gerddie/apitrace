#ifndef SAMPLERSTATE_HPP
#define SAMPLERSTATE_HPP

#include "ft_genobjectstate.hpp"

namespace frametrim {


class SamplerState : public GenObjectState
{
public:
    using Pointer = std::shared_ptr<SamplerState>;
    using GenObjectState::GenObjectState;

    void bind(PCall call);

private:
    void do_emit_calls_to_list(CallSet& list) const override;
    PCall m_bind_call;

};

using PSamplwerState = SamplerState::Pointer;

class SamplerStateMap : public TGenObjStateMap<SamplerState>
{
public:
    using TGenObjStateMap<SamplerState>::TGenObjStateMap;

    void bind(PCall call);

    void set_state(PCall call, unsigned addr_params);
private:

    std::unordered_map<unsigned, PSamplwerState> m_bound_samplers;
};


}

#endif // SAMPLERSTATE_HPP
