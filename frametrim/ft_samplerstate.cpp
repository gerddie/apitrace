#include "ft_samplerstate.hpp"

namespace frametrim {

void SamplerState::bind(PCall call)
{
    m_bind_call = call;
}

void SamplerState::do_emit_calls_to_list(CallSet& list) const
{
    if (m_bind_call)
        list.insert(m_bind_call);
}


void SamplerStateMap::bind(PCall call)
{
    unsigned id = call->arg(1).toUInt();
    unsigned unit = call->arg(0).toUInt();

    if (id > 0) {
        if (!m_bound_samplers[unit] ||
                m_bound_samplers[unit]->id() != id) {
            auto sampler = get_by_id(id);
            if (!sampler) {
                std::cerr << "No sampler with ID " << id << " found, ignore bind call\n";
                return;
            }
            sampler->bind(call);
            m_bound_samplers[unit] = sampler;
        }
    } else {
        if (m_bound_samplers[unit])
            m_bound_samplers[unit]->append_call(call);
        m_bound_samplers[unit] = nullptr;
    }
}

void SamplerStateMap::set_state(PCall call, unsigned addr_params)
{
    auto id = call->arg(0).toUInt();
    auto sampler = get_by_id(id);
    if (!sampler) {
        std::cerr << "No sampler with id " << id << " found in call "
                  << call->no;
        return;
    }
    sampler->set_state_call(call, addr_params);
}

void SamplerStateMap::do_emit_calls_to_list(CallSet& list) const
{
    for(auto& s : m_bound_samplers) {
        if (s.second)
            s.second->emit_calls_to_list(list);
    }
}

}