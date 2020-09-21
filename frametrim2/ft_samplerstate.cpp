#include "ft_samplerstate.hpp"

namespace frametrim {

void SamplerState::do_emit_calls_to_list(CallSet& list) const
{
    emit_gen_call(list);
}

void SamplerStateMap::set_state(const trace::Call &call, unsigned addr_params)
{
    auto id = call.arg(0).toUInt();
    auto sampler = get_by_id(id);
    if (!sampler) {
        std::cerr << "No sampler with id " << id << " found in call "
                  << call.no;
        return;
    }
    sampler->set_state_call(call, addr_params);
}

void SamplerStateMap::do_emit_calls_to_list(CallSet& list) const
{
    (void)list;
}

}